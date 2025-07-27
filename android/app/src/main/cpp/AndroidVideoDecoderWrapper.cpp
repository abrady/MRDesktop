#include "AndroidVideoDecoderWrapper.h"

AndroidVideoDecoderWrapper::AndroidVideoDecoderWrapper() 
    : m_decoder(std::make_unique<AndroidVideoDecoder>()) {
}

AndroidVideoDecoderWrapper::~AndroidVideoDecoderWrapper() {
    Cleanup();
}

bool AndroidVideoDecoderWrapper::Initialize(uint32_t width, uint32_t height, CompressionType compression) {
    return m_decoder->Initialize(width, height, compression);
}

bool AndroidVideoDecoderWrapper::DecodeFrame(const uint8_t* compressedData, size_t dataSize, std::vector<uint8_t>& decodedData) {
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