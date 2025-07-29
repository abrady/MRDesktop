#include "rtp/rtp_packet.hpp"
#include <cstring>

namespace rtp {

std::array<uint8_t, 1500> Packet::marshal() const {
  std::array<uint8_t, 1500> out{};
  size_t idx = 0;
  out[idx++] = (version << 6) | (padding << 5) | (extension << 4) | csrc_count;
  out[idx++] = (marker << 7) | payload_type;
  out[idx++] = sequence_number >> 8;
  out[idx++] = sequence_number & 0xFF;
  out[idx++] = timestamp >> 24;
  out[idx++] = (timestamp >> 16) & 0xFF;
  out[idx++] = (timestamp >> 8) & 0xFF;
  out[idx++] = timestamp & 0xFF;
  out[idx++] = ssrc >> 24;
  out[idx++] = (ssrc >> 16) & 0xFF;
  out[idx++] = (ssrc >> 8) & 0xFF;
  out[idx++] = ssrc & 0xFF;
  std::memcpy(out.data() + idx, payload.data(), payload.size());
  return out;
}

Packet Packet::parse(std::span<const uint8_t> data) {
  Packet pkt;
  size_t idx = 0;
  pkt.version = data[idx] >> 6;
  pkt.padding = (data[idx] >> 5) & 0x1;
  pkt.extension = (data[idx] >> 4) & 0x1;
  pkt.csrc_count = data[idx++] & 0xF;
  pkt.marker = data[idx] >> 7;
  pkt.payload_type = data[idx++] & 0x7F;
  pkt.sequence_number = (data[idx] << 8) | data[idx+1];
  idx += 2;
  pkt.timestamp = (data[idx] << 24) | (data[idx+1] << 16) |
                  (data[idx+2] << 8) | data[idx+3];
  idx += 4;
  pkt.ssrc = (data[idx] << 24) | (data[idx+1] << 16) |
             (data[idx+2] << 8) | data[idx+3];
  idx += 4;
  pkt.payload = data.subspan(idx);
  return pkt;
}

}  // namespace rtp
