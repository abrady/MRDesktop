#pragma once
#include <queue>
#include <vector>
#include "rtp/rtp_packet.hpp"

namespace rtp {

class JitterBuffer {
 public:
  explicit JitterBuffer(size_t depth_ms = 50);
  void push(const Packet& pkt);
  bool pop(Packet& pkt);

 private:
  std::priority_queue<Packet, std::vector<Packet>,
                      bool (*)(const Packet&, const Packet&)> queue_;
};

}  // namespace rtp
