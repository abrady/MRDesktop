#include <jni.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "FrameReceiver.h"
#include "input_sender.h"
#include <thread>
#include <atomic>
#include <memory>

#define LOG_TAG "NativeClient"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static std::unique_ptr<FrameReceiver> g_frameReceiver;
static std::unique_ptr<InputSender> g_inputSender;
static std::atomic<bool> g_isReceiving(false);
static std::thread g_receiveThread;
static ANativeWindow* g_nativeWindow = nullptr;
static JavaVM* g_jvm = nullptr;
static jobject g_mainActivity = nullptr;

// Forward declarations
void updateFrameInfo(JNIEnv* env, int frameCount, float fps, int width, int height);
void renderFrame(const FrameMessage& frameMsg, const std::vector<uint8_t>& frameData);

// Frame receiving thread function
void receiveFrameLoop() {
    JNIEnv* env = nullptr;
    if (g_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
        LOGE("Failed to attach thread to JVM");
        return;
    }
    
    FrameMessage frameMsg;
    std::vector<uint8_t> frameData;
    int frameCount = 0;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    LOGD("Frame receive loop started");
    
    while (g_isReceiving && g_frameReceiver) {
        if (g_frameReceiver->receiveFrame(frameMsg, frameData)) {
            frameCount++;
            
            // Render frame to surface
            if (g_nativeWindow) {
                renderFrame(frameMsg, frameData);
            }
            
            // Update UI every 30 frames
            if (frameCount % 30 == 0) {
                auto currentTime = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime);
                float fps = (frameCount * 1000.0f) / duration.count();
                
                updateFrameInfo(env, frameCount, fps, frameMsg.width, frameMsg.height);
            }
        } else {
            LOGE("Failed to receive frame, stopping loop");
            g_isReceiving = false;
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    LOGD("Frame receive loop ended, total frames: %d", frameCount);
    g_jvm->DetachCurrentThread();
}

void updateFrameInfo(JNIEnv* env, int frameCount, float fps, int width, int height) {
    if (!g_mainActivity) return;
    
    jclass activityClass = env->GetObjectClass(g_mainActivity);
    jmethodID methodID = env->GetMethodID(activityClass, "updateFrameInfo", "(IFII)V");
    
    if (methodID) {
        env->CallVoidMethod(g_mainActivity, methodID, frameCount, fps, width, height);
    }
    
    env->DeleteLocalRef(activityClass);
}

void renderFrame(const FrameMessage& frameMsg, const std::vector<uint8_t>& frameData) {
    if (!g_nativeWindow) return;
    
    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(g_nativeWindow, &buffer, nullptr) == 0) {
        // Simple BGRA to RGB conversion and scaling
        // This is a basic implementation - in production you'd want proper scaling and color conversion
        
        int srcWidth = frameMsg.width;
        int srcHeight = frameMsg.height;
        int dstWidth = buffer.width;
        int dstHeight = buffer.height;
        
        // Calculate scaling factors
        float scaleX = (float)srcWidth / dstWidth;
        float scaleY = (float)srcHeight / dstHeight;
        
        uint32_t* dst = (uint32_t*)buffer.bits;
        const uint32_t* src = (const uint32_t*)frameData.data();
        
        // Simple nearest neighbor scaling
        for (int y = 0; y < dstHeight; y++) {
            int srcY = (int)(y * scaleY);
            if (srcY >= srcHeight) srcY = srcHeight - 1;
            
            for (int x = 0; x < dstWidth; x++) {
                int srcX = (int)(x * scaleX);
                if (srcX >= srcWidth) srcX = srcWidth - 1;
                
                // Copy pixel (BGRA format)
                dst[y * dstWidth + x] = src[srcY * srcWidth + srcX];
            }
        }
        
        ANativeWindow_unlockAndPost(g_nativeWindow);
    }
}

extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_jvm = vm;
    return JNI_VERSION_1_6;
}

JNIEXPORT jboolean JNICALL
Java_com_mrdesktop_client_MainActivity_nativeConnect(JNIEnv* env, jobject thiz, jstring serverIp, jint port) {
    const char* serverIpStr = env->GetStringUTFChars(serverIp, nullptr);
    
    LOGD("Connecting to %s:%d", serverIpStr, port);
    
    // Store reference to main activity
    if (g_mainActivity) {
        env->DeleteGlobalRef(g_mainActivity);
    }
    g_mainActivity = env->NewGlobalRef(thiz);
    
    // Create frame receiver
    g_frameReceiver = std::make_unique<FrameReceiver>();
    
    bool connected = g_frameReceiver->connect(serverIpStr, port);
    env->ReleaseStringUTFChars(serverIp, serverIpStr);
    
    if (connected) {
        // Create input sender
        g_inputSender = std::make_unique<InputSender>(g_frameReceiver->getSocket());
        
        // Start receiving thread
        g_isReceiving = true;
        g_receiveThread = std::thread(receiveFrameLoop);
        
        LOGD("Connected successfully");
        return JNI_TRUE;
    } else {
        LOGE("Connection failed");
        g_frameReceiver.reset();
        return JNI_FALSE;
    }
}

JNIEXPORT void JNICALL
Java_com_mrdesktop_client_MainActivity_nativeDisconnect(JNIEnv* env, jobject thiz) {
    LOGD("Disconnecting...");
    
    // Stop receiving thread
    g_isReceiving = false;
    if (g_receiveThread.joinable()) {
        g_receiveThread.join();
    }
    
    // Clean up
    g_inputSender.reset();
    g_frameReceiver.reset();
    
    if (g_nativeWindow) {
        ANativeWindow_release(g_nativeWindow);
        g_nativeWindow = nullptr;
    }
    
    if (g_mainActivity) {
        env->DeleteGlobalRef(g_mainActivity);
        g_mainActivity = nullptr;
    }
    
    LOGD("Disconnected");
}

JNIEXPORT void JNICALL
Java_com_mrdesktop_client_MainActivity_nativeSetSurface(JNIEnv* env, jobject thiz, jobject surface) {
    if (g_nativeWindow) {
        ANativeWindow_release(g_nativeWindow);
        g_nativeWindow = nullptr;
    }
    
    if (surface) {
        g_nativeWindow = ANativeWindow_fromSurface(env, surface);
        if (g_nativeWindow) {
            // Set buffer format to RGBA_8888
            ANativeWindow_setBuffersGeometry(g_nativeWindow, 0, 0, WINDOW_FORMAT_RGBA_8888);
            LOGD("Native window set");
        } else {
            LOGE("Failed to create native window");
        }
    }
}

JNIEXPORT void JNICALL
Java_com_mrdesktop_client_MainActivity_nativeSendMouseMove(JNIEnv* env, jobject thiz, jint deltaX, jint deltaY) {
    if (g_inputSender) {
        g_inputSender->sendMouseMove(deltaX, deltaY);
    }
}

JNIEXPORT void JNICALL
Java_com_mrdesktop_client_MainActivity_nativeSendMouseClick(JNIEnv* env, jobject thiz, jint button, jboolean pressed) {
    if (g_inputSender) {
        MouseClickMessage::MouseButton btn = (MouseClickMessage::MouseButton)button;
        g_inputSender->sendMouseClick(btn, pressed);
    }
}

JNIEXPORT void JNICALL
Java_com_mrdesktop_client_MainActivity_nativeSendMouseScroll(JNIEnv* env, jobject thiz, jint deltaX, jint deltaY) {
    if (g_inputSender) {
        g_inputSender->sendMouseScroll(deltaX, deltaY);
    }
}

}