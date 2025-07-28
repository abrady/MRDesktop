#pragma once
#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>
#include <vector>
#include "protocol.h"

// Generic helper to read bytes until the requested size has been read.
// The callback should mimic the `recv` function and return the number of
// bytes read, 0 if no data is available yet, or a negative value on error.
inline bool ReadExact(
    const std::function<int(uint8_t*, int)>& recvFunc,
    uint8_t* buffer,
    int size) {
  int total = 0;
  while (total < size) {
    int r = recvFunc(buffer + total, size - total);
    if (r < 0) {
      return false;
    }
    if (r == 0) {
      // Would block - small sleep to avoid busy wait
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }
    total += r;
  }
  return true;
}

// Generic helper to read a full frame using a provided receive callback.
// The callback should mimic the `recv` function as described above.
inline bool ReadFrameGeneric(
    const std::function<int(uint8_t*, int)>& recvFunc,
    FrameMessage& frameMsg,
    std::vector<uint8_t>& frameData) {
  // Read basic header
  MessageHeader hdr;
  if (!ReadExact(recvFunc, reinterpret_cast<uint8_t*>(&hdr), sizeof(hdr)))
    return false;

  if (hdr.type == MSG_COMPRESSED_FRAME) {
    // Handle compressed frames - use separate CompressedFrameMessage variable
    CompressedFrameMessage compFrameMsg;
    compFrameMsg.header = hdr;
    int remaining = sizeof(CompressedFrameMessage) - sizeof(MessageHeader);
    if (!ReadExact(
            recvFunc,
            reinterpret_cast<uint8_t*>(&compFrameMsg) + sizeof(MessageHeader),
            remaining))
      return false;

    // Copy compatible fields to frameMsg for callers expecting FrameMessage
    frameMsg.header = compFrameMsg.header;
    frameMsg.width = compFrameMsg.width;
    frameMsg.height = compFrameMsg.height;
    frameMsg.dataSize = compFrameMsg.compressedSize;

    // Read compressed data
    frameData.resize(compFrameMsg.compressedSize);
    if (!ReadExact(
            recvFunc,
            frameData.data(),
            static_cast<int>(compFrameMsg.compressedSize)))
      return false;
    return true; // Compressed frame successfully read
  }

  if (hdr.type != MSG_FRAME_DATA)
    return false;

  // Read rest of FrameMessage
  frameMsg.header = hdr;
  int remaining = sizeof(FrameMessage) - sizeof(MessageHeader);
  if (!ReadExact(
          recvFunc,
          reinterpret_cast<uint8_t*>(&frameMsg) + sizeof(MessageHeader),
          remaining))
    return false;

  if (frameMsg.width == 0 || frameMsg.height == 0 || frameMsg.width > 10000 ||
      frameMsg.height > 10000 || frameMsg.dataSize > 100000000) {
    return false;
  }

  frameData.resize(frameMsg.dataSize);
  if (!ReadExact(
          recvFunc, frameData.data(), static_cast<int>(frameMsg.dataSize)))
    return false;
  return true;
}

#pragma pack(push, 1)
struct BMPFileHeader {
  uint16_t bfType{0};
  uint32_t bfSize{0};
  uint16_t bfReserved1{0};
  uint16_t bfReserved2{0};
  uint32_t bfOffBits{0};
};

struct BMPInfoHeader {
  uint32_t biSize{0};
  int32_t biWidth{0};
  int32_t biHeight{0};
  uint16_t biPlanes{1};
  uint16_t biBitCount{0};
  uint32_t biCompression{0};
  uint32_t biSizeImage{0};
  int32_t biXPelsPerMeter{0};
  int32_t biYPelsPerMeter{0};
  uint32_t biClrUsed{0};
  uint32_t biClrImportant{0};
};
#pragma pack(pop)

bool SaveFrameAsBMP(
    uint32_t width,
    uint32_t height,
    const std::vector<uint8_t>& frameData,
    const std::string& filename);
