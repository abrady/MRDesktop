#pragma once
#include <cstdint>

// Message types for client-server communication
enum MessageType : uint32_t {
    MSG_FRAME_DATA = 1,
    MSG_MOUSE_MOVE = 2,
    MSG_MOUSE_CLICK = 3,
    MSG_MOUSE_SCROLL = 4
};

// Base message header
struct MessageHeader {
    MessageType type;
    uint32_t size;
};

// Frame data message (existing)
struct FrameMessage {
    MessageHeader header;
    uint32_t width;
    uint32_t height;
    uint32_t dataSize;
    // Pixel data follows
};

// Mouse movement message
struct MouseMoveMessage {
    MessageHeader header;
    int32_t deltaX;
    int32_t deltaY;
    bool absolute;  // If true, x/y are absolute coordinates, otherwise relative
    int32_t x;      // Used if absolute is true
    int32_t y;      // Used if absolute is true
};

// Mouse click message
struct MouseClickMessage {
    MessageHeader header;
    enum MouseButton : uint32_t {
        LEFT_BUTTON = 1,
        RIGHT_BUTTON = 2,
        MIDDLE_BUTTON = 4
    } button;
    bool pressed;   // True for press, false for release
};

// Mouse scroll message
struct MouseScrollMessage {
    MessageHeader header;
    int32_t deltaX;   // Horizontal scroll
    int32_t deltaY;   // Vertical scroll
};