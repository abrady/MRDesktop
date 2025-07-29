#pragma once
#include <array>
#include <cstdint>
#include <span>

namespace rtp {

struct Packet {
  uint8_t version : 2 {2};
  uint8_t padding : 1 {0};
  uint8_t extension : 1 {0};
  uint8_t csrc_count : 4 {0};
  uint8_t marker : 1 {0};
  uint8_t payload_type : 7 {96};
  uint16_t sequence_number {0};
  uint32_t timestamp {0};
  uint32_t ssrc {0};
  std::span<const uint8_t> payload{};

  [[nodiscard]] std::array<uint8_t, 1500> marshal() const;
  static Packet parse(std::span<const uint8_t> data);
};

}  // namespace rtp
