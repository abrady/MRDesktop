#pragma once
#include <asio.hpp>
#include "rtp/rtp_packet.hpp"
#include "rtp/jitter_buffer.hpp"

namespace rtp {

class Receiver {
 public:
  explicit Receiver(asio::io_context& io);
  void bind(asio::ip::udp::endpoint local);
  void start();
  JitterBuffer& jitter_buffer();

 private:
  asio::ip::udp::socket socket_;
  JitterBuffer buffer_;
};

}  // namespace rtp
