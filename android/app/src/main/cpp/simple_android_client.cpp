#include <jni.h>
#include <android/log.h>
#include <thread>
#include <chrono>
#include <string>
#include <atomic>
#include "../../../../../../src/shared/AsioNetworking.h"

#define LOG_TAG "MRDesk.Simple"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class SimpleAndroidClient {
private:
    AsioConnection connection;
    std::atomic<bool> connected{false};
    
public:
    bool Connect(const std::string& serverIP, int port) {
        LOGI("Attempting to connect to %s:%d", serverIP.c_str(), port);
        
        connection.SetErrorCallback([this](const std::string& error) {
            LOGE("Connection error: %s", error.c_str());
            connected = false;
        });
        
        connection.SetDisconnectCallback([this]() {
            LOGI("Disconnected from server");
            connected = false;
        });
        
        bool result = connection.Connect(serverIP, port);
        if (result) {
            connected = true;
            LOGI("Successfully connected to server");
        } else {
            LOGE("Failed to connect to server");
        }
        
        return result;
    }
    
    void Disconnect() {
        connection.Disconnect();
        connected = false;
        LOGI("Disconnected from server");
    }
    
    bool IsConnected() const {
        return connected && connection.IsConnected();
    }
    
    bool SendTestMessage(const std::string& message) {
        if (!IsConnected()) {
            return false;
        }
        
        // For test purposes, just send the message as raw data
        bool sent = connection.SendData(message.c_str(), message.length());
        if (sent) {
            LOGI("Sent message: %s", message.c_str());
        } else {
            LOGE("Failed to send message");
        }
        
        return sent;
    }
};

static SimpleAndroidClient* g_client = nullptr;

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_mrdesktop_SimpleClient_nativeConnect(JNIEnv *env, jobject thiz, jstring serverIP, jint port) {
    if (!g_client) {
        g_client = new SimpleAndroidClient();
    }
    
    const char* ip = env->GetStringUTFChars(serverIP, nullptr);
    bool result = g_client->Connect(std::string(ip), port);
    env->ReleaseStringUTFChars(serverIP, ip);
    
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_mrdesktop_SimpleClient_nativeDisconnect(JNIEnv *env, jobject thiz) {
    if (g_client) {
        g_client->Disconnect();
        delete g_client;
        g_client = nullptr;
    }
}

JNIEXPORT jboolean JNICALL
Java_com_mrdesktop_SimpleClient_nativeIsConnected(JNIEnv *env, jobject thiz) {
    if (!g_client) {
        return JNI_FALSE;
    }
    return g_client->IsConnected() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_mrdesktop_SimpleClient_nativeSendTestMessage(JNIEnv *env, jobject thiz, jstring message) {
    if (!g_client) {
        return JNI_FALSE;
    }
    
    const char* msg = env->GetStringUTFChars(message, nullptr);
    bool result = g_client->SendTestMessage(std::string(msg));
    env->ReleaseStringUTFChars(message, msg);
    
    return result ? JNI_TRUE : JNI_FALSE;
}

}