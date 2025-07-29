#include <jni.h>
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <android/log.h>
#include "AndroidVideoDecoderWrapper.h"
#include "CrashSafeFrameHandler.h"
#include "FrameRenderer.h"
#define LOG_TAG "MRDesk.Client"
#include "Logging.h"
#include "NetworkReceiver.h"
#include "protocol.h"

static JavaVM* g_vm = nullptr;
static jclass g_clientClass = nullptr;
static jmethodID g_onFrameMethod = nullptr;

// #define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
// #define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG,
// __VA_ARGS__)

class AndroidNetworkClient {
 private:
  std::unique_ptr<NetworkReceiver> receiver;
  FrameRenderer frameRenderer;
  std::atomic<bool> running{false};
  std::mutex frameMutex; // Prevent race conditions in frame processing
  int frameCount = 0; // For debugging purposes
  std::thread pollingThread; // Background thread for polling frames

  // FPS tracking
  std::chrono::steady_clock::time_point lastFPSLog =
      std::chrono::steady_clock::now();
  int framesSinceLastLog = 0;
  const std::chrono::seconds FPS_LOG_INTERVAL{10}; // Log every 10 seconds
  std::shared_ptr<spdlog::logger> logger;

 public:
  AndroidNetworkClient() {
    // Initialize logger
    logger = MRDesk::GetLogger("MRDesk.AndroidClient");

    // Create NetworkReceiver with Android hardware decoder
    auto androidDecoder = std::make_unique<AndroidVideoDecoderWrapper>();
    receiver = std::make_unique<NetworkReceiver>(std::move(androidDecoder));
  }

  bool Connect(const std::string& serverIP, int port) {
    LOGI("Attempting to connect to {}:{}", serverIP, port);

    // Using H.264 for better Android MediaCodec compatibility
    CompressionType compression = COMPRESSION_H264;
    receiver->SetCompression(compression);
    LOGI("Set compression type: {}", static_cast<int>(compression));

    receiver->SetFrameCallback(
        [this](const FrameMessage& msg, const std::vector<uint8_t>& data) {
          LOGI(
              "Frame callback invoked({}): {}x{}, {} bytes",
              frameCount++,
              msg.width,
              msg.height,
              data.size());

          // Track FPS
          framesSinceLastLog++;
          auto now = std::chrono::steady_clock::now();
          auto timeSinceLastLog =
              std::chrono::duration_cast<std::chrono::seconds>(
                  now - lastFPSLog);

          if (timeSinceLastLog >= FPS_LOG_INTERVAL) {
            double fps = static_cast<double>(framesSinceLastLog) /
                timeSinceLastLog.count();
            logger->info(
                "Android Client FPS: {:.2f} ({} frames in {} seconds)",
                fps,
                framesSinceLastLog,
                timeSinceLastLog.count());

            // Reset counters
            framesSinceLastLog = 0;
            lastFPSLog = now;
          }

          // Use mutex to prevent race conditions
          // std::lock_guard<std::mutex> lock(frameMutex);

          // Log frame info and check crash safety
          CrashSafeFrameHandler::LogFrameInfo(
              msg.width, msg.height, data.size());

          if (CrashSafeFrameHandler::ShouldSkipFrameProcessing()) {
            LOGE("Skipping frame processing due to previous crashes");
            return;
          }

          if (!CrashSafeFrameHandler::IsFrameRenderingEnabled()) {
            LOGI("Frame rendering disabled, skipping conversion");
            return;
          }

          // SAFE frame rate limiting: process all frames but limit
          // rendering/display
          static auto lastRenderTime = std::chrono::steady_clock::now();
          auto currentTime = std::chrono::steady_clock::now();
          auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(
              currentTime - lastRenderTime);

          bool shouldRender = timeDiff.count() >= 50; // ~20 FPS max
          if (shouldRender) {
            lastRenderTime = currentTime;
          }

          LOGI(
              "Processing frame: {}x{}, {} bytes",
              msg.width,
              msg.height,
              data.size());

          // Additional validation for frame size limits
          const size_t maxFrameSize = 50 * 1024 * 1024; // 50MB limit
          if (data.size() > maxFrameSize) {
            LOGE("Frame too large: %zu bytes, skipping", data.size());
            return;
          }

          // Convert YUV to ARGB format for Android display
          std::vector<uint8_t> argbData;
          if (!frameRenderer.ConvertYUVToARGB(
                  data, msg.width, msg.height, argbData)) {
            LOGE("Failed to convert YUV frame to ARGB");
            CrashSafeFrameHandler::DisableFrameRendering();
            return;
          }

          // Only send to Java layer if we should render this frame (rate
          // limiting)
          if (shouldRender && g_vm && g_onFrameMethod && g_clientClass &&
              !argbData.empty()) {
            JNIEnv* env = nullptr;
            bool attached = false;

            // Get JNI environment safely
            jint getEnvResult =
                g_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
            if (getEnvResult == JNI_EDETACHED) {
              if (g_vm->AttachCurrentThread(&env, nullptr) == JNI_OK) {
                attached = true;
              } else {
                LOGE("Failed to attach thread to JVM");
                return;
              }
            } else if (getEnvResult != JNI_OK || env == nullptr) {
              LOGE("Failed to get JNI environment: %d", getEnvResult);
              return;
            }

            try {
              // Limit array size to prevent OOM and add safety margin
              const size_t maxArraySize =
                  16 * 1024 * 1024; // Reduced to 16MB limit
              if (argbData.size() > maxArraySize) {
                LOGE(
                    "Frame too large for JNI transfer: %zu bytes",
                    argbData.size());
                if (attached)
                  g_vm->DetachCurrentThread();
                return;
              }

              // Check if we have enough memory before creating large arrays
              if (argbData.empty()) {
                LOGE("ARGB data is empty after conversion");
                if (attached)
                  g_vm->DetachCurrentThread();
                return;
              }

              // Create Java byte array with error checking
              jbyteArray arr =
                  env->NewByteArray(static_cast<jsize>(argbData.size()));
              if (arr == nullptr) {
                LOGE("Failed to create Java byte array for frame data");
                if (env->ExceptionCheck()) {
                  env->ExceptionDescribe();
                  env->ExceptionClear();
                }
                if (attached)
                  g_vm->DetachCurrentThread();
                return;
              }

              // Copy data safely in chunks to avoid large memory operations
              const size_t chunkSize = 1024 * 1024; // 1MB chunks
              for (size_t offset = 0; offset < argbData.size();
                   offset += chunkSize) {
                size_t currentChunkSize =
                    std::min(chunkSize, argbData.size() - offset);
                env->SetByteArrayRegion(
                    arr,
                    static_cast<jsize>(offset),
                    static_cast<jsize>(currentChunkSize),
                    reinterpret_cast<const jbyte*>(argbData.data() + offset));

                if (env->ExceptionCheck()) {
                  LOGE(
                      "Exception while setting byte array region at offset %zu",
                      offset);
                  env->ExceptionDescribe();
                  env->ExceptionClear();
                  env->DeleteLocalRef(arr);
                  if (attached)
                    g_vm->DetachCurrentThread();
                  return;
                }
              }

              // Call Java callback with frame data
              env->CallStaticVoidMethod(
                  g_clientClass,
                  g_onFrameMethod,
                  arr,
                  (jint)msg.width,
                  (jint)msg.height);

              if (env->ExceptionCheck()) {
                LOGE("Exception while calling Java frame callback");
                env->ExceptionDescribe();
                env->ExceptionClear();
              } else {
                LOGI(
                    "Frame sent to Java layer: {}x{}, {} ARGB bytes",
                    msg.width,
                    msg.height,
                    argbData.size());
              }

              env->DeleteLocalRef(arr);

            } catch (...) {
              LOGE("Exception in JNI frame transfer");
            }

            if (attached) {
              g_vm->DetachCurrentThread();
            }
          } else {
            if (!shouldRender) {
              LOGI("Frame processed but not rendered (rate limited)");
            } else {
              if (!g_vm)
                LOGE("JavaVM not initialized");
              if (!g_onFrameMethod)
                LOGE("Frame callback method not found");
              if (!g_clientClass)
                LOGE("Client class not initialized");
              if (argbData.empty())
                LOGE("ARGB data is empty");
            }
          }
        });

    receiver->SetErrorCallback([](const std::string& error) {
      LOGE("Network error: %s", error.c_str());
    });

    receiver->SetDisconnectedCallback([]() {
      LOGI("Disconnected from server");
    });

    bool connected = receiver->Connect(serverIP, port);
    if (connected) {
      LOGI("Successfully connected to server");
      running = true;

      // Start polling thread to ensure frames are processed
      pollingThread = std::thread([this]() {
        LOGI("Frame polling thread started");
        while (running) {
          if (receiver->PollFrame()) {
            LOGI("PollFrame returned true - frame was available");
          }
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        LOGI("Frame polling thread stopped");
      });
    } else {
      LOGE("Failed to connect to server");
    }

    return connected;
  }

  void Disconnect() {
    LOGI("Disconnecting from server");
    running = false;
    receiver->Disconnect();

    // Wait for polling thread to finish
    if (pollingThread.joinable()) {
      LOGI("Waiting for polling thread to finish");
      pollingThread.join();
      LOGI("Polling thread finished");
    }
  }

  bool SendMouseMove(
      int32_t deltaX,
      int32_t deltaY,
      bool absolute = false,
      int32_t x = 0,
      int32_t y = 0) {
    return receiver->SendMouseMove(deltaX, deltaY, absolute, x, y);
  }

  bool SendMouseClick(int button, bool pressed) {
    MouseClickMessage::MouseButton btn;
    switch (button) {
      case 0:
        btn = MouseClickMessage::LEFT_BUTTON;
        break;
      case 1:
        btn = MouseClickMessage::RIGHT_BUTTON;
        break;
      case 2:
        btn = MouseClickMessage::MIDDLE_BUTTON;
        break;
      default:
        return false;
    }
    return receiver->SendMouseClick(btn, pressed);
  }

  bool IsConnected() const { return receiver->IsConnected(); }
};

static AndroidNetworkClient* g_client = nullptr;

extern "C" {

JNIEXPORT jboolean JNICALL Java_com_mrdesktop_MRDesktopClient_nativeConnect(
    JNIEnv* env, jobject thiz, jstring serverIP, jint port) {
  if (!g_client) {
    g_client = new AndroidNetworkClient();
  }

  const char* ip = env->GetStringUTFChars(serverIP, nullptr);
  bool result = g_client->Connect(std::string(ip), port);
  env->ReleaseStringUTFChars(serverIP, ip);

  return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_mrdesktop_MRDesktopClient_nativeDisconnect(JNIEnv* env, jobject thiz) {
  if (g_client) {
    g_client->Disconnect();
    delete g_client;
    g_client = nullptr;
  }
}

JNIEXPORT jboolean JNICALL Java_com_mrdesktop_MRDesktopClient_nativeIsConnected(
    JNIEnv* env, jobject thiz) {
  if (!g_client) {
    return JNI_FALSE;
  }
  return g_client->IsConnected() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_mrdesktop_MRDesktopClient_nativeSendMouseMove(
    JNIEnv* env, jobject thiz, jint deltaX, jint deltaY) {
  if (!g_client) {
    return JNI_FALSE;
  }
  return g_client->SendMouseMove(deltaX, deltaY) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_mrdesktop_MRDesktopClient_nativeSendMouseMoveAbsolute(
    JNIEnv* env, jobject thiz, jint x, jint y) {
  if (!g_client) {
    return JNI_FALSE;
  }
  return g_client->SendMouseMove(0, 0, true, x, y) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_mrdesktop_MRDesktopClient_nativeSendMouseClick(
    JNIEnv* env, jobject thiz, jint button, jboolean pressed) {
  if (!g_client) {
    return JNI_FALSE;
  }
  return g_client->SendMouseClick(button, pressed == JNI_TRUE)
      ? JNI_TRUE
      : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_mrdesktop_MRDesktopClient_nativeSetFrameCallback(
    JNIEnv* env, jclass clazz) {
  LOGI("Setting up frame callback from Java");

  env->GetJavaVM(&g_vm);
  if (g_clientClass) {
    env->DeleteGlobalRef(g_clientClass);
    g_clientClass = nullptr;
  }
  g_clientClass = reinterpret_cast<jclass>(env->NewGlobalRef(clazz));
  g_onFrameMethod =
      env->GetStaticMethodID(g_clientClass, "onFrameReceived", "([BII)V");

  if (g_onFrameMethod == nullptr) {
    LOGE("Failed to find onFrameReceived method");
  } else {
    LOGI("Frame callback setup completed successfully");
  }
}
}