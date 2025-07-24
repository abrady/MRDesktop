#include "input_sender.h"
#include <android/log.h>
#include <sys/socket.h>
#include <cstring>

#define LOG_TAG "InputSender"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

InputSender::InputSender(int socket) : m_socket(socket) {
}

bool InputSender::sendMouseMove(int32_t deltaX, int32_t deltaY, bool absolute, int32_t x, int32_t y) {
    MouseMoveMessage msg;
    msg.header.type = MSG_MOUSE_MOVE;
    msg.header.size = sizeof(MouseMoveMessage);
    msg.deltaX = deltaX;
    msg.deltaY = deltaY;
    msg.absolute = absolute;
    msg.x = x;
    msg.y = y;
    
    ssize_t sent = send(m_socket, &msg, sizeof(msg), 0);
    if (sent != sizeof(msg)) {
        LOGE("Failed to send mouse move message");
        return false;
    }
    
    LOGD("Sent mouse move: dx=%d, dy=%d, abs=%d, x=%d, y=%d", deltaX, deltaY, absolute, x, y);
    return true;
}

bool InputSender::sendMouseClick(MouseClickMessage::MouseButton button, bool pressed) {
    MouseClickMessage msg;
    msg.header.type = MSG_MOUSE_CLICK;
    msg.header.size = sizeof(MouseClickMessage);
    msg.button = button;
    msg.pressed = pressed;
    
    ssize_t sent = send(m_socket, &msg, sizeof(msg), 0);
    if (sent != sizeof(msg)) {
        LOGE("Failed to send mouse click message");
        return false;
    }
    
    LOGD("Sent mouse %s button %d", pressed ? "press" : "release", button);
    return true;
}

bool InputSender::sendMouseScroll(int32_t deltaX, int32_t deltaY) {
    MouseScrollMessage msg;
    msg.header.type = MSG_MOUSE_SCROLL;
    msg.header.size = sizeof(MouseScrollMessage);
    msg.deltaX = deltaX;
    msg.deltaY = deltaY;
    
    ssize_t sent = send(m_socket, &msg, sizeof(msg), 0);
    if (sent != sizeof(msg)) {
        LOGE("Failed to send mouse scroll message");
        return false;
    }
    
    LOGD("Sent mouse scroll: dx=%d, dy=%d", deltaX, deltaY);
    return true;
}