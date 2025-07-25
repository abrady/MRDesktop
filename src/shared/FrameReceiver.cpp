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
    // std::cout << "CLIENT RECV: Frame " << frameNumber << " - Expecting header size: " << sizeof(FrameMessage) << std::endl;

    // Receive frame message header
    int received = recv(m_socket, reinterpret_cast<char*>(&frameMsg), sizeof(FrameMessage), 0);
    // std::cout << "CLIENT RECV: Frame " << frameNumber << " - Header received: " << received << " bytes" << std::endl;

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

    if (received != sizeof(FrameMessage)) {
        if (received <= 0) {
            return false;
        }
        return false;
    }

    if (frameMsg.header.type != MSG_FRAME_DATA) {
        return false;
    }

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

    // std::cout << "CLIENT RECV: Frame " << frameNumber << " - Pixel data received: " << totalReceived << " bytes" << std::endl;

    frameData.assign(m_frameBuffer.begin(), m_frameBuffer.begin() + frameMsg.dataSize);
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
}
