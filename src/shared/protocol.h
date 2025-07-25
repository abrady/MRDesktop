#pragma once
#include <windows.h>

// Ensure consistent struct packing across machines
#pragma pack(push, 1)

// Message types for client-server communication
enum MessageType : UINT32 {
    MSG_FRAME_DATA = 1,
    MSG_MOUSE_MOVE = 2,
    MSG_MOUSE_CLICK = 3,
    MSG_MOUSE_SCROLL = 4,
    MSG_COMPRESSED_FRAME = 5,
    MSG_COMPRESSION_REQUEST = 6
};

// Supported compression formats
enum CompressionType : UINT32 {
    COMPRESSION_NONE = 0,
    COMPRESSION_H264 = 1,
    COMPRESSION_AV1  = 2
};

// Base message header
struct MessageHeader {
    MessageType type;
    UINT32 size;
};

// Frame data message (existing - uncompressed)
struct FrameMessage {
    MessageHeader header;
    UINT32 width;
    UINT32 height;
    UINT32 dataSize;
    // Pixel data follows
};

// Compressed frame data message
struct CompressedFrameMessage {
    MessageHeader header;
    UINT32 width;
    UINT32 height;
    UINT32 compressedSize;
    UINT32 isKeyframe;  // 1 for keyframe, 0 for delta frame
    // Compressed data follows (format determined by negotiation)
};

// Compression negotiation message sent from client to server
struct CompressionRequestMessage {
    MessageHeader header;
    CompressionType compression;
};

// Mouse movement message
struct MouseMoveMessage {
    MessageHeader header;
    INT32 deltaX;
    INT32 deltaY;
    UINT32 absolute;  // If true, x/y are absolute coordinates, otherwise relative
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
    UINT32 pressed;   // True for press, false for release
};

// Mouse scroll message
struct MouseScrollMessage {
    MessageHeader header;
    INT32 deltaX;   // Horizontal scroll
    INT32 deltaY;   // Vertical scroll
};

// Restore default packing
#pragma pack(pop)