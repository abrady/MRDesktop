#include "FrameUtils.h"
#include <fstream>

bool SaveFrameAsBMP(
    uint32_t width,
    uint32_t height,
    const std::vector<uint8_t>& frameData,
    const std::string& filename) {
  BMPFileHeader fileHeader{};
  BMPInfoHeader infoHeader{};

  fileHeader.bfType = 0x4D42; // "BM"
  fileHeader.bfSize =
      sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + frameData.size();
  fileHeader.bfOffBits = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);

  infoHeader.biSize = sizeof(BMPInfoHeader);
  infoHeader.biWidth = static_cast<int32_t>(width);
  infoHeader.biHeight = -static_cast<int32_t>(height);
  infoHeader.biPlanes = 1;
  infoHeader.biBitCount = 32;
  infoHeader.biCompression = 0;
  infoHeader.biSizeImage = static_cast<uint32_t>(frameData.size());

  std::ofstream out(filename, std::ios::binary);
  if (!out.is_open()) {
    return false;
  }

  out.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
  out.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));
  out.write(reinterpret_cast<const char*>(frameData.data()), frameData.size());

  return out.good();
}
