#include "frame_receiver.h"
#include <android/log.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#define LOG_TAG "FrameReceiver"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

FrameReceiver::FrameReceiver() : m_socket(-1) {
}

FrameReceiver::~FrameReceiver() {
    disconnect();
}

bool FrameReceiver::connect(const char* serverIP, int port) {
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket < 0) {
        LOGE("Failed to create socket");
        return false;
    }
    
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, serverIP, &serverAddr.sin_addr) <= 0) {
        LOGE("Invalid server IP address: %s", serverIP);
        close(m_socket);
        m_socket = -1;
        return false;
    }
    
    if (::connect(m_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        LOGE("Failed to connect to server %s:%d", serverIP, port);
        close(m_socket);
        m_socket = -1;
        return false;
    }
    
    LOGD("Connected to server %s:%d", serverIP, port);
    return true;
}

void FrameReceiver::disconnect() {
    if (m_socket >= 0) {
        close(m_socket);
        m_socket = -1;
        LOGD("Disconnected from server");
    }
}

bool FrameReceiver::receiveFrame(FrameMessage& frameMsg, std::vector<uint8_t>& frameData) {
    if (m_socket < 0) return false;
    
    // Receive frame message header
    ssize_t received = recv(m_socket, &frameMsg, sizeof(FrameMessage), 0);
    if (received != sizeof(FrameMessage)) {
        if (received == 0) {
            LOGD("Server disconnected");
        } else if (received > 0) {
            LOGE("Partial frame header received: %zd", received);
        } else {
            LOGE("Failed to receive frame header: %s", strerror(errno));
        }
        return false;
    }
    
    // Verify this is a frame message
    if (frameMsg.header.type != MSG_FRAME_DATA) {
        LOGE("Unexpected message type: %u", frameMsg.header.type);
        return false;
    }
    
    // Resize buffer if needed
    if (m_frameBuffer.size() < frameMsg.dataSize) {
        m_frameBuffer.resize(frameMsg.dataSize);
    }
    
    // Receive frame pixel data
    uint32_t totalReceived = 0;
    while (totalReceived < frameMsg.dataSize) {
        received = recv(m_socket, m_frameBuffer.data() + totalReceived, 
                       frameMsg.dataSize - totalReceived, 0);
        if (received <= 0) {
            LOGE("Failed to receive frame data: %s", strerror(errno));
            return false;
        }
        totalReceived += received;
    }
    
    // Copy frame data
    frameData.clear();
    frameData.resize(frameMsg.dataSize);
    memcpy(frameData.data(), m_frameBuffer.data(), frameMsg.dataSize);
    
    return true;
}

int FrameReceiver::getSocket() const {
    return m_socket;
}