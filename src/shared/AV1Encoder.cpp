#include "AV1Encoder.h"
#include <cstring>

AV1Encoder::AV1Encoder()
    : m_width(0), m_height(0), m_fps(60), m_initialized(false), m_forceKeyframe(false), m_frameCount(0) {
    std::memset(&m_codec, 0, sizeof(m_codec));
    std::memset(&m_cfg, 0, sizeof(m_cfg));
}

AV1Encoder::~AV1Encoder() { Cleanup(); }

bool AV1Encoder::Initialize(int width, int height, int fps) {
    m_width = width;
    m_height = height;
    m_fps = fps;
    m_frameCount = 0;
    m_forceKeyframe = false;

    const aom_codec_iface_t* iface = aom_codec_av1_cx();
    if (aom_codec_enc_config_default(iface, &m_cfg, 0) != AOM_CODEC_OK)
        return false;

    m_cfg.g_w = width;
    m_cfg.g_h = height;
    m_cfg.g_timebase.num = 1;
    m_cfg.g_timebase.den = fps;
    m_cfg.rc_target_bitrate = 5000; // kbps
    m_cfg.g_error_resilient = 0;
    m_cfg.g_threads = 2;

    if (aom_codec_init(&m_codec, iface, &m_cfg, 0) != AOM_CODEC_OK)
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

    aom_image_t raw;
    aom_img_wrap(&raw, AOM_IMG_FMT_I420, m_width, m_height, 1, yuv.data());

    int flags = m_forceKeyframe ? AOM_EFLAG_FORCE_KF : 0;
    if (aom_codec_encode(&m_codec, &raw, m_frameCount, 1, flags) != AOM_CODEC_OK)
        return false;

    m_forceKeyframe = false;

    aom_codec_iter_t iter = nullptr;
    const aom_codec_cx_pkt_t* pkt = nullptr;
    compressedData.clear();
    while ((pkt = aom_codec_get_cx_data(&m_codec, &iter)) != nullptr) {
        if (pkt->kind == AOM_CODEC_CX_FRAME_PKT) {
            const uint8_t* buf = static_cast<const uint8_t*>(pkt->data.frame.buf);
            compressedData.insert(compressedData.end(), buf, buf + pkt->data.frame.sz);
            isKeyframe = (pkt->data.frame.flags & AOM_FRAME_IS_KEY) != 0;
        }
    }

    m_frameCount++;
    return !compressedData.empty();
}

void AV1Encoder::ForceKeyframe() { m_forceKeyframe = true; }

void AV1Encoder::Cleanup() {
    if (m_initialized) {
        aom_codec_destroy(&m_codec);
        m_initialized = false;
    }
}
