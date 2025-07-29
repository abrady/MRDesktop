#include <gtest/gtest.h>
#include "rtp/rtp_packet.hpp"

using namespace rtp;

TEST(RtpPacket, MarshalParseRoundTrip) {
  std::array<uint8_t, 4> payload = {1,2,3,4};
  Packet p;
  p.sequence_number = 123;
  p.timestamp = 456;
  p.ssrc = 789;
  p.payload = payload;

  auto data = p.marshal();
  auto parsed = Packet::parse(std::span<const uint8_t>(data.data(), 16));

  EXPECT_EQ(parsed.sequence_number, p.sequence_number);
  EXPECT_EQ(parsed.timestamp, p.timestamp);
  EXPECT_EQ(parsed.ssrc, p.ssrc);
}
