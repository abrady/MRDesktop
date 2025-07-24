#pragma once
#include "protocol.h"
#include <vector>

class FrameReceiver {
private:
    int m_socket;
    std::vector<uint8_t> m_frameBuffer;
    
public:
    FrameReceiver();
    ~FrameReceiver();
    
    bool connect(const char* serverIP, int port);
    void disconnect();
    bool receiveFrame(FrameMessage& frameMsg, std::vector<uint8_t>& frameData);
    int getSocket() const;
};