#pragma once

#include "protocol.h"
#include <vector>
#include <chrono>
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

class FrameReceiver {
private:
    SocketType m_socket = INVALID_SOCKET;
    std::vector<uint8_t> m_frameBuffer;
    CompressionType m_compression = COMPRESSION_NONE;

public:
    bool Connect(const char* serverIP, int port);
    void SetCompression(CompressionType compression) { m_compression = compression; }
    bool ReceiveFrame(FrameMessage& frameMsg, std::vector<uint8_t>& frameData, int frameNumber = 0);
    void Disconnect();
    SocketType GetSocket() const { return m_socket; }
    ~FrameReceiver();
};
