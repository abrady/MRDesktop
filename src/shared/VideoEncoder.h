#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/frame.h>
}

#include <vector>
#include <memory>
#include "protocol.h"

class VideoEncoder {
private:
    AVCodecContext* m_CodecContext = nullptr;
    AVFrame* m_Frame = nullptr;
    AVPacket* m_Packet = nullptr;
    SwsContext* m_SwsContext = nullptr;
    
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    uint32_t m_Framerate = 60;
    uint32_t m_Bitrate = 5000000; // 5 Mbps default
    CompressionType m_CompressionType = COMPRESSION_NONE;
    bool m_IsInitialized = false;
    int64_t m_FrameCount = 0;
    
    const char* GetCodecName(CompressionType type);
    
public:
    VideoEncoder();
    ~VideoEncoder();
    
    bool Initialize(uint32_t width, uint32_t height, CompressionType compression, 
                   uint32_t framerate = 60, uint32_t bitrate = 5000000);
    bool EncodeFrame(const uint8_t* bgraData, std::vector<uint8_t>& compressedData, bool& isKeyframe);
    void Cleanup();
    
    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }
    CompressionType GetCompressionType() const { return m_CompressionType; }
    bool IsInitialized() const { return m_IsInitialized; }
};