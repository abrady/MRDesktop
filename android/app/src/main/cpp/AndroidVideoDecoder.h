#pragma once

#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>
#include <vector>
#include <string>
#include "protocol.h"

class AndroidVideoDecoder {
private:
    AMediaCodec* m_codec = nullptr;
    AMediaFormat* m_format = nullptr;
    
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    CompressionType m_compressionType = COMPRESSION_NONE;
    bool m_isInitialized = false;
    
    const char* GetMimeType(CompressionType type);
    
public:
    AndroidVideoDecoder();
    ~AndroidVideoDecoder();
    
    bool Initialize(uint32_t width, uint32_t height, CompressionType compression);
    bool DecodeFrame(const uint8_t* compressedData, size_t dataSize, std::vector<uint8_t>& rgbaData);
    void Cleanup();
    
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    CompressionType GetCompressionType() const { return m_compressionType; }
    bool IsInitialized() const { return m_isInitialized; }
    
    // Test functionality
    bool SaveFrameAsPPM(const std::vector<uint8_t>& rgbaData, const std::string& filename);
    bool SaveFrameAsBMP(const std::vector<uint8_t>& yuvData, const std::string& filename);
};