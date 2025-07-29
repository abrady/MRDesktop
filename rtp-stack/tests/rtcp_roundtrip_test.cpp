#include <gtest/gtest.h>
#include "rtp/rtcp_packet.hpp"

using namespace rtp;

TEST(Rtcp, RoundTrip) {
  RtcpPacket p;
  p.type = 201;
  p.ssrc = 42;
  auto buf = p.marshal();
  auto parsed = RtcpPacket::parse(std::span<const uint8_t>(buf.data(), 8));
  EXPECT_EQ(parsed.type, 201);
  EXPECT_EQ(parsed.ssrc, 42u);
}
