#include <jni.h>
#include <android/log.h>
#include <thread>
#include <chrono>
#include <string>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define LOG_TAG "MRDesktopClient"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class SimpleAndroidClient {
private:
    int socket_fd = -1;
    std::atomic<bool> connected{false};
    
public:
    bool Connect(const std::string& serverIP, int port) {
        LOGI("Attempting to connect to %s:%d", serverIP.c_str(), port);
        
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd < 0) {
            LOGE("Failed to create socket");
            return false;
        }
        
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        
        if (inet_pton(AF_INET, serverIP.c_str(), &server_addr.sin_addr) <= 0) {
            LOGE("Invalid IP address: %s", serverIP.c_str());
            close(socket_fd);
            socket_fd = -1;
            return false;
        }
        
        if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            LOGE("Failed to connect to server");
            close(socket_fd);
            socket_fd = -1;
            return false;
        }
        
        connected = true;
        LOGI("Successfully connected to server");
        return true;
    }
    
    void Disconnect() {
        if (socket_fd >= 0) {
            close(socket_fd);
            socket_fd = -1;
        }
        connected = false;
        LOGI("Disconnected from server");
    }
    
    bool IsConnected() const {
        return connected;
    }
    
    bool SendTestMessage(const std::string& message) {
        if (!connected || socket_fd < 0) {
            return false;
        }
        
        ssize_t sent = send(socket_fd, message.c_str(), message.length(), 0);
        if (sent < 0) {
            LOGE("Failed to send message");
            return false;
        }
        
        LOGI("Sent message: %s", message.c_str());
        return true;
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