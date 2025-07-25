#include "NetworkReceiver.h"
#include "FrameUtils.h"
#include "VideoDecoder.h"
#include <iostream>
#include <chrono>
#include <cerrno>

NetworkReceiver::NetworkReceiver() : m_isConnected(false) {
#ifdef _WIN32
    // Initialize Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        if (m_onError) {
            m_onError("Failed to initialize Winsock: " + std::to_string(result));
        }
        m_winsockInitialized = false;
    } else {
        m_winsockInitialized = true;
    }
#endif
}

NetworkReceiver::~NetworkReceiver() {
    Disconnect();
#ifdef _WIN32
    if (m_winsockInitialized) {
        WSACleanup();
    }
#endif
}

bool NetworkReceiver::Connect(const std::string& serverIP, int port) {
    if (m_isConnected) {
        Disconnect();
    }

#ifdef _WIN32
    if (!m_winsockInitialized) {
        if (m_onError) {
            m_onError("Winsock not initialized");
        }
        return false;
    }
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#else
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
#endif
    if (m_socket == INVALID_SOCKET) {
#ifdef _WIN32
        if (m_onError) {
            m_onError("Failed to create socket: " + std::to_string(WSAGetLastError()));
        }
#else
        if (m_onError) {
            m_onError("Failed to create socket");
        }
#endif
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(static_cast<u_short>(port));

    if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0) {
        if (m_onError) {
            m_onError("Invalid server IP address: " + serverIP);
        }
#ifdef _WIN32
        closesocket(m_socket);
#else
        close(m_socket);
#endif
        m_socket = INVALID_SOCKET;
        return false;
    }

    if (connect(m_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
#ifdef _WIN32
        if (m_onError) {
            m_onError("Failed to connect to server " + serverIP + ":" + std::to_string(port) + 
                     " Error: " + std::to_string(WSAGetLastError()));
        }
        closesocket(m_socket);
#else
        if (m_onError) {
            m_onError("Failed to connect to server " + serverIP + ":" + std::to_string(port));
        }
        close(m_socket);
#endif
        m_socket = INVALID_SOCKET;
        return false;
    }

    // Set socket to non-blocking for polling
#ifdef _WIN32
    u_long mode = 1;
    if (ioctlsocket(m_socket, FIONBIO, &mode) != 0) {
        if (m_onError) {
            m_onError("Failed to set socket non-blocking: " + std::to_string(WSAGetLastError()));
        }
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }
#else
    int flags = fcntl(m_socket, F_GETFL, 0);
    if (flags >= 0) fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);
#endif

    // Send compression negotiation message to server
    if (!SendCompressionRequest(m_compression)) {
        if (m_onError) {
            m_onError("Failed to send compression negotiation message");
        }
        Disconnect();
        return false;
    }

    m_isConnected = true;
    return true;
}

void NetworkReceiver::Disconnect() {
    m_isConnected = false;
    
    if (m_socket != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(m_socket);
#else
        close(m_socket);
#endif
        m_socket = INVALID_SOCKET;
    }
    
    if (m_onDisconnected) {
        m_onDisconnected();
    }
}

bool NetworkReceiver::PollFrame() {
    if (!m_isConnected || m_socket == INVALID_SOCKET) {
        return false;
    }

    FrameMessage frameMsg;
    std::vector<uint8_t> frameData;
    
    // Try to receive a frame (non-blocking)
    if (!ReceiveFrame(frameMsg, frameData)) {
        return false; // No frame available or connection error
    }
    
    // Debug: Print received message details
    std::cout << "Received message - Type: " << std::dec << frameMsg.header.type 
              << ", Size: " << frameMsg.header.size 
              << ", Width: " << frameMsg.width 
              << ", Height: " << frameMsg.height << std::endl;
    
    // Notify about raw frame type received
    if (m_onRawFrameReceived) {
        m_onRawFrameReceived(frameMsg.header.type);
    }
    
    // Handle different message types
    if (frameMsg.header.type == MSG_COMPRESSED_FRAME) {
        // Cast to CompressedFrameMessage to get additional fields
        CompressedFrameMessage* compFrame = reinterpret_cast<CompressedFrameMessage*>(&frameMsg);
        
        std::cout << "Received compressed frame: " << compFrame->compressedSize << " bytes (" 
                  << (compFrame->isKeyframe ? "KEY" : "DELTA") << ")" << std::endl;
        
        // Initialize decoder if needed
        if (!m_decoder && m_compression != COMPRESSION_NONE) {
            m_decoder = std::make_unique<VideoDecoder>();
            if (!m_decoder->Initialize(compFrame->width, compFrame->height, m_compression)) {
                std::cerr << "Failed to initialize video decoder" << std::endl;
                m_decoder.reset();
                return true; // Skip this frame
            } else {
                std::cout << "Video decoder initialized successfully" << std::endl;
            }
        }
        
        // Decode frame if decoder is available
        if (m_decoder) {
            std::vector<uint8_t> decodedFrame;
            if (m_decoder->DecodeFrame(frameData.data(), frameData.size(), decodedFrame)) {
                // Create a FrameMessage for the decoded frame
                FrameMessage decodedFrameMsg;
                decodedFrameMsg.header.type = MSG_FRAME_DATA;
                decodedFrameMsg.header.size = sizeof(FrameMessage);
                decodedFrameMsg.width = compFrame->width;
                decodedFrameMsg.height = compFrame->height;
                decodedFrameMsg.dataSize = static_cast<uint32_t>(decodedFrame.size());
                
                // Call frame received callback with decoded frame
                if (m_onFrameReceived) {
                    m_onFrameReceived(decodedFrameMsg, decodedFrame);
                }
            } else {
                std::cout << "Failed to decode compressed frame" << std::endl;
            }
        }
        
        return true; // Frame received and processed
    } else if (frameMsg.header.type != MSG_FRAME_DATA || 
               frameMsg.width > 10000 || frameMsg.height > 10000 ||
               frameMsg.dataSize > 100000000) {
        std::cout << "Skipping corrupted frame (Type: " << frameMsg.header.type << ")" << std::endl;
        return true; // Frame received but not processed
    }
    
    // Call frame received callback
    if (m_onFrameReceived) {
        m_onFrameReceived(frameMsg, frameData);
    }
    
    return true; // Frame successfully received and processed
}

bool NetworkReceiver::ReceiveFrame(FrameMessage& frameMsg, std::vector<uint8_t>& frameData, int /*frameNumber*/) {
    if (m_socket == INVALID_SOCKET) return false;

    auto recvWrapper = [this](uint8_t* buf, int len) -> int {
        int r = recv(m_socket, reinterpret_cast<char*>(buf), len, 0);
        if (r == SOCKET_ERROR) {
#ifdef _WIN32
            if (WSAGetLastError() == WSAEWOULDBLOCK)
                return 0;
#else
            if (errno == EWOULDBLOCK || errno == EAGAIN)
                return 0;
#endif
            return -1;
        }
        return r;
    };

    std::vector<uint8_t> temp;
    bool ok = ReadFrameGeneric(recvWrapper, frameMsg, temp);
    if (ok) {
        frameData.swap(temp);
    }
    return ok;
}

bool NetworkReceiver::SendCompressionRequest(CompressionType compression) {
    if (m_socket == INVALID_SOCKET) return false;
    
    CompressionRequestMessage msg;
    msg.header.type = MSG_COMPRESSION_REQUEST;
    msg.header.size = sizeof(CompressionRequestMessage);
    msg.compression = compression;
    
    int sent = send(m_socket, reinterpret_cast<const char*>(&msg), sizeof(msg), 0);
    return sent == sizeof(msg);
}

bool NetworkReceiver::SendMouseMove(int32_t deltaX, int32_t deltaY, bool absolute, int32_t x, int32_t y) {
    if (m_socket == INVALID_SOCKET) return false;
    
    MouseMoveMessage msg;
    msg.header.type = MSG_MOUSE_MOVE;
    msg.header.size = sizeof(MouseMoveMessage);
    msg.deltaX = deltaX;
    msg.deltaY = deltaY;
    msg.absolute = absolute ? 1 : 0;
    msg.x = x;
    msg.y = y;
    
    int sent = send(m_socket, reinterpret_cast<const char*>(&msg), sizeof(msg), 0);
    return sent == sizeof(msg);
}

bool NetworkReceiver::SendMouseClick(MouseClickMessage::MouseButton button, bool pressed) {
    if (m_socket == INVALID_SOCKET) return false;
    
    MouseClickMessage msg;
    msg.header.type = MSG_MOUSE_CLICK;
    msg.header.size = sizeof(MouseClickMessage);
    msg.button = button;
    msg.pressed = pressed ? 1 : 0;
    
    int sent = send(m_socket, reinterpret_cast<const char*>(&msg), sizeof(msg), 0);
    return sent == sizeof(msg);
}

bool NetworkReceiver::SendMouseScroll(int32_t deltaX, int32_t deltaY) {
    if (m_socket == INVALID_SOCKET) return false;
    
    MouseScrollMessage msg;
    msg.header.type = MSG_MOUSE_SCROLL;
    msg.header.size = sizeof(MouseScrollMessage);
    msg.deltaX = deltaX;
    msg.deltaY = deltaY;
    
    int sent = send(m_socket, reinterpret_cast<const char*>(&msg), sizeof(msg), 0);
    return sent == sizeof(msg);
}