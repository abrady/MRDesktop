#pragma once
#include <vector>
#include <x265.h>

class H265Encoder {
public:
    H265Encoder();
    ~H265Encoder();

    bool Initialize(int width, int height, int fps = 60);
    bool EncodeFrame(const uint8_t* frameData, std::vector<uint8_t>& compressedData, bool& isKeyframe);
    void ForceKeyframe();
    void Cleanup();

private:
    x265_param* m_param;
    x265_picture* m_picture;
    x265_encoder* m_encoder;
    int m_width;
    int m_height;
    int m_fps;
    int m_frameCount;
    bool m_initialized;
    bool m_forceKeyframe;
};
