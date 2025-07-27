#pragma once

#include <cstdint>
#include <vector>
#include "protocol.h"

/**
 * Abstract interface for video decoders.
 * Allows platform-specific implementations (desktop VideoDecoder, AndroidVideoDecoder, etc.)
 */
class IVideoDecoder {
public:
    virtual ~IVideoDecoder() = default;
    
    virtual bool Initialize(uint32_t width, uint32_t height, CompressionType compression) = 0;
    virtual bool DecodeFrame(const uint8_t* compressedData, size_t dataSize, std::vector<uint8_t>& decodedData) = 0;
    virtual bool IsInitialized() const = 0;
    virtual void Cleanup() = 0;
};