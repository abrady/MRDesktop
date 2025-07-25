#include "AV1Decoder.h"
#include <iostream>
#include <cstring>

AV1Decoder::AV1Decoder()
    : m_context(nullptr)
    , m_initialized(false) {
}

AV1Decoder::~AV1Decoder() {
    Cleanup();
}

bool AV1Decoder::Initialize() {
    if (m_initialized) {
        Cleanup();
    }
    
    // Set decoder settings
    Dav1dSettings settings;
    dav1d_default_settings(&settings);
    
    // Configure for low latency
    settings.max_frame_delay = 1;
    settings.apply_grain = 0; // Disable film grain for speed
    
    // Create decoder context
    int res = dav1d_open(&m_context, &settings);
    if (res < 0) {
        std::cerr << "AV1 Decoder: Failed to open context: " << res << std::endl;
        return false;
    }
    
    m_initialized = true;
    std::cout << "AV1 Decoder initialized successfully" << std::endl;
    return true;
}

bool AV1Decoder::DecodeFrame(const uint8_t* compressedData, size_t compressedSize, 
                            std::vector<uint8_t>& frameData, int& width, int& height) {
    if (!m_initialized) {
        return false;
    }
    
    // Allocate data buffer
    Dav1dData data;
    int res = dav1d_data_create(&data, compressedSize);
    if (res < 0) {
        std::cerr << "AV1 Decoder: Failed to create data buffer: " << res << std::endl;
        return false;
    }
    
    // Copy compressed data
    memcpy(data.data, compressedData, compressedSize);
    
    // Send data to decoder
    res = dav1d_send_data(m_context, &data);
    if (res < 0) {
        std::cerr << "AV1 Decoder: Failed to send data: " << res << std::endl;
        dav1d_data_unref(&data);
        return false;
    }
    
    // Get decoded picture
    Dav1dPicture pic;
    res = dav1d_get_picture(m_context, &pic);
    if (res < 0) {
        if (res != DAV1D_ERR(EAGAIN)) {
            std::cerr << "AV1 Decoder: Failed to get picture: " << res << std::endl;
        }
        return false;
    }
    
    // Extract dimensions
    width = pic.p.w;
    height = pic.p.h;
    
    // Allocate output buffer
    size_t outputSize = width * height * 4; // BGRA
    frameData.resize(outputSize);
    
    // Convert YUV to BGRA
    ConvertYUV420PtoBGRA(pic, frameData.data(), width, height);
    
    // Clean up picture
    dav1d_picture_unref(&pic);
    
    return true;
}

void AV1Decoder::Cleanup() {
    if (m_initialized) {
        if (m_context) {
            dav1d_close(&m_context);
            m_context = nullptr;
        }
        m_initialized = false;
    }
}

void AV1Decoder::ConvertYUV420PtoBGRA(const Dav1dPicture& pic, uint8_t* bgra, int width, int height) {
    const uint8_t* y = reinterpret_cast<const uint8_t*>(pic.data[0]);
    const uint8_t* u = reinterpret_cast<const uint8_t*>(pic.data[1]);
    const uint8_t* v = reinterpret_cast<const uint8_t*>(pic.data[2]);
    
    int yStride = pic.stride[0];
    int uvStride = pic.stride[1];
    
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            int yIdx = row * yStride + col;
            int uvIdx = (row / 2) * uvStride + (col / 2);
            int bgraIdx = (row * width + col) * 4;
            
            float yVal = y[yIdx];
            float uVal = u[uvIdx] - 128.0f;
            float vVal = v[uvIdx] - 128.0f;
            
            // ITU-R BT.709 conversion
            float r = yVal + 1.5748 * vVal;
            float g = yVal - 0.1873 * uVal - 0.4681 * vVal;
            float b = yVal + 1.8556 * uVal;
            
            // Clamp and convert to uint8_t
            bgra[bgraIdx + 0] = (uint8_t)std::max(0.0f, std::min(255.0f, b)); // B
            bgra[bgraIdx + 1] = (uint8_t)std::max(0.0f, std::min(255.0f, g)); // G
            bgra[bgraIdx + 2] = (uint8_t)std::max(0.0f, std::min(255.0f, r)); // R
            bgra[bgraIdx + 3] = 255; // A
        }
    }
}