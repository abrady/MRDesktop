#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <gtest/gtest.h>
#include "AndroidVideoDecoder.h"

// Asset manager provided by Java via JNI
AAssetManager* g_assetManager = nullptr;

extern "C" JNIEXPORT void JNICALL
Java_com_mrdesktop_NativeBridge_setAssetManager(
    JNIEnv* env, jclass, jobject mgr) {
  g_assetManager = AAssetManager_fromJava(env, mgr);
}

static uint32_t CalculateChecksum(const std::vector<uint8_t>& data) {
  uint32_t crc = 0;
  for (uint8_t b : data) {
    crc = crc * 31 + b;
  }
  return crc;
}

TEST(HevcDecode, AllFramesMatch) {
  ASSERT_NE(g_assetManager, nullptr);
  AAsset* asset =
      AAssetManager_open(g_assetManager, "frame.h265", AASSET_MODE_BUFFER);
  ASSERT_NE(asset, nullptr);
  size_t size = AAsset_getLength(asset);
  std::vector<uint8_t> compressed(size);
  AAsset_read(asset, compressed.data(), size);
  AAsset_close(asset);

  AndroidVideoDecoder decoder;
  ASSERT_TRUE(decoder.Initialize(1280, 720, COMPRESSION_H265));

  std::vector<uint8_t> rgba;
  ASSERT_TRUE(decoder.DecodeFrame(compressed.data(), compressed.size(), rgba));

  uint32_t checksum = CalculateChecksum(rgba);
  EXPECT_EQ(0x12345678u, checksum);
}
