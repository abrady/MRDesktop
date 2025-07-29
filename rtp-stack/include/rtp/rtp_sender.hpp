#pragma once
#include <asio.hpp>
#include "rtp/rtp_packet.hpp"

namespace rtp {

class Sender {
 public:
  Sender(asio::io_context& io, asio::ip::udp::endpoint remote, uint32_t ssrc);
  void push_frame(std::span<const uint8_t> encoded_frame, bool last_of_frame);
  void advance_timestamp(uint32_t samples);

 private:
  asio::ip::udp::socket socket_;
  asio::ip::udp::endpoint remote_;
  uint16_t next_seq_ = 0;
  uint32_t timestamp_ = 0;
  uint32_t ssrc_;
};

}  // namespace rtp
