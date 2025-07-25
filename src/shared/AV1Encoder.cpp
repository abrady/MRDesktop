#include "AV1Encoder.h"
#include <cstring>

AV1Encoder::AV1Encoder()
    : m_width(0), m_height(0), m_fps(60), m_initialized(false), m_forceKeyframe(false), m_frameCount(0), m_handle(nullptr) {
    std::memset(&m_cfg, 0, sizeof(m_cfg));
}

AV1Encoder::~AV1Encoder() { Cleanup(); }

bool AV1Encoder::Initialize(int width, int height, int fps) {
    m_width = width;
    m_height = height;
    m_fps = fps;
    m_frameCount = 0;
    m_forceKeyframe = false;

    if (svt_av1_enc_init_handle(&m_handle, nullptr, &m_cfg) != EB_ErrorNone)
        return false;

    m_cfg.source_width = width;
    m_cfg.source_height = height;
    m_cfg.frame_rate_numerator = fps;
    m_cfg.frame_rate_denominator = 1;
    m_cfg.target_bit_rate = 5000000; // bits per second
    m_cfg.enc_mode = 12; // speed vs quality tradeoff

    if (svt_av1_enc_set_parameter(m_handle, &m_cfg) != EB_ErrorNone)
        return false;

    if (svt_av1_enc_init(m_handle) != EB_ErrorNone)
        return false;

    m_initialized = true;
    return true;
}

static void BGRAtoI420(const uint8_t* src, int width, int height, std::vector<uint8_t>& yuv) {
    int frameSize = width * height;
    yuv.resize(frameSize * 3 / 2);
    uint8_t* yPlane = yuv.data();
    uint8_t* uPlane = yPlane + frameSize;
    uint8_t* vPlane = uPlane + frameSize / 4;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const uint8_t* px = src + (y * width + x) * 4;
            uint8_t b = px[0];
            uint8_t g = px[1];
            uint8_t r = px[2];
            int Y = static_cast<int>(0.2126 * r + 0.7152 * g + 0.0722 * b);
            yPlane[y * width + x] = static_cast<uint8_t>(Y);
            if ((y % 2 == 0) && (x % 2 == 0)) {
                int U = static_cast<int>(-0.1146 * r - 0.3854 * g + 0.5 * b + 128);
                int V = static_cast<int>(0.5 * r - 0.4542 * g - 0.0458 * b + 128);
                int idx = (y / 2) * (width / 2) + (x / 2);
                uPlane[idx] = static_cast<uint8_t>(U);
                vPlane[idx] = static_cast<uint8_t>(V);
            }
        }
    }
}

bool AV1Encoder::EncodeFrame(const uint8_t* frameData, std::vector<uint8_t>& compressedData, bool& isKeyframe) {
    if (!m_initialized) return false;

    std::vector<uint8_t> yuv;
    BGRAtoI420(frameData, m_width, m_height, yuv);

    EbBufferHeaderType input{};
    input.size = sizeof(EbBufferHeaderType);
    input.p_buffer = yuv.data();
    input.n_filled_len = (uint32_t)yuv.size();
    input.n_alloc_len = (uint32_t)yuv.size();
    input.pic_type = m_forceKeyframe ? EB_AV1_KEY_PICTURE : EB_AV1_INVALID_PICTURE;

    if (svt_av1_enc_send_picture(m_handle, &input) != EB_ErrorNone)
        return false;

    m_forceKeyframe = false;

    EbBufferHeaderType* output = nullptr;
    compressedData.clear();
    while (svt_av1_enc_get_packet(m_handle, &output, 0) == EB_ErrorNone && output) {
        const uint8_t* buf = output->p_buffer;
        compressedData.insert(compressedData.end(), buf, buf + output->n_filled_len);
        isKeyframe = (output->pic_type == EB_AV1_KEY_PICTURE);
        svt_av1_enc_release_out_buffer(&output);
    }

    m_frameCount++;
    return !compressedData.empty();
}

void AV1Encoder::ForceKeyframe() { m_forceKeyframe = true; }

void AV1Encoder::Cleanup() {
    if (m_initialized) {
        svt_av1_enc_deinit(m_handle);
        svt_av1_enc_deinit_handle(m_handle);
        m_handle = nullptr;
        m_initialized = false;
    }
}
