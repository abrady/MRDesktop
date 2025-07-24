#pragma once
#include <windows.h>

// Message types for client-server communication
enum MessageType : UINT32 {
    MSG_FRAME_DATA = 1,
    MSG_MOUSE_MOVE = 2,
    MSG_MOUSE_CLICK = 3,
    MSG_MOUSE_SCROLL = 4
};

// Base message header
struct MessageHeader {
    MessageType type;
    UINT32 size;
};

// Frame data message (existing)
struct FrameMessage {
    MessageHeader header;
    UINT32 width;
    UINT32 height;
    UINT32 dataSize;
    // Pixel data follows
};

// Mouse movement message
struct MouseMoveMessage {
    MessageHeader header;
    INT32 deltaX;
    INT32 deltaY;
    BOOL absolute;  // If true, x/y are absolute coordinates, otherwise relative
    INT32 x;        // Used if absolute is true
    INT32 y;        // Used if absolute is true
};

// Mouse click message
struct MouseClickMessage {
    MessageHeader header;
    enum MouseButton : UINT32 {
        LEFT_BUTTON = 1,
        RIGHT_BUTTON = 2,
        MIDDLE_BUTTON = 4
    } button;
    BOOL pressed;   // True for press, false for release
};

// Mouse scroll message
struct MouseScrollMessage {
    MessageHeader header;
    INT32 deltaX;   // Horizontal scroll
    INT32 deltaY;   // Vertical scroll
};