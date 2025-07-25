#include "AV1Decoder.h"
#include <cstring>
#include <algorithm>

AV1Decoder::AV1Decoder() : m_handle(nullptr), m_initialized(false) {
    std::memset(&m_cfg, 0, sizeof(m_cfg));
}

AV1Decoder::~AV1Decoder() { Cleanup(); }

bool AV1Decoder::Initialize(int width, int height) {
    if (m_initialized) return true;
    if (svt_av1_dec_init_handle(&m_handle, nullptr, &m_cfg) != EB_ErrorNone)
        return false;
    m_cfg.max_picture_width = width;
    m_cfg.max_picture_height = height;
    m_cfg.threads = 0;
    if (svt_av1_dec_set_parameter(m_handle, &m_cfg) != EB_ErrorNone)
        return false;
    if (svt_av1_dec_init(m_handle) != EB_ErrorNone)
        return false;
    m_initialized = true;
    return true;
}

static void I420ToBGRA(const uint8_t* src, int width, int height, std::vector<uint8_t>& bgra) {
    int frameSize = width * height;
    const uint8_t* yPlane = src;
    const uint8_t* uPlane = yPlane + frameSize;
    const uint8_t* vPlane = uPlane + frameSize / 4;
    bgra.resize(frameSize * 4);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int Y = yPlane[y * width + x];
            int U = uPlane[(y/2) * (width/2) + (x/2)] - 128;
            int V = vPlane[(y/2) * (width/2) + (x/2)] - 128;
            int C = Y - 16;
            int D = U;
            int E = V;
            int R = (298 * C + 409 * E + 128) >> 8;
            int G = (298 * C - 100 * D - 208 * E + 128) >> 8;
            int B = (298 * C + 516 * D + 128) >> 8;
            R = std::clamp(R, 0, 255);
            G = std::clamp(G, 0, 255);
            B = std::clamp(B, 0, 255);
            uint8_t* px = &bgra[(y * width + x) * 4];
            px[0] = static_cast<uint8_t>(B);
            px[1] = static_cast<uint8_t>(G);
            px[2] = static_cast<uint8_t>(R);
            px[3] = 255;
        }
    }
}

bool AV1Decoder::DecodeFrame(const uint8_t* data, size_t size, std::vector<uint8_t>& outBgra) {
    if (!m_initialized) return false;
    if (svt_av1_dec_frame(m_handle, data, size, 0) != EB_ErrorNone)
        return false;
    EbBufferHeaderType output{};
    if (svt_av1_dec_get_picture(m_handle, &output, nullptr, nullptr) != EB_ErrorNone)
        return false;
    EbSvtIOFormat* img = reinterpret_cast<EbSvtIOFormat*>(output.p_buffer);
    int width = static_cast<int>(img->width);
    int height = static_cast<int>(img->height);
    int frameSize = width * height;
    std::vector<uint8_t> yuv(frameSize * 3 / 2);
    for (int y = 0; y < height; ++y)
        std::memcpy(&yuv[y * width], img->luma + y * img->y_stride, width);
    for (int y = 0; y < height / 2; ++y)
        std::memcpy(&yuv[frameSize + y * (width / 2)], img->cb + y * img->cb_stride, width / 2);
    for (int y = 0; y < height / 2; ++y)
        std::memcpy(&yuv[frameSize + frameSize / 4 + y * (width / 2)], img->cr + y * img->cr_stride, width / 2);
    I420ToBGRA(yuv.data(), width, height, outBgra);
    return true;
}

void AV1Decoder::Cleanup() {
    if (m_initialized) {
        svt_av1_dec_deinit(m_handle);
        svt_av1_dec_deinit_handle(m_handle);
        m_handle = nullptr;
        m_initialized = false;
    }
}
