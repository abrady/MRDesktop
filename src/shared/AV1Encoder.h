#pragma once
#include <vector>
#include <aom/aom_encoder.h>
#include <aom/aomcx.h>

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
    aom_codec_ctx_t m_codec;
    aom_codec_enc_cfg_t m_cfg;
};
