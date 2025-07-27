#include <memory>
#include <vector>
#include <gtest/gtest.h>
#include "VideoEncoder.h"

class VideoEncoderTest : public ::testing::Test {
 protected:
  void SetUp() override { encoder = std::make_unique<VideoEncoder>(); }

  void TearDown() override { encoder.reset(); }

  std::unique_ptr<VideoEncoder> encoder;
};

TEST_F(VideoEncoderTest, InitializeEncoder) {
  // Test basic encoder initialization
  bool initialized = encoder->Initialize(1920, 1080, COMPRESSION_H265);
  EXPECT_TRUE(initialized);
}

TEST_F(VideoEncoderTest, InitializeWithInvalidDimensions) {
  // Test initialization with invalid dimensions
  bool initialized = encoder->Initialize(0, 0, COMPRESSION_H265);
  EXPECT_FALSE(initialized);
}

TEST_F(VideoEncoderTest, EncodeFrameWithoutInitialization) {
  // Test encoding without initialization
  std::vector<uint8_t> frameData(1920 * 1080 * 4, 0); // RGBA data
  std::vector<uint8_t> encodedData;
  bool isKeyframe = false;

  bool encoded =
      encoder->EncodeFrame(frameData.data(), encodedData, isKeyframe);
  EXPECT_FALSE(encoded); // Should fail without initialization
}

TEST_F(VideoEncoderTest, EncodeValidFrame) {
  // Test encoding a valid frame
  bool initialized = encoder->Initialize(640, 480, COMPRESSION_H265);
  ASSERT_TRUE(initialized);

  std::vector<uint8_t> frameData(640 * 480 * 4, 128); // Gray RGBA data
  std::vector<uint8_t> encodedData;
  bool isKeyframe = false;

  bool encoded =
      encoder->EncodeFrame(frameData.data(), encodedData, isKeyframe);
  EXPECT_TRUE(encoded);
  EXPECT_GT(encodedData.size(), 0);
}