#pragma once
#include <cstdint>

// Ensure consistent struct packing across machines
#pragma pack(push, 1)

// Message types for client-server communication
enum MessageType : uint32_t {
    MSG_FRAME_DATA = 1,
    MSG_MOUSE_MOVE = 2,
    MSG_MOUSE_CLICK = 3,
    MSG_MOUSE_SCROLL = 4,
    MSG_COMPRESSED_FRAME = 5,
    MSG_COMPRESSION_REQUEST = 6
};

// Supported compression formats
enum CompressionType : uint32_t {
    COMPRESSION_NONE = 0,
    COMPRESSION_H264 = 1,
    COMPRESSION_AV1  = 2
};

// Base message header
struct MessageHeader {
    MessageType type;
    uint32_t size;
};

// Frame data message (existing - uncompressed)
struct FrameMessage {
    MessageHeader header;
    uint32_t width;
    uint32_t height;
    uint32_t dataSize;
    // Pixel data follows
};

// Compressed frame data message
struct CompressedFrameMessage {
    MessageHeader header;
    uint32_t width;
    uint32_t height;
    uint32_t compressedSize;
    uint32_t isKeyframe;  // 1 for keyframe, 0 for delta frame
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
    int32_t deltaX;
    int32_t deltaY;
    uint32_t absolute;  // If true, x/y are absolute coordinates, otherwise relative
    int32_t x;        // Used if absolute is true
    int32_t y;        // Used if absolute is true
};

// Mouse click message
struct MouseClickMessage {
    MessageHeader header;
    enum MouseButton : uint32_t {
        LEFT_BUTTON = 1,
        RIGHT_BUTTON = 2,
        MIDDLE_BUTTON = 4
    } button;
    uint32_t pressed;   // True for press, false for release
};

// Mouse scroll message
struct MouseScrollMessage {
    MessageHeader header;
    int32_t deltaX;   // Horizontal scroll
    int32_t deltaY;   // Vertical scroll
};

// Restore default packing
#pragma pack(pop)
