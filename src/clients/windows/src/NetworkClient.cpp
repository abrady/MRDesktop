#include "NetworkClient.h"
#include <iostream>

NetworkClient::NetworkClient() 
    : m_isConnected(false)
    , m_shouldStop(false) {
    
    // Initialize Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        if (m_onError) {
            m_onError("Failed to initialize Winsock: " + std::to_string(result));
        }
    }
}

NetworkClient::~NetworkClient() {
    Disconnect();
    WSACleanup();
}

bool NetworkClient::Connect(const std::string& serverIP, int port) {
    if (m_isConnected) {
        Disconnect();
    }
    
    // Use FrameReceiver for connection
    if (!m_frameReceiver.Connect(serverIP.c_str(), port)) {
        if (m_onError) {
            m_onError("Failed to connect to server " + serverIP + ":" + std::to_string(port));
        }
        return false;
    }
    
    // Send compression negotiation message to server
    if (!m_frameReceiver.SendCompressionRequest(COMPRESSION_H264)) {
        if (m_onError) {
            m_onError("Failed to send compression negotiation message");
        }
        m_frameReceiver.Disconnect();
        return false;
    }
    
    std::cout << "Sent compression negotiation message (COMPRESSION_H264)" << std::endl;
    
    // Start receiving thread
    m_isConnected = true;
    m_shouldStop = false;
    m_receiveThread = std::thread(&NetworkClient::ReceiveThreadProc, this);
    
    return true;
}

void NetworkClient::Disconnect() {
    if (!m_isConnected) return;
    
    m_shouldStop = true;
    m_isConnected = false;
    
    m_frameReceiver.Disconnect();
    
    if (m_receiveThread.joinable()) {
        m_receiveThread.join();
    }
}

void NetworkClient::ReceiveThreadProc() {
    while (m_isConnected && !m_shouldStop) {
        FrameMessage frameMsg;
        std::vector<uint8_t> frameData;
        
        // Use FrameReceiver to get the frame
        if (!m_frameReceiver.ReceiveFrame(frameMsg, frameData)) {
            // No frame available yet (non-blocking), continue
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        
        // Debug: Print received message details
        std::cout << "Received message - Type: " << std::dec << frameMsg.header.type 
                  << ", Size: " << frameMsg.header.size 
                  << ", Width: " << frameMsg.width 
                  << ", Height: " << frameMsg.height << std::endl;
        
        // Handle different message types
        if (frameMsg.header.type == MSG_COMPRESSED_FRAME) {
            std::cout << "Received compressed frame (skipping - no decoder yet)" << std::endl;
            continue;
        } else if (frameMsg.header.type != MSG_FRAME_DATA || 
                   frameMsg.width > 10000 || frameMsg.height > 10000 ||
                   frameMsg.dataSize > 100000000) {
            std::cout << "Skipping corrupted frame (Type: " << frameMsg.header.type << ")" << std::endl;
            continue;
        }
        
        // Convert to Windows BYTE vector for callback compatibility
        std::vector<BYTE> winFrameData(frameData.begin(), frameData.end());
        
        // Call frame received callback
        if (m_onFrameReceived) {
            m_onFrameReceived(frameMsg, winFrameData);
        }
    }
    
    // Connection ended
    m_isConnected = false;
    if (m_onDisconnected) {
        m_onDisconnected();
    }
}


bool NetworkClient::SendMouseMove(int32_t deltaX, int32_t deltaY, bool absolute, int32_t x, int32_t y) {
    if (!m_isConnected) return false;
    return m_frameReceiver.SendMouseMove(deltaX, deltaY, absolute, x, y);
}

bool NetworkClient::SendMouseClick(MouseClickMessage::MouseButton button, bool pressed) {
    if (!m_isConnected) return false;
    return m_frameReceiver.SendMouseClick(button, pressed);
}

bool NetworkClient::SendMouseScroll(int32_t deltaX, int32_t deltaY) {
    if (!m_isConnected) return false;
    return m_frameReceiver.SendMouseScroll(deltaX, deltaY);
}