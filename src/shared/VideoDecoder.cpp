#include "VideoDecoder.h"
#include <iostream>

VideoDecoder::VideoDecoder() {
    // FFmpeg 4.0+ automatically registers codecs, no need for avcodec_register_all()
}

VideoDecoder::~VideoDecoder() {
    Cleanup();
}

const char* VideoDecoder::GetCodecName(CompressionType type) {
    switch (type) {
        case COMPRESSION_H264:
            return "h264";
        case COMPRESSION_H265:
            return "hevc";
        case COMPRESSION_AV1:
            return "av1";
        default:
            return nullptr;
    }
}

bool VideoDecoder::Initialize(uint32_t width, uint32_t height, CompressionType compression) {
    if (compression == COMPRESSION_NONE) {
        std::cerr << "VideoDecoder: Cannot initialize with COMPRESSION_NONE" << std::endl;
        return false;
    }
    
    const char* codecName = GetCodecName(compression);
    if (!codecName) {
        std::cerr << "VideoDecoder: Unsupported compression type: " << compression << std::endl;
        return false;
    }
    
    // Find decoder
    const AVCodec* codec = avcodec_find_decoder_by_name(codecName);
    if (!codec) {
        std::cerr << "VideoDecoder: Could not find decoder: " << codecName << std::endl;
        return false;
    }
    
    // Allocate codec context
    m_CodecContext = avcodec_alloc_context3(codec);
    if (!m_CodecContext) {
        std::cerr << "VideoDecoder: Could not allocate codec context" << std::endl;
        return false;
    }
    
    // Set codec parameters
    m_CodecContext->width = width;
    m_CodecContext->height = height;
    m_CodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    
    // Open codec
    if (avcodec_open2(m_CodecContext, codec, nullptr) < 0) {
        std::cerr << "VideoDecoder: Could not open codec" << std::endl;
        Cleanup();
        return false;
    }
    
    // Allocate frame
    m_Frame = av_frame_alloc();
    if (!m_Frame) {
        std::cerr << "VideoDecoder: Could not allocate frame" << std::endl;
        Cleanup();
        return false;
    }
    
    // Allocate packet
    m_Packet = av_packet_alloc();
    if (!m_Packet) {
        std::cerr << "VideoDecoder: Could not allocate packet" << std::endl;
        Cleanup();
        return false;
    }
    
    // Initialize scaling context for YUV420P to BGRA conversion
    m_SwsContext = sws_getContext(width, height, AV_PIX_FMT_YUV420P,
                                  width, height, AV_PIX_FMT_BGRA,
                                  SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
    if (!m_SwsContext) {
        std::cerr << "VideoDecoder: Could not create scaling context" << std::endl;
        Cleanup();
        return false;
    }
    
    m_Width = width;
    m_Height = height;
    m_CompressionType = compression;
    m_IsInitialized = true;
    
    std::cout << "VideoDecoder: Initialized " << codecName << " decoder (" << width << "x" << height << ")" << std::endl;
    
    return true;
}

bool VideoDecoder::DecodeFrame(const uint8_t* compressedData, size_t dataSize, std::vector<uint8_t>& bgraData) {
    if (!m_IsInitialized) {
        return false;
    }
    
    // Set packet data
    m_Packet->data = const_cast<uint8_t*>(compressedData);
    m_Packet->size = static_cast<int>(dataSize);
    
    // Send packet to decoder
    int ret = avcodec_send_packet(m_CodecContext, m_Packet);
    if (ret < 0) {
        std::cerr << "VideoDecoder: Error sending packet to decoder" << std::endl;
        return false;
    }
    
    // Receive decoded frame
    ret = avcodec_receive_frame(m_CodecContext, m_Frame);
    if (ret == AVERROR(EAGAIN)) {
        // Need more packets before getting a frame
        return false;
    } else if (ret < 0) {
        std::cerr << "VideoDecoder: Error receiving frame from decoder" << std::endl;
        return false;
    }
    
    // Prepare output buffer
    size_t outputSize = m_Width * m_Height * 4; // BGRA format
    bgraData.resize(outputSize);
    
    // Convert YUV420P to BGRA
    uint8_t* dstData[4] = { bgraData.data(), nullptr, nullptr, nullptr };
    int dstLinesize[4] = { (int)(m_Width * 4), 0, 0, 0 };
    
    sws_scale(m_SwsContext, m_Frame->data, m_Frame->linesize, 0, m_Height,
              dstData, dstLinesize);
    
    return true;
}

void VideoDecoder::Cleanup() {
    if (m_SwsContext) {
        sws_freeContext(m_SwsContext);
        m_SwsContext = nullptr;
    }
    
    if (m_Packet) {
        av_packet_free(&m_Packet);
    }
    
    if (m_Frame) {
        av_frame_free(&m_Frame);
    }
    
    if (m_CodecContext) {
        avcodec_free_context(&m_CodecContext);
    }
    
    m_IsInitialized = false;
}