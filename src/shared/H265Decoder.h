#pragma once
#include <vector>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

class H265Decoder {
public:
    H265Decoder();
    ~H265Decoder();

    bool Initialize();
    bool DecodeFrame(const uint8_t* data, size_t size, std::vector<uint8_t>& bgra, int& width, int& height, bool& isKeyframe);
    void Cleanup();

private:
    AVCodecContext* m_ctx;
    AVFrame* m_frame;
    AVPacket* m_packet;
};
