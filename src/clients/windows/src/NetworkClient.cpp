#include "NetworkClient.h"
#include <iostream>

NetworkClient::NetworkClient() 
    : m_socket(INVALID_SOCKET)
    , m_isConnected(false)
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
    
    // Create socket
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) {
        if (m_onError) {
            m_onError("Failed to create socket: " + std::to_string(WSAGetLastError()));
        }
        return false;
    }
    
    // Setup server address
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(static_cast<u_short>(port));
    
    if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0) {
        if (m_onError) {
            m_onError("Invalid server IP address: " + serverIP);
        }
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }
    
    // Connect to server
    if (connect(m_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        if (m_onError) {
            m_onError("Failed to connect to server " + serverIP + ":" + std::to_string(port) + 
                     " Error: " + std::to_string(WSAGetLastError()));
        }
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }
    
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
    
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
    
    if (m_receiveThread.joinable()) {
        m_receiveThread.join();
    }
}

void NetworkClient::ReceiveThreadProc() {
    while (m_isConnected && !m_shouldStop) {
        // Receive message header
        FrameMessage frameMsg;
        if (!ReceiveData(&frameMsg, sizeof(FrameMessage))) {
            break;
        }
        
        // Debug: Print received message details
        std::cout << "Received message - Type: " << std::dec << frameMsg.header.type 
                  << ", Size: " << frameMsg.header.size 
                  << ", Width: " << frameMsg.width 
                  << ", Height: " << frameMsg.height << std::endl;
        
        // Handle different message types
        if (frameMsg.header.type == MSG_COMPRESSED_FRAME) {
            std::cout << "Received compressed frame (skipping - no decoder yet)" << std::endl;
            // For now, skip compressed frames - the server will fall back to uncompressed if encoder fails
            continue;
        } else if (frameMsg.header.type != MSG_FRAME_DATA || 
                   frameMsg.width > 10000 || frameMsg.height > 10000 ||
                   frameMsg.dataSize > 100000000) {  // Skip obviously corrupted frames
            
            std::cout << "Skipping corrupted frame (Type: " << frameMsg.header.type << ")" << std::endl;
            continue;
        }
        
        // Resize receive buffer if needed
        if (m_receiveBuffer.size() < frameMsg.dataSize) {
            m_receiveBuffer.resize(frameMsg.dataSize);
        }
        
        // Receive frame pixel data
        if (!ReceiveData(m_receiveBuffer.data(), frameMsg.dataSize)) {
            break;
        }
        
        // Create a copy of the frame data for the callback
        std::vector<BYTE> frameData(m_receiveBuffer.begin(), m_receiveBuffer.begin() + frameMsg.dataSize);
        
        // Call frame received callback
        if (m_onFrameReceived) {
            m_onFrameReceived(frameMsg, frameData);
        }
    }
    
    // Connection ended
    m_isConnected = false;
    if (m_onDisconnected) {
        m_onDisconnected();
    }
}

bool NetworkClient::ReceiveData(void* buffer, size_t size) {
    char* byteBuffer = static_cast<char*>(buffer);
    size_t totalReceived = 0;
    
    while (totalReceived < size && !m_shouldStop) {
        int received = recv(m_socket, byteBuffer + totalReceived, 
                           static_cast<int>(size - totalReceived), 0);
        
        if (received <= 0) {
            if (received == 0) {
                // Server closed connection
                if (m_onError) {
                    m_onError("Server closed connection");
                }
            } else {
                // Socket error
                int error = WSAGetLastError();
                if (error != WSAECONNABORTED && error != WSAEINTR) {
                    if (m_onError) {
                        m_onError("Socket receive error: " + std::to_string(error));
                    }
                }
            }
            return false;
        }
        
        totalReceived += received;
    }
    
    return totalReceived == size;
}

bool NetworkClient::SendMouseMove(int32_t deltaX, int32_t deltaY, bool absolute, int32_t x, int32_t y) {
    if (!m_isConnected) return false;
    
    MouseMoveMessage msg;
    msg.header.type = MSG_MOUSE_MOVE;
    msg.header.size = sizeof(MouseMoveMessage);
    msg.deltaX = deltaX;
    msg.deltaY = deltaY;
    msg.absolute = absolute;
    msg.x = x;
    msg.y = y;
    
    int sent = send(m_socket, reinterpret_cast<const char*>(&msg), sizeof(msg), 0);
    return sent == sizeof(msg);
}

bool NetworkClient::SendMouseClick(MouseClickMessage::MouseButton button, bool pressed) {
    if (!m_isConnected) return false;
    
    MouseClickMessage msg;
    msg.header.type = MSG_MOUSE_CLICK;
    msg.header.size = sizeof(MouseClickMessage);
    msg.button = button;
    msg.pressed = pressed;
    
    int sent = send(m_socket, reinterpret_cast<const char*>(&msg), sizeof(msg), 0);
    return sent == sizeof(msg);
}

bool NetworkClient::SendMouseScroll(int32_t deltaX, int32_t deltaY) {
    if (!m_isConnected) return false;
    
    MouseScrollMessage msg;
    msg.header.type = MSG_MOUSE_SCROLL;
    msg.header.size = sizeof(MouseScrollMessage);
    msg.deltaX = deltaX;
    msg.deltaY = deltaY;
    
    int sent = send(m_socket, reinterpret_cast<const char*>(&msg), sizeof(msg), 0);
    return sent == sizeof(msg);
}