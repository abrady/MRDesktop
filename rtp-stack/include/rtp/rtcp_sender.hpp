#pragma once
#include <asio.hpp>
#include "rtp/rtcp_packet.hpp"

namespace rtp {

class RtcpSender {
 public:
  RtcpSender(asio::io_context& io, asio::ip::udp::endpoint remote, uint32_t ssrc);
  void send_sr(uint32_t rtp_timestamp, uint64_t ntp_time);
  void send_rr();

 private:
  asio::ip::udp::socket socket_;
  asio::ip::udp::endpoint remote_;
  uint32_t ssrc_;
};

}  // namespace rtp
