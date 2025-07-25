#include "H265Decoder.h"
#include <iostream>

H265Decoder::H265Decoder() : m_ctx(nullptr), m_frame(nullptr), m_packet(nullptr) {}

H265Decoder::~H265Decoder() { Cleanup(); }

bool H265Decoder::Initialize() {
    const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_HEVC);
    if (!codec) return false;

    m_ctx = avcodec_alloc_context3(codec);
    if (!m_ctx) return false;

    if (avcodec_open2(m_ctx, codec, nullptr) < 0) {
        avcodec_free_context(&m_ctx);
        return false;
    }

    m_frame = av_frame_alloc();
    m_packet = av_packet_alloc();
    return m_frame && m_packet;
}

bool H265Decoder::DecodeFrame(const uint8_t* data, size_t size, std::vector<uint8_t>& bgra, int& width, int& height, bool& isKeyframe) {
    if (!m_ctx) return false;

    av_packet_unref(m_packet);
    if (av_new_packet(m_packet, (int)size) < 0) return false;
    memcpy(m_packet->data, data, size);

    if (avcodec_send_packet(m_ctx, m_packet) < 0) return false;
    int ret = avcodec_receive_frame(m_ctx, m_frame);
    if (ret < 0) return false;

    width = m_frame->width;
    height = m_frame->height;
    isKeyframe = m_frame->key_frame != 0;

    SwsContext* sws = sws_getContext(width, height, (AVPixelFormat)m_frame->format,
                                     width, height, AV_PIX_FMT_BGRA, SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!sws) return false;

    bgra.resize(width * height * 4);
    uint8_t* dest[4] = { bgra.data(), nullptr, nullptr, nullptr };
    int destLinesize[4] = { width * 4, 0, 0, 0 };
    sws_scale(sws, m_frame->data, m_frame->linesize, 0, height, dest, destLinesize);
    sws_freeContext(sws);

    return true;
}

void H265Decoder::Cleanup() {
    if (m_packet) {
        av_packet_free(&m_packet);
        m_packet = nullptr;
    }
    if (m_frame) {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }
    if (m_ctx) {
        avcodec_free_context(&m_ctx);
        m_ctx = nullptr;
    }
}
