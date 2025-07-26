#include "VideoDecoder.h"
#include <android/log.h>

#define LOG_TAG "VideoDecoder"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

VideoDecoder::VideoDecoder() {
    LOGD("VideoDecoder created (Android)");
}

VideoDecoder::~VideoDecoder() {
    Cleanup();
}

bool VideoDecoder::Initialize(uint32_t width, uint32_t height, CompressionType compression) {
    LOGD("Initializing VideoDecoder: %dx%d, compression=%d", width, height, compression);
    
    m_Width = width;
    m_Height = height;
    m_CompressionType = compression;
    
    m_androidDecoder = std::make_unique<AndroidVideoDecoder>();
    m_IsInitialized = m_androidDecoder->Initialize(width, height, compression);
    
    return m_IsInitialized;
}

bool VideoDecoder::DecodeFrame(const uint8_t* compressedData, size_t dataSize, std::vector<uint8_t>& bgraData) {
    if (!m_IsInitialized || !m_androidDecoder) {
        return false;
    }
    
    // AndroidVideoDecoder outputs RGBA, but we need BGRA for compatibility
    std::vector<uint8_t> rgbaData;
    if (!m_androidDecoder->DecodeFrame(compressedData, dataSize, rgbaData)) {
        return false;
    }
    
    // Convert RGBA to BGRA
    bgraData.resize(rgbaData.size());
    for (size_t i = 0; i < rgbaData.size(); i += 4) {
        if (i + 3 < rgbaData.size()) {
            bgraData[i]     = rgbaData[i + 2]; // B
            bgraData[i + 1] = rgbaData[i + 1]; // G
            bgraData[i + 2] = rgbaData[i];     // R
            bgraData[i + 3] = rgbaData[i + 3]; // A
        }
    }
    
    // Save first frame for testing
    static bool firstFrameSaved = false;
    if (!firstFrameSaved) {
        m_androidDecoder->SaveFrameAsPPM(rgbaData, "decoded_frame.ppm");
        m_androidDecoder->SaveFrameAsBMP(rgbaData, "decoded_frame.bmp");
        firstFrameSaved = true;
        LOGD("First decoded frame saved to /sdcard/Download/ as .ppm and .bmp");
    }
    
    return true;
}

void VideoDecoder::Cleanup() {
    if (m_androidDecoder) {
        m_androidDecoder->Cleanup();
        m_androidDecoder.reset();
    }
    m_IsInitialized = false;
}

const char* VideoDecoder::GetCodecName(CompressionType type) {
    switch (type) {
        case COMPRESSION_H264: return "H.264 (Android MediaCodec)";
        case COMPRESSION_H265: return "H.265 (Android MediaCodec)";
        case COMPRESSION_AV1:  return "AV1 (Android MediaCodec)";
        default: return "Unknown";
    }
}