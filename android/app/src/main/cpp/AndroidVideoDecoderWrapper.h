#pragma once

#include "IVideoDecoder.h"
#include "AndroidVideoDecoder.h"
#include <memory>

/**
 * Wrapper around AndroidVideoDecoder to implement IVideoDecoder interface
 */
class AndroidVideoDecoderWrapper : public IVideoDecoder {
private:
    std::unique_ptr<AndroidVideoDecoder> m_decoder;

public:
    AndroidVideoDecoderWrapper();
    ~AndroidVideoDecoderWrapper() override;

    bool Initialize(uint32_t width, uint32_t height, CompressionType compression) override;
    bool DecodeFrame(const uint8_t* compressedData, size_t dataSize, std::vector<uint8_t>& decodedData) override;
    bool IsInitialized() const override;
    void Cleanup() override;
};