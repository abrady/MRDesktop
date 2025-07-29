#include "rtp/rtcp_packet.hpp"
#include <cstring>

namespace rtp {

std::array<uint8_t, 1500> RtcpPacket::marshal() const {
  std::array<uint8_t, 1500> out{};
  out[0] = (version << 6) | (padding << 5) | rc;
  out[1] = type;
  out[2] = length >> 8;
  out[3] = length & 0xFF;
  out[4] = ssrc >> 24;
  out[5] = (ssrc >> 16) & 0xFF;
  out[6] = (ssrc >> 8) & 0xFF;
  out[7] = ssrc & 0xFF;
  return out;
}

RtcpPacket RtcpPacket::parse(std::span<const uint8_t> data) {
  RtcpPacket p;
  p.version = data[0] >> 6;
  p.padding = (data[0] >> 5) & 1;
  p.rc = data[0] & 0x1F;
  p.type = data[1];
  p.length = (data[2] << 8) | data[3];
  p.ssrc = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];
  return p;
}

}  // namespace rtp
