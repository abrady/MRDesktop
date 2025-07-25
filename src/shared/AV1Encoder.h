#pragma once
#include <vector>
#include <svt-av1/EbSvtAv1Enc.h>

class AV1Encoder {
public:
    AV1Encoder();
    ~AV1Encoder();

    bool Initialize(int width, int height, int fps = 60);
    bool EncodeFrame(const uint8_t* frameData, std::vector<uint8_t>& compressedData, bool& isKeyframe);
    void ForceKeyframe();
    void Cleanup();

private:
    int m_width;
    int m_height;
    int m_fps;
    bool m_initialized;
    bool m_forceKeyframe;
    int m_frameCount;
    EbComponentType* m_handle;
    EbSvtAv1EncConfiguration m_cfg;
};
