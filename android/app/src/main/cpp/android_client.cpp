#include "NetworkReceiver.h"
#include "protocol.h"
#include <jni.h>
#include <android/log.h>
#include <thread>
#include <chrono>
#include <string>
#include <atomic>

#define LOG_TAG "MRDesktopClient"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class AndroidNetworkClient {
private:
    NetworkReceiver receiver;
    std::atomic<bool> running{false};
    std::thread networkThread;
    
public:
    bool Connect(const std::string& serverIP, int port) {
        LOGI("Attempting to connect to %s:%d", serverIP.c_str(), port);
        
        receiver.SetFrameCallback([](const FrameMessage& msg, const std::vector<uint8_t>& data) {
            LOGI("Received frame: %dx%d, size: %zu bytes", msg.width, msg.height, data.size());
        });
        
        receiver.SetErrorCallback([](const std::string& error) {
            LOGE("Network error: %s", error.c_str());
        });
        
        receiver.SetDisconnectedCallback([]() {
            LOGI("Disconnected from server");
        });
        
        bool connected = receiver.Connect(serverIP, port);
        if (connected) {
            LOGI("Successfully connected to server");
            running = true;
            
            networkThread = std::thread([this]() {
                while (running && receiver.IsConnected()) {
                    if (!receiver.PollFrame()) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
                    }
                }
                LOGI("Network thread stopped");
            });
        } else {
            LOGE("Failed to connect to server");
        }
        
        return connected;
    }
    
    void Disconnect() {
        LOGI("Disconnecting from server");
        running = false;
        
        receiver.Disconnect();
        
        if (networkThread.joinable()) {
            networkThread.join();
        }
    }
    
    bool SendMouseMove(int32_t deltaX, int32_t deltaY) {
        return receiver.SendMouseMove(deltaX, deltaY);
    }
    
    bool SendMouseClick(int button, bool pressed) {
        MouseClickMessage::MouseButton btn;
        switch(button) {
            case 0: btn = MouseClickMessage::LEFT_BUTTON; break;
            case 1: btn = MouseClickMessage::RIGHT_BUTTON; break;
            case 2: btn = MouseClickMessage::MIDDLE_BUTTON; break;
            default: return false;
        }
        return receiver.SendMouseClick(btn, pressed);
    }
    
    bool IsConnected() const {
        return receiver.IsConnected();
    }
};

static AndroidNetworkClient* g_client = nullptr;

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_mrdesktop_MRDesktopClient_nativeConnect(JNIEnv *env, jobject thiz, jstring serverIP, jint port) {
    if (!g_client) {
        g_client = new AndroidNetworkClient();
    }
    
    const char* ip = env->GetStringUTFChars(serverIP, nullptr);
    bool result = g_client->Connect(std::string(ip), port);
    env->ReleaseStringUTFChars(serverIP, ip);
    
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_mrdesktop_MRDesktopClient_nativeDisconnect(JNIEnv *env, jobject thiz) {
    if (g_client) {
        g_client->Disconnect();
        delete g_client;
        g_client = nullptr;
    }
}

JNIEXPORT jboolean JNICALL
Java_com_mrdesktop_MRDesktopClient_nativeIsConnected(JNIEnv *env, jobject thiz) {
    if (!g_client) {
        return JNI_FALSE;
    }
    return g_client->IsConnected() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_mrdesktop_MRDesktopClient_nativeSendMouseMove(JNIEnv *env, jobject thiz, jint deltaX, jint deltaY) {
    if (!g_client) {
        return JNI_FALSE;
    }
    return g_client->SendMouseMove(deltaX, deltaY) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_mrdesktop_MRDesktopClient_nativeSendMouseClick(JNIEnv *env, jobject thiz, jint button, jboolean pressed) {
    if (!g_client) {
        return JNI_FALSE;
    }
    return g_client->SendMouseClick(button, pressed == JNI_TRUE) ? JNI_TRUE : JNI_FALSE;
}

}