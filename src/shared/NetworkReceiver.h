#pragma once

#include "protocol.h"
#include <vector>
#include <chrono>
#include <functional>
#include <string>
#include <atomic>
#include <memory>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
using SocketType = SOCKET;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <errno.h>
using SocketType = int;
#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif
#endif

class VideoDecoder;

class NetworkReceiver {
private:
    SocketType m_socket = INVALID_SOCKET;
    std::vector<uint8_t> m_frameBuffer;
    CompressionType m_compression = COMPRESSION_NONE;
    std::atomic<bool> m_isConnected{false};
    
    // Video decoder for compressed frames
    std::unique_ptr<VideoDecoder> m_decoder;
    
    // Callbacks
    std::function<void(const FrameMessage&, const std::vector<uint8_t>&)> m_onFrameReceived;
    std::function<void(const std::string&)> m_onError;
    std::function<void()> m_onDisconnected;
    
#ifdef _WIN32
    bool m_winsockInitialized = false;
#endif

public:
    NetworkReceiver();
    ~NetworkReceiver();
    
    // Connection management
    bool Connect(const std::string& serverIP, int port);
    void Disconnect();
    bool IsConnected() const { return m_isConnected; }
    
    // Frame receiving (polling-based)
    bool PollFrame(); // Returns true if frame was received and processed
    void SetCompression(CompressionType compression) { m_compression = compression; }
    
    // Input message sending methods
    bool SendCompressionRequest(CompressionType compression);
    bool SendMouseMove(int32_t deltaX, int32_t deltaY, bool absolute = false, int32_t x = 0, int32_t y = 0);
    bool SendMouseClick(MouseClickMessage::MouseButton button, bool pressed);
    bool SendMouseScroll(int32_t deltaX, int32_t deltaY);
    
    // Callback setters
    void SetFrameCallback(std::function<void(const FrameMessage&, const std::vector<uint8_t>&)> callback) {
        m_onFrameReceived = callback;
    }
    
    void SetErrorCallback(std::function<void(const std::string&)> callback) {
        m_onError = callback;
    }
    
    void SetDisconnectedCallback(std::function<void()> callback) {
        m_onDisconnected = callback;
    }

private:
    bool ReceiveFrame(FrameMessage& frameMsg, std::vector<uint8_t>& frameData, int frameNumber = 0);
};
