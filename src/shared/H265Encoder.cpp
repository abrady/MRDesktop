#include "H265Encoder.h"
#include <cstring>

H265Encoder::H265Encoder()
    : m_param(nullptr), m_picture(nullptr), m_encoder(nullptr),
      m_width(0), m_height(0), m_fps(60), m_frameCount(0),
      m_initialized(false), m_forceKeyframe(false) {}

H265Encoder::~H265Encoder() { Cleanup(); }

bool H265Encoder::Initialize(int width, int height, int fps) {
    if (m_initialized) return true;

    m_width = width;
    m_height = height;
    m_fps = fps;
    m_frameCount = 0;
    m_forceKeyframe = false;

    m_param = x265_param_alloc();
    if (!m_param) return false;

    x265_param_default_preset(m_param, "ultrafast", "zerolatency");
    m_param->sourceWidth = width;
    m_param->sourceHeight = height;
    m_param->fpsNum = fps;
    m_param->fpsDen = 1;
    m_param->internalCsp = X265_CSP_I420;

    m_encoder = x265_encoder_open(m_param);
    if (!m_encoder) return false;

    m_picture = x265_picture_alloc();
    x265_picture_init(m_param, m_picture);
    m_picture->colorSpace = X265_CSP_I420;
    m_picture->stride[0] = width;
    m_picture->stride[1] = width/2;
    m_picture->stride[2] = width/2;

    m_initialized = true;
    return true;
}

static void BGRAtoI420(const uint8_t* src, int width, int height, std::vector<uint8_t>& yuv) {
    int frameSize = width * height;
    yuv.resize(frameSize * 3 / 2);
    uint8_t* yPlane = yuv.data();
    uint8_t* uPlane = yPlane + frameSize;
    uint8_t* vPlane = uPlane + frameSize/4;
    for (int y=0; y<height; ++y) {
        for (int x=0; x<width; ++x) {
            const uint8_t* px = src + (y*width + x)*4;
            uint8_t b = px[0];
            uint8_t g = px[1];
            uint8_t r = px[2];
            int Y = int(0.2126*r + 0.7152*g + 0.0722*b);
            yPlane[y*width + x] = (uint8_t)Y;
            if ((y%2==0) && (x%2==0)) {
                int U = int(-0.1146*r -0.3854*g +0.5*b +128);
                int V = int(0.5*r -0.4542*g -0.0458*b +128);
                int idx = (y/2)*(width/2) + (x/2);
                uPlane[idx] = (uint8_t)U;
                vPlane[idx] = (uint8_t)V;
            }
        }
    }
}

bool H265Encoder::EncodeFrame(const uint8_t* frameData, std::vector<uint8_t>& compressedData, bool& isKeyframe) {
    if (!m_initialized) return false;

    std::vector<uint8_t> yuv;
    BGRAtoI420(frameData, m_width, m_height, yuv);

    m_picture->planes[0] = yuv.data();
    m_picture->planes[1] = yuv.data() + m_width*m_height;
    m_picture->planes[2] = yuv.data() + m_width*m_height + m_width*m_height/4;
    m_picture->pts = m_frameCount;
    if (m_forceKeyframe || m_frameCount==0) {
        m_picture->sliceType = X265_TYPE_IDR;
        m_forceKeyframe = false;
    } else {
        m_picture->sliceType = X265_TYPE_AUTO;
    }

    x265_nal* nals = nullptr;
    uint32_t nalCount = 0;
    int ret = x265_encoder_encode(m_encoder, &nals, &nalCount, m_picture, nullptr);
    if (ret < 0) return false;

    compressedData.clear();
    for (uint32_t i=0; i<nalCount; ++i) {
        compressedData.insert(compressedData.end(), nals[i].payload, nals[i].payload+nals[i].sizeBytes);
    }
    isKeyframe = (m_picture->sliceType == X265_TYPE_IDR);

    m_frameCount++;
    return !compressedData.empty();
}

void H265Encoder::ForceKeyframe() { m_forceKeyframe = true; }

void H265Encoder::Cleanup() {
    if (m_encoder) {
        x265_encoder_close(m_encoder);
        m_encoder = nullptr;
    }
    if (m_picture) {
        x265_picture_free(m_picture);
        m_picture = nullptr;
    }
    if (m_param) {
        x265_param_free(m_param);
        m_param = nullptr;
    }
    m_initialized = false;
}
