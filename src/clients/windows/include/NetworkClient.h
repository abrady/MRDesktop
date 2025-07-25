#pragma once
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include <string>
#include "protocol.h"

#pragma comment(lib, "ws2_32.lib")

class NetworkClient {
private:
    SOCKET m_socket;
    std::thread m_receiveThread;
    std::atomic<bool> m_isConnected;
    std::atomic<bool> m_shouldStop;
    std::vector<BYTE> m_receiveBuffer;
    
    // Callbacks
    std::function<void(const FrameMessage&, const std::vector<BYTE>&)> m_onFrameReceived;
    std::function<void(const std::string&)> m_onError;
    std::function<void()> m_onDisconnected;
    
    void ReceiveThreadProc();
    bool ReceiveData(void* buffer, size_t size);
    
public:
    NetworkClient();
    ~NetworkClient();
    
    bool Connect(const std::string& serverIP, int port);
    void Disconnect();
    bool IsConnected() const { return m_isConnected; }
    
    // Send input messages to server
    bool SendMouseMove(int32_t deltaX, int32_t deltaY, bool absolute = false, int32_t x = 0, int32_t y = 0);
    bool SendMouseClick(MouseClickMessage::MouseButton button, bool pressed);
    bool SendMouseScroll(int32_t deltaX, int32_t deltaY);
    
    // Set callbacks
    void SetFrameCallback(std::function<void(const FrameMessage&, const std::vector<BYTE>&)> callback) {
        m_onFrameReceived = callback;
    }
    
    void SetErrorCallback(std::function<void(const std::string&)> callback) {
        m_onError = callback;
    }
    
    void SetDisconnectedCallback(std::function<void()> callback) {
        m_onDisconnected = callback;
    }
};