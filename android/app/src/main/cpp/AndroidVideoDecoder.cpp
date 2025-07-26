#include "AndroidVideoDecoder.h"
#include <android/log.h>
#include <fstream>

#define LOG_TAG "AndroidVideoDecoder"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

AndroidVideoDecoder::AndroidVideoDecoder() {
    LOGD("AndroidVideoDecoder created");
}

AndroidVideoDecoder::~AndroidVideoDecoder() {
    Cleanup();
}

const char* AndroidVideoDecoder::GetMimeType(CompressionType type) {
    switch (type) {
        case COMPRESSION_H264:
            return "video/avc";
        case COMPRESSION_H265:
            return "video/hevc";
        case COMPRESSION_AV1:
            return "video/av01";
        default:
            return nullptr;
    }
}

bool AndroidVideoDecoder::Initialize(uint32_t width, uint32_t height, CompressionType compression) {
    LOGD("Initializing decoder: %dx%d, compression=%d", width, height, compression);
    
    Cleanup(); // Clean up any existing decoder
    
    m_width = width;
    m_height = height;
    m_compressionType = compression;
    
    const char* mimeType = GetMimeType(compression);
    if (!mimeType) {
        LOGE("Unsupported compression type: %d", compression);
        return false;
    }
    
    // Create decoder for the specified MIME type
    m_codec = AMediaCodec_createDecoderByType(mimeType);
    if (!m_codec) {
        LOGE("Failed to create decoder for MIME type: %s", mimeType);
        return false;
    }
    
    // Create format
    m_format = AMediaFormat_new();
    AMediaFormat_setString(m_format, AMEDIAFORMAT_KEY_MIME, mimeType);
    AMediaFormat_setInt32(m_format, AMEDIAFORMAT_KEY_WIDTH, width);
    AMediaFormat_setInt32(m_format, AMEDIAFORMAT_KEY_HEIGHT, height);
    
    // Configure the decoder
    media_status_t status = AMediaCodec_configure(m_codec, m_format, nullptr, nullptr, 0);
    if (status != AMEDIA_OK) {
        LOGE("Failed to configure decoder: %d", status);
        Cleanup();
        return false;
    }
    
    // Start the decoder
    status = AMediaCodec_start(m_codec);
    if (status != AMEDIA_OK) {
        LOGE("Failed to start decoder: %d", status);
        Cleanup();
        return false;
    }
    
    m_isInitialized = true;
    LOGD("Decoder initialized successfully");
    return true;
}

bool AndroidVideoDecoder::DecodeFrame(const uint8_t* compressedData, size_t dataSize, std::vector<uint8_t>& rgbaData) {
    if (!m_isInitialized || !m_codec) {
        LOGE("Decoder not initialized");
        return false;
    }
    
    LOGD("Decoding frame: %zu bytes", dataSize);
    
    // Get input buffer
    ssize_t inputBufferIndex = AMediaCodec_dequeueInputBuffer(m_codec, 10000); // 10ms timeout
    if (inputBufferIndex < 0) {
        LOGE("No input buffer available: %zd", inputBufferIndex);
        return false;
    }
    
    // Fill input buffer
    size_t inputBufferSize;
    uint8_t* inputBuffer = AMediaCodec_getInputBuffer(m_codec, inputBufferIndex, &inputBufferSize);
    if (!inputBuffer || inputBufferSize < dataSize) {
        LOGE("Input buffer too small: %zu < %zu", inputBufferSize, dataSize);
        return false;
    }
    
    memcpy(inputBuffer, compressedData, dataSize);
    
    // Queue input buffer
    media_status_t status = AMediaCodec_queueInputBuffer(m_codec, inputBufferIndex, 0, dataSize, 0, 0);
    if (status != AMEDIA_OK) {
        LOGE("Failed to queue input buffer: %d", status);
        return false;
    }
    
    // Get output buffer
    AMediaCodecBufferInfo bufferInfo;
    ssize_t outputBufferIndex = AMediaCodec_dequeueOutputBuffer(m_codec, &bufferInfo, 10000); // 10ms timeout
    
    if (outputBufferIndex == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
        LOGD("Output format changed");
        AMediaFormat* newFormat = AMediaCodec_getOutputFormat(m_codec);
        if (newFormat) {
            int32_t width, height;
            AMediaFormat_getInt32(newFormat, AMEDIAFORMAT_KEY_WIDTH, &width);
            AMediaFormat_getInt32(newFormat, AMEDIAFORMAT_KEY_HEIGHT, &height);
            LOGD("New output format: %dx%d", width, height);
            AMediaFormat_delete(newFormat);
        }
        return false; // Try again next time
    }
    
    if (outputBufferIndex < 0) {
        LOGE("No output buffer available: %zd", outputBufferIndex);
        return false;
    }
    
    // Get decoded data
    size_t outputBufferSize;
    uint8_t* outputBuffer = AMediaCodec_getOutputBuffer(m_codec, outputBufferIndex, &outputBufferSize);
    if (!outputBuffer) {
        LOGE("Failed to get output buffer");
        AMediaCodec_releaseOutputBuffer(m_codec, outputBufferIndex, false);
        return false;
    }
    
    // Validate buffer size
    if (bufferInfo.size <= 0 || bufferInfo.size > (int32_t)outputBufferSize) {
        LOGE("Invalid buffer size: bufferInfo.size=%d, outputBufferSize=%zu", bufferInfo.size, outputBufferSize);
        AMediaCodec_releaseOutputBuffer(m_codec, outputBufferIndex, false);
        return false;
    }
    
    LOGD("Decoded frame: %d bytes (buffer size: %zu)", (int)bufferInfo.size, outputBufferSize);
    
    // For now, just copy the raw decoded data safely
    // In a real implementation, you'd need to convert from YUV to RGBA
    try {
        rgbaData.resize(bufferInfo.size);
        if (bufferInfo.size > 0) {
            memcpy(rgbaData.data(), outputBuffer + bufferInfo.offset, bufferInfo.size);
        }
    } catch (const std::exception& e) {
        LOGE("Failed to copy buffer data: %s", e.what());
        AMediaCodec_releaseOutputBuffer(m_codec, outputBufferIndex, false);
        return false;
    }
    
    // Release output buffer
    AMediaCodec_releaseOutputBuffer(m_codec, outputBufferIndex, false);
    
    return true;
}

void AndroidVideoDecoder::Cleanup() {
    if (m_codec) {
        AMediaCodec_stop(m_codec);
        AMediaCodec_delete(m_codec);
        m_codec = nullptr;
    }
    
    if (m_format) {
        AMediaFormat_delete(m_format);
        m_format = nullptr;
    }
    
    m_isInitialized = false;
    LOGD("Decoder cleaned up");
}

bool AndroidVideoDecoder::SaveFrameAsPPM(const std::vector<uint8_t>& rgbaData, const std::string& filename) {
    if (rgbaData.empty() || m_width == 0 || m_height == 0) {
        LOGE("Invalid frame data for saving");
        return false;
    }
    
    // Save to external storage path that's accessible
    std::string fullPath = "/sdcard/Download/" + filename;
    
    std::ofstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        LOGE("Failed to open file for writing: %s", fullPath.c_str());
        return false;
    }
    
    LOGD("Saving frame: %dx%d, data size: %zu bytes", m_width, m_height, rgbaData.size());
    
    // For debugging, save raw data first
    std::string rawPath = "/sdcard/Download/raw_" + filename + ".bin";
    std::ofstream rawFile(rawPath, std::ios::binary);
    if (rawFile.is_open()) {
        rawFile.write(reinterpret_cast<const char*>(rgbaData.data()), rgbaData.size());
        rawFile.close();
        LOGD("Raw frame data saved as: %s", rawPath.c_str());
    }
    
    // Since MediaCodec outputs YUV data, not RGBA, create a simple grayscale image
    // Using Y component for now (first byte of each pixel in NV21/YUV420)
    file << "P5\n" << m_width << " " << m_height << "\n255\n";
    
    // Write Y component (grayscale)
    size_t ySize = m_width * m_height;
    if (rgbaData.size() >= ySize) {
        file.write(reinterpret_cast<const char*>(rgbaData.data()), ySize);
    } else {
        LOGE("Insufficient data for Y component: have %zu bytes, need %zu", rgbaData.size(), ySize);
        file.close();
        return false;
    }
    
    file.close();
    LOGD("Frame saved as grayscale: %s", fullPath.c_str());
    return true;
}

bool AndroidVideoDecoder::SaveFrameAsBMP(const std::vector<uint8_t>& yuvData, const std::string& filename) {
    if (yuvData.empty() || m_width == 0 || m_height == 0) {
        LOGE("Invalid frame data for saving BMP");
        return false;
    }
    
    // Save to external storage path that's accessible
    std::string fullPath = "/sdcard/Download/" + filename;
    
    std::ofstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        LOGE("Failed to open BMP file for writing: %s", fullPath.c_str());
        return false;
    }
    
    LOGD("Saving BMP: %dx%d, data size: %zu bytes", m_width, m_height, yuvData.size());
    
    // BMP Header structures
    struct BMPFileHeader {
        uint16_t type = 0x4D42;     // "BM"
        uint32_t size;              // File size
        uint16_t reserved1 = 0;
        uint16_t reserved2 = 0;
        uint32_t offset = 54;       // Offset to pixel data (14 + 40)
    } __attribute__((packed));
    
    struct BMPInfoHeader {
        uint32_t size = 40;         // Info header size
        int32_t width;              // Image width
        int32_t height;             // Image height (negative for top-down)
        uint16_t planes = 1;        // Color planes
        uint16_t bitCount = 24;     // Bits per pixel (RGB)
        uint32_t compression = 0;   // No compression
        uint32_t sizeImage = 0;     // Image size (can be 0 for uncompressed)
        int32_t xPelsPerMeter = 0;  // Horizontal resolution
        int32_t yPelsPerMeter = 0;  // Vertical resolution
        uint32_t clrUsed = 0;       // Colors used
        uint32_t clrImportant = 0;  // Important colors
    } __attribute__((packed));
    
    // Calculate row padding (BMP rows must be multiple of 4 bytes)
    uint32_t rowSize = ((m_width * 3 + 3) / 4) * 4;
    uint32_t imageSize = rowSize * m_height;
    
    // Prepare headers
    BMPFileHeader fileHeader;
    fileHeader.size = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + imageSize;
    
    BMPInfoHeader infoHeader;
    infoHeader.width = m_width;
    infoHeader.height = -(int32_t)m_height; // Negative for top-down
    infoHeader.sizeImage = imageSize;
    
    // Write headers
    file.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
    file.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));
    
    // Convert YUV to RGB and write pixel data
    // For now, create grayscale by using Y component as RGB
    size_t ySize = m_width * m_height;
    if (yuvData.size() < ySize) {
        LOGE("Insufficient Y data for BMP: have %zu bytes, need %zu", yuvData.size(), ySize);
        file.close();
        return false;
    }
    
    // Write pixel data row by row (BMP is bottom-up by default, but we use negative height for top-down)
    std::vector<uint8_t> rowBuffer(rowSize, 0);
    for (uint32_t y = 0; y < m_height; y++) {
        for (uint32_t x = 0; x < m_width; x++) {
            uint8_t yValue = yuvData[y * m_width + x];
            uint32_t pixelOffset = x * 3;
            
            // BGR format (BMP uses BGR, not RGB)
            rowBuffer[pixelOffset]     = yValue; // B
            rowBuffer[pixelOffset + 1] = yValue; // G
            rowBuffer[pixelOffset + 2] = yValue; // R
        }
        file.write(reinterpret_cast<const char*>(rowBuffer.data()), rowSize);
    }
    
    file.close();
    LOGD("BMP frame saved as: %s", fullPath.c_str());
    return true;
}