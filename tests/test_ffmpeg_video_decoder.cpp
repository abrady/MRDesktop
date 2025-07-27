#include <memory>
#include <vector>
#include <gtest/gtest.h>
#include "FFmpegVideoDecoder.h"

class FFmpegVideoDecoderTest : public ::testing::Test {
 protected:
  void SetUp() override { decoder = std::make_unique<FFmpegVideoDecoder>(); }

  void TearDown() override { decoder.reset(); }

  std::unique_ptr<FFmpegVideoDecoder> decoder;
};

TEST_F(FFmpegVideoDecoderTest, InitializeDecoder) {
  // Test basic decoder initialization
  bool initialized = decoder->Initialize(640, 480, COMPRESSION_H265);
  EXPECT_TRUE(initialized);
}

TEST_F(FFmpegVideoDecoderTest, DecodeInvalidData) {
  // Test decoding with invalid data
  bool initialized = decoder->Initialize(640, 480, COMPRESSION_H265);
  ASSERT_TRUE(initialized);

  std::vector<uint8_t> invalidData = {0xFF, 0xFF, 0xFF, 0xFF}; // Invalid data
  std::vector<uint8_t> decodedData;

  bool decoded =
      decoder->DecodeFrame(invalidData.data(), invalidData.size(), decodedData);
  EXPECT_FALSE(decoded);
}

TEST_F(FFmpegVideoDecoderTest, DecodeEmptyData) {
  // Test decoding with empty data
  bool initialized = decoder->Initialize(640, 480, COMPRESSION_H265);
  ASSERT_TRUE(initialized);

  std::vector<uint8_t> decodedData;

  bool decoded = decoder->DecodeFrame(nullptr, 0, decodedData);
  EXPECT_FALSE(decoded);
}