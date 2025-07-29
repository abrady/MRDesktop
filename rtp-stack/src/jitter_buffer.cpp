#include "rtp/jitter_buffer.hpp"

namespace rtp {

static bool packet_less(const Packet& a, const Packet& b) {
  return a.sequence_number > b.sequence_number; // for min-heap
}

JitterBuffer::JitterBuffer(size_t) : queue_(packet_less) {}

void JitterBuffer::push(const Packet& pkt) {
  queue_.push(pkt);
}

bool JitterBuffer::pop(Packet& pkt) {
  if (queue_.empty()) return false;
  pkt = queue_.top();
  queue_.pop();
  return true;
}

}  // namespace rtp
