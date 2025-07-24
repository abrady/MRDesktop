#pragma once
#include "protocol.h"

class InputSender {
private:
    int m_socket;
    
public:
    InputSender(int socket);
    
    bool sendMouseMove(int32_t deltaX, int32_t deltaY, bool absolute = false, int32_t x = 0, int32_t y = 0);
    bool sendMouseClick(MouseClickMessage::MouseButton button, bool pressed);
    bool sendMouseScroll(int32_t deltaX, int32_t deltaY);
};