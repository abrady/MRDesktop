#include "rtp/rtp_receiver.hpp"

namespace rtp {

Receiver::Receiver(asio::io_context& io) : socket_(io) {}

void Receiver::bind(asio::ip::udp::endpoint local) {
  socket_.open(local.protocol());
  socket_.bind(local);
}

void Receiver::start() {
  std::array<uint8_t, 1500> buffer{};
  socket_.async_receive(asio::buffer(buffer),
      [this, buffer](std::error_code ec, std::size_t len) mutable {
        if (!ec) {
          auto pkt = Packet::parse(std::span{buffer.data(), len});
          buffer_.push(pkt);
          start();
        }
      });
}

JitterBuffer& Receiver::jitter_buffer() { return buffer_; }

}  // namespace rtp
