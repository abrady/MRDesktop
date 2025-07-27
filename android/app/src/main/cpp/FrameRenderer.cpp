#include "FrameRenderer.h"
#include <algorithm>
#include <android/log.h>

#define LOG_TAG "MRDesk.Render"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

FrameRenderer::FrameRenderer() {
  LOGI("FrameRenderer created");
}

FrameRenderer::~FrameRenderer() {
  LOGI("FrameRenderer destroyed");
}

bool FrameRenderer::ValidateYUVData(
    const std::vector<uint8_t>& yuvData, uint32_t width, uint32_t height) {
  // YUV420P format: Y plane + U plane + V plane
  // Y plane: width * height bytes
  // U plane: (width/2) * (height/2) bytes
  // V plane: (width/2) * (height/2) bytes
  size_t expectedSize = width * height * 3 / 2;

  // Allow some flexibility for MediaCodec padding/alignment
  // MediaCodec often adds padding bytes for memory alignment
  size_t minSize = expectedSize;
  size_t maxSize = expectedSize + 64; // Allow up to 64 bytes of padding

  if (yuvData.size() < minSize || yuvData.size() > maxSize) {
    LOGE(
        "Invalid YUV data size: got %zu, expected %zu-%zu",
        yuvData.size(),
        minSize,
        maxSize);
    return false;
  }

  if (width == 0 || height == 0 || width % 2 != 0 || height % 2 != 0) {
    LOGE(
        "Invalid dimensions: %ux%u (must be non-zero and even)", width, height);
    return false;
  }

  LOGI(
      "YUV validation passed: %ux%u, %zu bytes (expected %zu)",
      width,
      height,
      yuvData.size(),
      expectedSize);
  return true;
}

uint8_t FrameRenderer::ClampByte(int32_t value) {
  return static_cast<uint8_t>(std::max(0, std::min(255, value)));
}

void FrameRenderer::ConvertYUVPixelToRGB(
    uint8_t y, uint8_t u, uint8_t v, uint8_t& r, uint8_t& g, uint8_t& b) {
  // BT.601 YUV to RGB conversion formulas
  int32_t c = y - 16;
  int32_t d = u - 128;
  int32_t e = v - 128;

  int32_t r_val = (298 * c + 409 * e + 128) >> 8;
  int32_t g_val = (298 * c - 100 * d - 208 * e + 128) >> 8;
  int32_t b_val = (298 * c + 516 * d + 128) >> 8;

  r = ClampByte(r_val);
  g = ClampByte(g_val);
  b = ClampByte(b_val);
}

bool FrameRenderer::ConvertYUVToARGB(
    const std::vector<uint8_t>& yuvData,
    uint32_t width,
    uint32_t height,
    std::vector<uint8_t>& argbData) {
  if (!ValidateYUVData(yuvData, width, height)) {
    return false;
  }

  // Additional safety checks
  if (yuvData.empty() || width > 8192 || height > 8192) {
    LOGE("Frame dimensions too large or data empty: %ux%u", width, height);
    return false;
  }

  try {
    // Prepare output buffer: 4 bytes per pixel (ARGB)
    argbData.clear();
    argbData.resize(width * height * 4);

    const uint8_t* yPlane = yuvData.data();
    const uint8_t* uPlane = yPlane + (width * height);
    const uint8_t* vPlane = uPlane + (width * height / 4);

    // Verify plane boundaries
    const uint8_t* dataEnd = yuvData.data() + yuvData.size();
    if (vPlane + (width * height / 4) > dataEnd) {
      LOGE("YUV plane boundaries exceed data buffer");
      return false;
    }

    LOGI("Converting YUV frame %ux%u to ARGB", width, height);

    for (uint32_t row = 0; row < height; row++) {
      for (uint32_t col = 0; col < width; col++) {
        // Bounds check for Y plane access
        uint32_t yIndex = row * width + col;
        if (yIndex >= width * height) {
          LOGE("Y plane index out of bounds: %u >= %u", yIndex, width * height);
          return false;
        }

        uint8_t y = yPlane[yIndex];

        // Get U,V values (subsampled 2x2) with bounds checking
        uint32_t uvRow = row / 2;
        uint32_t uvCol = col / 2;
        uint32_t uvIndex = uvRow * (width / 2) + uvCol;
        uint32_t uvPlaneSize = (width * height) / 4;

        if (uvIndex >= uvPlaneSize) {
          LOGE("UV plane index out of bounds: %u >= %u", uvIndex, uvPlaneSize);
          return false;
        }

        uint8_t u = uPlane[uvIndex];
        uint8_t v = vPlane[uvIndex];

        // Convert to RGB
        uint8_t r, g, b;
        ConvertYUVPixelToRGB(y, u, v, r, g, b);

        // Write ARGB pixel with bounds checking
        uint32_t pixelIndex = (row * width + col) * 4;
        if (pixelIndex + 3 >= argbData.size()) {
          LOGE(
              "ARGB pixel index out of bounds: %u >= %zu",
              pixelIndex + 3,
              argbData.size());
          return false;
        }

        // Android expects BGRA ordering for Bitmap.Config.ARGB_8888
        argbData[pixelIndex + 0] = b; // Blue
        argbData[pixelIndex + 1] = g; // Green
        argbData[pixelIndex + 2] = r; // Red
        argbData[pixelIndex + 3] = 0xFF; // Alpha (fully opaque)
      }
    }

    LOGI("YUV to ARGB conversion completed successfully");
    return true;
  } catch (const std::exception& e) {
    LOGE("Exception during YUV conversion: %s", e.what());
    return false;
  } catch (...) {
    LOGE("Unknown exception during YUV conversion");
    return false;
  }
}