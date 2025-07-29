#include "rtp/rtp_sender.hpp"

namespace rtp {

Sender::Sender(asio::io_context& io, asio::ip::udp::endpoint remote, uint32_t ssrc)
    : socket_(io), remote_(remote), ssrc_(ssrc) {
  socket_.open(asio::ip::udp::v4());
}

void Sender::push_frame(std::span<const uint8_t> encoded_frame, bool last_of_frame) {
  Packet pkt;
  pkt.sequence_number = next_seq_++;
  pkt.timestamp = timestamp_;
  pkt.ssrc = ssrc_;
  pkt.marker = last_of_frame ? 1 : 0;
  pkt.payload = encoded_frame;
  auto buf = pkt.marshal();
  socket_.send_to(asio::buffer(buf.data(), encoded_frame.size() + 12), remote_);
}

void Sender::advance_timestamp(uint32_t samples) {
  timestamp_ += samples;
}

}  // namespace rtp
