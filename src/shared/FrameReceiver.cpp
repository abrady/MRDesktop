#include "FrameReceiver.h"
#include <iostream>
#include <thread>
#include <chrono>

bool FrameReceiver::Connect(const char* serverIP, int port) {
#ifdef _WIN32
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#else
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
#endif
    if (m_socket == INVALID_SOCKET) {
#ifdef _WIN32
        std::cerr << "Failed to create socket: " << WSAGetLastError() << std::endl;
#else
        std::cerr << "Failed to create socket" << std::endl;
#endif
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(static_cast<u_short>(port));

    if (inet_pton(AF_INET, serverIP, &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid server IP address" << std::endl;
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
        std::cerr << "Failed to connect to server: " << WSAGetLastError() << std::endl;
#else
        std::cerr << "Failed to connect to server" << std::endl;
#endif
#ifdef _WIN32
        closesocket(m_socket);
#else
        close(m_socket);
#endif
        m_socket = INVALID_SOCKET;
        return false;
    }

#ifdef _WIN32
    u_long mode = 1;
    if (ioctlsocket(m_socket, FIONBIO, &mode) != 0) {
        std::cerr << "Failed to set socket non-blocking: " << WSAGetLastError() << std::endl;
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }
#else
    int flags = fcntl(m_socket, F_GETFL, 0);
    if (flags >= 0) fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);
#endif
    // std::cout << "Connected to server successfully!" << std::endl;
    return true;
}

bool FrameReceiver::ReceiveFrame(FrameMessage& frameMsg, std::vector<uint8_t>& frameData, int frameNumber) {
    if (m_socket == INVALID_SOCKET) return false;

    // First receive the basic message header to determine type
    MessageHeader msgHeader;
    int received = recv(m_socket, reinterpret_cast<char*>(&msgHeader), sizeof(MessageHeader), 0);

    if (received == SOCKET_ERROR) {
#ifdef _WIN32
        int error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK) {
            return false;
        }
#else
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return false;
        }
#endif
        return false;
    }

    if (received != sizeof(MessageHeader)) {
        if (received <= 0) {
            return false;
        }
        return false;
    }

    // Handle different message types
    if (msgHeader.type == MSG_FRAME_DATA) {
        // Receive rest of uncompressed frame message
        char* remainingData = reinterpret_cast<char*>(&frameMsg) + sizeof(MessageHeader);
        int remainingSize = sizeof(FrameMessage) - sizeof(MessageHeader);
        
        received = recv(m_socket, remainingData, remainingSize, 0);
        if (received != remainingSize) {
            return false;
        }
        
        // Copy header into frameMsg
        frameMsg.header = msgHeader;

        if (frameMsg.width == 0 || frameMsg.height == 0 ||
            frameMsg.width > 10000 || frameMsg.height > 10000 ||
            frameMsg.dataSize > 100000000) {
            return false;
        }

        if (m_frameBuffer.size() < frameMsg.dataSize) {
            m_frameBuffer.resize(frameMsg.dataSize);
        }

        uint32_t totalReceived = 0;
        while (totalReceived < frameMsg.dataSize) {
            received = recv(m_socket,
                            reinterpret_cast<char*>(m_frameBuffer.data()) + totalReceived,
                            frameMsg.dataSize - totalReceived, 0);
            if (received == SOCKET_ERROR) {
#ifdef _WIN32
                int error = WSAGetLastError();
                if (error == WSAEWOULDBLOCK) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
#else
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
#endif
                return false;
            }
            if (received == 0) {
                return false;
            }
            totalReceived += received;
        }
        
        // Copy uncompressed data to output
        frameData.resize(frameMsg.dataSize);
        memcpy(frameData.data(), m_frameBuffer.data(), frameMsg.dataSize);
        
    } else if (msgHeader.type == MSG_COMPRESSED_FRAME) {
        CompressedFrameMessage compressedMsg;
        compressedMsg.header = msgHeader;

        char* remainingData = reinterpret_cast<char*>(&compressedMsg) + sizeof(MessageHeader);
        int remainingSize = sizeof(CompressedFrameMessage) - sizeof(MessageHeader);

        received = recv(m_socket, remainingData, remainingSize, 0);
        if (received != remainingSize) {
            return false;
        }

        std::vector<uint8_t> compressedBuffer(compressedMsg.compressedSize);
        uint32_t totalReceived = 0;
        while (totalReceived < compressedMsg.compressedSize) {
            received = recv(m_socket,
                            reinterpret_cast<char*>(compressedBuffer.data()) + totalReceived,
                            compressedMsg.compressedSize - totalReceived, 0);
            if (received == SOCKET_ERROR) {
#ifdef _WIN32
                int error = WSAGetLastError();
                if (error == WSAEWOULDBLOCK) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
#else
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
#endif
                return false;
            }
            if (received <= 0) return false;
            totalReceived += received;
        }

        if (m_compression == COMPRESSION_H265) {
            if (!m_decoderInitialized) {
                m_decoderInitialized = m_h265Decoder.Initialize();
            }
            if (!m_decoderInitialized) return false;

            bool keyframe = false;
            int width = 0, height = 0;
            if (!m_h265Decoder.DecodeFrame(compressedBuffer.data(), compressedBuffer.size(), frameData, width, height, keyframe))
                return false;
            frameMsg.header.type = MSG_FRAME_DATA;
            frameMsg.header.size = sizeof(FrameMessage);
            frameMsg.width = static_cast<UINT32>(width);
            frameMsg.height = static_cast<UINT32>(height);
            frameMsg.dataSize = static_cast<UINT32>(frameData.size());
        } else {
            // Unsupported compression, skip
            return false;
        }
        
    } else {
        // Unknown message type
        return false;
    }

    return true;
}

void FrameReceiver::Disconnect() {
    if (m_socket != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(m_socket);
#else
        close(m_socket);
#endif
        m_socket = INVALID_SOCKET;
    }
}

FrameReceiver::~FrameReceiver() {
    Disconnect();
    m_h265Decoder.Cleanup();
}
