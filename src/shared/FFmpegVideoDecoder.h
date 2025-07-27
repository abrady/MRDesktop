#pragma once

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning( \
    disable : 4244) // Disable conversion warnings from FFmpeg headers
#endif

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <memory>
#include <vector>
#include "protocol.h"
#include "IVideoDecoder.h"

/**
 * FFmpeg-based video decoder for desktop platforms (Windows, Linux, macOS).
 * Uses software decoding with FFmpeg libraries.
 */
class FFmpegVideoDecoder : public IVideoDecoder {
 private:
  AVCodecContext* m_CodecContext = nullptr;
  AVFrame* m_Frame = nullptr;
  AVPacket* m_Packet = nullptr;
  SwsContext* m_SwsContext = nullptr;

  uint32_t m_Width = 0;
  uint32_t m_Height = 0;
  CompressionType m_CompressionType = COMPRESSION_NONE;
  bool m_IsInitialized = false;

  const char* GetCodecName(CompressionType type);

 public:
  FFmpegVideoDecoder();
  ~FFmpegVideoDecoder();

  bool Initialize(uint32_t width, uint32_t height, CompressionType compression) override;
  bool DecodeFrame(
      const uint8_t* compressedData,
      size_t dataSize,
      std::vector<uint8_t>& bgraData) override;
  void Cleanup() override;

  uint32_t GetWidth() const { return m_Width; }
  uint32_t GetHeight() const { return m_Height; }
  CompressionType GetCompressionType() const { return m_CompressionType; }
  bool IsInitialized() const override { return m_IsInitialized; }
};