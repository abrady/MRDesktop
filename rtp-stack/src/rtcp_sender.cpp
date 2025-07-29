#include "rtp/rtcp_sender.hpp"

namespace rtp {

RtcpSender::RtcpSender(asio::io_context& io, asio::ip::udp::endpoint remote,
                       uint32_t ssrc)
    : socket_(io), remote_(remote), ssrc_(ssrc) {
  socket_.open(asio::ip::udp::v4());
}

void RtcpSender::send_sr(uint32_t, uint64_t) {
  RtcpPacket p;
  p.ssrc = ssrc_;
  auto buf = p.marshal();
  socket_.send_to(asio::buffer(buf.data(), 8), remote_);
}

void RtcpSender::send_rr() {
  RtcpPacket p;
  p.type = 201;
  p.ssrc = ssrc_;
  auto buf = p.marshal();
  socket_.send_to(asio::buffer(buf.data(), 8), remote_);
}

}  // namespace rtp
