#include <gtest/gtest.h>
#include <asio.hpp>
#include "rtp/rtp_sender.hpp"
#include "rtp/rtp_receiver.hpp"

using namespace rtp;

class AsioEnv : public ::testing::Environment {
 public:
  void SetUp() override { th_ = std::thread([this]{ io.run(); }); }
  void TearDown() override { io.stop(); th_.join(); }
  asio::io_context io;
 private:
  std::thread th_;
};

static AsioEnv* env;

TEST(EndToEnd, Basic) {
  asio::ip::udp::endpoint src(asio::ip::make_address("127.0.0.1"), 9000);
  asio::ip::udp::endpoint dst(asio::ip::make_address("127.0.0.1"), 9001);
  Sender sender(env->io, dst, 1);
  Receiver receiver(env->io);
  receiver.bind(dst);
  receiver.start();

  std::array<uint8_t, 3> frame{1,2,3};
  sender.push_frame(frame, true);

  env->io.run_for(std::chrono::milliseconds(10));
  Packet out;
  ASSERT_TRUE(receiver.jitter_buffer().pop(out));
  EXPECT_EQ(out.payload.size(), frame.size());
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  env = static_cast<AsioEnv*>(::testing::AddGlobalTestEnvironment(new AsioEnv));
  return RUN_ALL_TESTS();
}
