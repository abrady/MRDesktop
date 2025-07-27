#pragma once

#include <vector>
#include <cstdint>

class FrameRenderer {
public:
    FrameRenderer();
    ~FrameRenderer();
    
    // Convert YUV420P to ARGB8888 format for Android display
    bool ConvertYUVToARGB(const std::vector<uint8_t>& yuvData, 
                          uint32_t width, uint32_t height,
                          std::vector<uint8_t>& argbData);
                          
    // Validate YUV data size and format
    bool ValidateYUVData(const std::vector<uint8_t>& yuvData, 
                         uint32_t width, uint32_t height);

private:
    // Helper methods for YUV to RGB conversion
    uint8_t ClampByte(int32_t value);
    void ConvertYUVPixelToRGB(uint8_t y, uint8_t u, uint8_t v,
                              uint8_t& r, uint8_t& g, uint8_t& b);
};