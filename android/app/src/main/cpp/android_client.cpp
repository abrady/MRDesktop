#include <jni.h>
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <android/log.h>
#include <android/native_window_jni.h>
#include "AndroidSurfaceVideoDecoder.h"
#include "CrashSafeFrameHandler.h"
#define LOG_TAG "MRDesk.Client"
#include "Logging.h"
#include "NetworkReceiver.h"
#include "protocol.h"

static JavaVM* g_vm = nullptr;
static jclass g_clientClass = nullptr;
static jmethodID g_onSurfaceFrameMethod = nullptr;

// #define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
// #define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG,
// __VA_ARGS__)

class AndroidNetworkClient {
private:
    std::unique_ptr<NetworkReceiver> receiver;
    std::unique_ptr<AndroidSurfaceVideoDecoder> surfaceDecoder;
    std::atomic<bool> running{false};
    std::mutex frameMutex;
    int frameCount = 0;
    std::thread pollingThread;
    
    // Surface management
    ANativeWindow* nativeWindow = nullptr;
    uint32_t currentTextureId = 0;

    // FPS tracking
    std::chrono::steady_clock::time_point lastFPSLog = std::chrono::steady_clock::now();
    int framesSinceLastLog = 0;
    const std::chrono::seconds FPS_LOG_INTERVAL{5}; // Log every 5 seconds
    std::shared_ptr<spdlog::logger> logger;

public:
    AndroidNetworkClient() {
        logger = MRDesk::GetLogger("MRDesk.AndroidClient");
        
        // Create AndroidSurfaceVideoDecoder instead of regular decoder
        surfaceDecoder = std::make_unique<AndroidSurfaceVideoDecoder>();
        
        // Create NetworkReceiver with our Surface decoder
        receiver = std::make_unique<NetworkReceiver>(
            std::unique_ptr<IVideoDecoder>(new AndroidSurfaceVideoDecoder()));
    }

    bool InitializeSurface(ANativeWindow* surface) {
        if (!surface) {
            LOGE("Surface is null");
            return false;
        }
        
        nativeWindow = surface;
        LOGI("Surface initialized for Android client");
        return true;
    }

    bool Connect(const std::string& serverIP, int port) {
        LOGI("Android Client attempting to connect to {}:{}", serverIP, port);

        // Using H.264 for hardware MediaCodec decoding
        CompressionType compression = COMPRESSION_H264;
        receiver->SetCompression(compression);
        LOGI("Set compression type for Android: {}", static_cast<int>(compression));

        receiver->SetFrameCallback(
            [this](const FrameMessage& msg, const std::vector<uint8_t>& data) {
                LOGI("Surface Frame callback: {}x{}, {} bytes (Surface-decoded)", 
                     msg.width, msg.height, data.size());

                framesSinceLastLog++;
                auto now = std::chrono::steady_clock::now();
                auto timeSinceLastLog = std::chrono::duration_cast<std::chrono::seconds>(
                    now - lastFPSLog);

                if (timeSinceLastLog >= FPS_LOG_INTERVAL) {
                    double fps = static_cast<double>(framesSinceLastLog) / timeSinceLastLog.count();
                    logger->info("Android Client FPS: {:.2f} (Surface decoding - no CPU conversion)", fps);
                    
                    framesSinceLastLog = 0;
                    lastFPSLog = now;
                }

                // For Surface decoding, we don't process pixel data
                // The frame is already rendered to the Surface/texture
                // Just notify Java layer that a new frame is available
                NotifyFrameAvailable(msg.width, msg.height);

                frameCount++;
            });

        receiver->SetErrorCallback([](const std::string& error) {
            LOGE("Android Network error: %s", error.c_str());
        });

        receiver->SetDisconnectedCallback([]() {
            LOGI("Android Client disconnected from server");
        });

        bool connected = receiver->Connect(serverIP, port);
        if (connected) {
            LOGI("Android Client connected successfully");
            receiver->SendPixelFormatRequest(PIXEL_FORMAT_YUV420); // MediaCodec natural format
            
            // Initialize decoder with Surface after connection
            if (nativeWindow && !surfaceDecoder->IsInitialized()) {
                if (!surfaceDecoder->InitializeWithSurface(640, 480, COMPRESSION_H264, nativeWindow)) {
                    LOGE("Failed to initialize Surface decoder");
                    return false;
                }
            }
            
            running = true;

            // Start polling thread
            pollingThread = std::thread([this]() {
                LOGI("Android Frame polling thread started");
                while (running) {
                    if (receiver->PollFrame()) {
                        // Frame processed through Surface callback
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(8)); // ~120fps polling
                }
                LOGI("Android Frame polling thread stopped");
            });
        } else {
            LOGE("Android Client failed to connect to server");
        }

        return connected;
    }

    void NotifyFrameAvailable(int width, int height) {
        if (g_vm && g_onSurfaceFrameMethod && g_clientClass) {
            JNIEnv* env = nullptr;
            bool attached = false;

            jint getEnvResult = g_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
            if (getEnvResult == JNI_EDETACHED) {
                if (g_vm->AttachCurrentThread(&env, nullptr) == JNI_OK) {
                    attached = true;
                } else {
                    LOGE("Failed to attach thread for Surface frame notification");
                    return;
                }
            } else if (getEnvResult != JNI_OK || env == nullptr) {
                LOGE("Failed to get JNI environment for Surface frame");
                return;
            }

            // Call Java callback with texture ID and dimensions
            env->CallStaticVoidMethod(
                g_clientClass,
                g_onSurfaceFrameMethod,
                (jint)currentTextureId,
                (jint)width,
                (jint)height);

            if (env->ExceptionCheck()) {
                LOGE("Exception while calling Surface frame callback");
                env->ExceptionDescribe();
                env->ExceptionClear();
            } else {
                LOGD("Surface frame notification sent to Java: texture={}, {}x{}", 
                     currentTextureId, width, height);
            }

            if (attached) {
                g_vm->DetachCurrentThread();
            }
        }
    }

    void Disconnect() {
        LOGI("Android Client disconnecting");
        running = false;
        receiver->Disconnect();

        if (pollingThread.joinable()) {
            pollingThread.join();
        }
    }

    bool SendMouseMove(int32_t deltaX, int32_t deltaY, bool absolute = false, int32_t x = 0, int32_t y = 0) {
        return receiver->SendMouseMove(deltaX, deltaY, absolute, x, y);
    }

    bool SendMouseClick(int button, bool pressed) {
        MouseClickMessage::MouseButton btn;
        switch (button) {
            case 0: btn = MouseClickMessage::LEFT_BUTTON; break;
            case 1: btn = MouseClickMessage::RIGHT_BUTTON; break;
            case 2: btn = MouseClickMessage::MIDDLE_BUTTON; break;
            default: return false;
        }
        return receiver->SendMouseClick(btn, pressed);
    }

    bool IsConnected() const {
        return receiver->IsConnected();
    }

    void SetTextureId(uint32_t textureId) {
        currentTextureId = textureId;
    }

    void Cleanup() {
        if (nativeWindow) {
            ANativeWindow_release(nativeWindow);
            nativeWindow = nullptr;
        }
    }
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

JNIEXPORT void JNICALL Java_com_mrdesktop_MRDesktopClient_nativeDisconnect(
    JNIEnv* env, jobject thiz) {
    if (g_client) {
        g_client->Disconnect();
        g_client->Cleanup();
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

JNIEXPORT void JNICALL Java_com_mrdesktop_MRDesktopClient_nativeSetSurfaceFrameCallback(
    JNIEnv* env, jclass clazz) {
    LOGI("Setting up Surface frame callback for Android");

    env->GetJavaVM(&g_vm);
    if (g_clientClass) {
        env->DeleteGlobalRef(g_clientClass);
        g_clientClass = nullptr;
    }
    g_clientClass = reinterpret_cast<jclass>(env->NewGlobalRef(clazz));
    g_onSurfaceFrameMethod = env->GetStaticMethodID(
        g_clientClass, "onSurfaceFrameAvailable", "(III)V");

    if (g_onSurfaceFrameMethod == nullptr) {
        LOGE("Failed to find onSurfaceFrameAvailable method");
    } else {
        LOGI("Android Surface frame callback setup completed");
    }
}

JNIEXPORT jboolean JNICALL Java_com_mrdesktop_MRDesktopClient_nativeInitializeSurface(
    JNIEnv* env, jobject thiz, jobject surface) {
    if (!g_client) {
        g_client = new AndroidNetworkClient();
    }

    ANativeWindow* nativeWindow = ANativeWindow_fromSurface(env, surface);
    if (!nativeWindow) {
        LOGE("Failed to get ANativeWindow from Surface");
        return JNI_FALSE;
    }

    bool result = g_client->InitializeSurface(nativeWindow);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL Java_com_mrdesktop_MRDesktopClient_nativeCleanupSurface(
    JNIEnv* env, jobject thiz) {
    if (g_client) {
        g_client->Cleanup();
    }
}
}