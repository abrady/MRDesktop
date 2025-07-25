#pragma once
#include <vector>
#include <svt-av1/EbSvtAv1Dec.h>

class AV1Decoder {
public:
    AV1Decoder();
    ~AV1Decoder();

    bool Initialize(int width, int height);
    bool DecodeFrame(const uint8_t* data, size_t size, std::vector<uint8_t>& outBgra);
    void Cleanup();

private:
    EbComponentType* m_handle;
    EbSvtAv1DecConfiguration m_cfg;
    bool m_initialized;
};
