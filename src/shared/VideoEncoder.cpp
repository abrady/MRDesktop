#include "VideoEncoder.h"
#include <iostream>

VideoEncoder::VideoEncoder() {
    // Initialize ffmpeg (only needs to be done once globally)
    static bool ffmpeg_initialized = false;
    if (!ffmpeg_initialized) {
        avcodec_register_all();
        ffmpeg_initialized = true;
    }
}

VideoEncoder::~VideoEncoder() {
    Cleanup();
}

const char* VideoEncoder::GetCodecName(CompressionType type) {
    switch (type) {
        case COMPRESSION_H264:
            return "libx264";
        case COMPRESSION_H265:
            return "libx265";
        case COMPRESSION_AV1:
            return "libaom-av1";
        default:
            return nullptr;
    }
}

bool VideoEncoder::Initialize(uint32_t width, uint32_t height, CompressionType compression, 
                             uint32_t framerate, uint32_t bitrate) {
    if (compression == COMPRESSION_NONE) {
        std::cerr << "VideoEncoder: Cannot initialize with COMPRESSION_NONE" << std::endl;
        return false;
    }
    
    const char* codecName = GetCodecName(compression);
    if (!codecName) {
        std::cerr << "VideoEncoder: Unsupported compression type: " << compression << std::endl;
        return false;
    }
    
    // Find encoder
    AVCodec* codec = avcodec_find_encoder_by_name(codecName);
    if (!codec) {
        std::cerr << "VideoEncoder: Could not find encoder: " << codecName << std::endl;
        return false;
    }
    
    // Allocate codec context
    m_CodecContext = avcodec_alloc_context3(codec);
    if (!m_CodecContext) {
        std::cerr << "VideoEncoder: Could not allocate codec context" << std::endl;
        return false;
    }
    
    // Set codec parameters
    m_CodecContext->bit_rate = bitrate;
    m_CodecContext->width = width;
    m_CodecContext->height = height;
    m_CodecContext->time_base = {1, (int)framerate};
    m_CodecContext->framerate = {(int)framerate, 1};
    m_CodecContext->gop_size = framerate; // Keyframe every second
    m_CodecContext->max_b_frames = 0; // Disable B-frames for low latency
    m_CodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    
    // Set codec-specific options for low latency
    if (compression == COMPRESSION_H264) {
        av_opt_set(m_CodecContext->priv_data, "preset", "ultrafast", 0);
        av_opt_set(m_CodecContext->priv_data, "tune", "zerolatency", 0);
    } else if (compression == COMPRESSION_H265) {
        av_opt_set(m_CodecContext->priv_data, "preset", "ultrafast", 0);
        av_opt_set(m_CodecContext->priv_data, "tune", "zerolatency", 0);
    }
    
    // Open codec
    if (avcodec_open2(m_CodecContext, codec, nullptr) < 0) {
        std::cerr << "VideoEncoder: Could not open codec" << std::endl;
        Cleanup();
        return false;
    }
    
    // Allocate frame
    m_Frame = av_frame_alloc();
    if (!m_Frame) {
        std::cerr << "VideoEncoder: Could not allocate frame" << std::endl;
        Cleanup();
        return false;
    }
    
    m_Frame->format = m_CodecContext->pix_fmt;
    m_Frame->width = width;
    m_Frame->height = height;
    
    if (av_frame_get_buffer(m_Frame, 32) < 0) {
        std::cerr << "VideoEncoder: Could not allocate frame buffer" << std::endl;
        Cleanup();
        return false;
    }
    
    // Allocate packet
    m_Packet = av_packet_alloc();
    if (!m_Packet) {
        std::cerr << "VideoEncoder: Could not allocate packet" << std::endl;
        Cleanup();
        return false;
    }
    
    // Initialize scaling context for BGRA to YUV420P conversion
    m_SwsContext = sws_getContext(width, height, AV_PIX_FMT_BGRA,
                                  width, height, AV_PIX_FMT_YUV420P,
                                  SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
    if (!m_SwsContext) {
        std::cerr << "VideoEncoder: Could not create scaling context" << std::endl;
        Cleanup();
        return false;
    }
    
    m_Width = width;
    m_Height = height;
    m_Framerate = framerate;
    m_Bitrate = bitrate;
    m_CompressionType = compression;
    m_IsInitialized = true;
    
    std::cout << "VideoEncoder: Initialized " << codecName << " encoder (" << width << "x" << height 
              << " @ " << framerate << "fps, " << bitrate << " bps)" << std::endl;
    
    return true;
}

bool VideoEncoder::EncodeFrame(const uint8_t* bgraData, std::vector<uint8_t>& compressedData, bool& isKeyframe) {
    if (!m_IsInitialized) {
        return false;
    }
    
    // Convert BGRA to YUV420P
    const uint8_t* srcData[4] = { bgraData, nullptr, nullptr, nullptr };
    int srcLinesize[4] = { (int)(m_Width * 4), 0, 0, 0 };
    
    sws_scale(m_SwsContext, srcData, srcLinesize, 0, m_Height,
              m_Frame->data, m_Frame->linesize);
    
    // Set frame PTS
    static int64_t pts = 0;
    m_Frame->pts = pts++;
    
    // Send frame to encoder
    int ret = avcodec_send_frame(m_CodecContext, m_Frame);
    if (ret < 0) {
        std::cerr << "VideoEncoder: Error sending frame to encoder" << std::endl;
        return false;
    }
    
    // Receive encoded packet
    ret = avcodec_receive_packet(m_CodecContext, m_Packet);
    if (ret == AVERROR(EAGAIN)) {
        // Need more frames before getting a packet
        return false;
    } else if (ret < 0) {
        std::cerr << "VideoEncoder: Error receiving packet from encoder" << std::endl;
        return false;
    }
    
    // Copy compressed data
    compressedData.resize(m_Packet->size);
    memcpy(compressedData.data(), m_Packet->data, m_Packet->size);
    
    // Check if this is a keyframe
    isKeyframe = (m_Packet->flags & AV_PKT_FLAG_KEY) != 0;
    
    av_packet_unref(m_Packet);
    
    return true;
}

void VideoEncoder::Cleanup() {
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