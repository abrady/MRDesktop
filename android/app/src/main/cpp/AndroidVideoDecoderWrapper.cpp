#include "AndroidVideoDecoderWrapper.h"
#include <android/log.h>
#include "FrameRenderer.h"

#define LOG_TAG "MRDesk.AndroidVideoDecoderWrapper"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

AndroidVideoDecoderWrapper::AndroidVideoDecoderWrapper()
    : m_decoder(std::make_unique<AndroidVideoDecoder>()) {}

AndroidVideoDecoderWrapper::~AndroidVideoDecoderWrapper() {
  Cleanup();
}

bool AndroidVideoDecoderWrapper::Initialize(
    uint32_t width, uint32_t height, CompressionType compression) {
  return m_decoder->Initialize(width, height, compression);
}

bool AndroidVideoDecoderWrapper::DecodeFrame(
    const uint8_t* compressedData,
    size_t dataSize,
    std::vector<uint8_t>& decodedData) {
  return m_decoder->DecodeFrame(compressedData, dataSize, decodedData);
}

bool AndroidVideoDecoderWrapper::IsInitialized() const {
  return m_decoder->IsInitialized();
}

void AndroidVideoDecoderWrapper::Cleanup() {
  if (m_decoder) {
    m_decoder->Cleanup();
  }
}