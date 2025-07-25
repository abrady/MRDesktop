#pragma once
#include <vector>
#include <memory>
#include <dav1d/dav1d.h>

class AV1Decoder {
public:
    AV1Decoder();
    ~AV1Decoder();

    // Initialize decoder
    bool Initialize();
    
    // Decode a compressed frame to BGRA format
    bool DecodeFrame(const uint8_t* compressedData, size_t compressedSize, 
                     std::vector<uint8_t>& frameData, int& width, int& height);
    
    // Cleanup
    void Cleanup();

private:
    Dav1dContext* m_context;
    bool m_initialized;
    
    // Convert YUV420P to BGRA
    void ConvertYUV420PtoBGRA(const Dav1dPicture& pic, uint8_t* bgra, int width, int height);
};