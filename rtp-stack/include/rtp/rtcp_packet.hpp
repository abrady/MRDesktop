#pragma once
#include <array>
#include <cstdint>
#include <span>

namespace rtp {

struct RtcpPacket {
  uint8_t version : 2 {2};
  uint8_t padding : 1 {0};
  uint8_t rc : 5 {0};
  uint8_t type {200};
  uint16_t length {0};
  uint32_t ssrc {0};

  [[nodiscard]] std::array<uint8_t, 1500> marshal() const;
  static RtcpPacket parse(std::span<const uint8_t> data);
};

}  // namespace rtp
