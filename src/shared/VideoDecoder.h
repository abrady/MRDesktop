#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/frame.h>
}

#include <vector>
#include <memory>
#include "protocol.h"

class VideoDecoder {
private:
    AVCodecContext* m_CodecContext = nullptr;
    AVFrame* m_Frame = nullptr;
    AVPacket* m_Packet = nullptr;
    SwsContext* m_SwsContext = nullptr;
    
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    CompressionType m_CompressionType = COMPRESSION_NONE;
    bool m_IsInitialized = false;
    
    const char* GetCodecName(CompressionType type);
    
public:
    VideoDecoder();
    ~VideoDecoder();
    
    bool Initialize(uint32_t width, uint32_t height, CompressionType compression);
    bool DecodeFrame(const uint8_t* compressedData, size_t dataSize, std::vector<uint8_t>& bgraData);
    void Cleanup();
    
    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }
    CompressionType GetCompressionType() const { return m_CompressionType; }
    bool IsInitialized() const { return m_IsInitialized; }
};