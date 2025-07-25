#include "FrameReceiver.h"
#include "FrameUtils.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cerrno>

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

bool FrameReceiver::ReceiveFrame(FrameMessage& frameMsg, std::vector<uint8_t>& frameData, int /*frameNumber*/) {
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
