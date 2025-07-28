#include <cstring>
#include <gtest/gtest.h>
#include "protocol.h"

class ProtocolTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Setup code here
  }

  void TearDown() override {
    // Cleanup code here
  }
};

TEST_F(ProtocolTest, MessageTypesAreValid) {
  // Test that message types are within expected ranges
  EXPECT_GE(MSG_FRAME_DATA, 0);
  EXPECT_GE(MSG_MOUSE_MOVE, 0);
  EXPECT_GE(MSG_MOUSE_CLICK, 0);
  EXPECT_GE(MSG_MOUSE_SCROLL, 0);

  // Ensure message types are distinct
  EXPECT_NE(MSG_FRAME_DATA, MSG_MOUSE_MOVE);
  EXPECT_NE(MSG_FRAME_DATA, MSG_MOUSE_CLICK);
  EXPECT_NE(MSG_FRAME_DATA, MSG_MOUSE_SCROLL);
  EXPECT_NE(MSG_MOUSE_MOVE, MSG_MOUSE_CLICK);
  EXPECT_NE(MSG_MOUSE_MOVE, MSG_MOUSE_SCROLL);
  EXPECT_NE(MSG_MOUSE_CLICK, MSG_MOUSE_SCROLL);
}

TEST_F(ProtocolTest, FrameDataMessageSize) {
  FrameMessage msg;
  msg.header.type = MSG_FRAME_DATA;
  msg.header.size =
      sizeof(msg.width) + sizeof(msg.height) + sizeof(msg.dataSize);
  msg.width = 1920;
  msg.height = 1080;
  msg.dataSize = 1920 * 1080 * 4; // RGBA

  EXPECT_EQ(msg.header.type, MSG_FRAME_DATA);
  EXPECT_GT(msg.header.size, 0);
  EXPECT_EQ(msg.width, 1920);
  EXPECT_EQ(msg.height, 1080);
  EXPECT_EQ(msg.dataSize, 1920 * 1080 * 4);
}

TEST_F(ProtocolTest, MouseMoveMessage) {
  MouseMoveMessage msg;
  msg.header.type = MSG_MOUSE_MOVE;
  msg.header.size = sizeof(msg.x) + sizeof(msg.y);
  msg.x = 100;
  msg.y = 200;

  EXPECT_EQ(msg.header.type, MSG_MOUSE_MOVE);
  EXPECT_EQ(msg.x, 100);
  EXPECT_EQ(msg.y, 200);
}

TEST_F(ProtocolTest, MouseClickMessage) {
  MouseClickMessage msg;
  msg.header.type = MSG_MOUSE_CLICK;
  msg.header.size = sizeof(msg.button) + sizeof(msg.pressed);
  msg.button = MouseClickMessage::MouseButton::LEFT_BUTTON;
  msg.pressed = true;

  EXPECT_EQ(msg.header.type, MSG_MOUSE_CLICK);
  EXPECT_EQ(msg.button, MouseClickMessage::LEFT_BUTTON);
  EXPECT_TRUE(msg.pressed);
}

TEST_F(ProtocolTest, MouseScrollMessage) {
  MouseScrollMessage msg;
  msg.header.type = MSG_MOUSE_SCROLL;
  msg.header.size = sizeof(msg.deltaX) + sizeof(msg.deltaY);
  msg.deltaX = 0;
  msg.deltaY = 120; // Scroll up

  EXPECT_EQ(msg.header.type, MSG_MOUSE_SCROLL);
  EXPECT_EQ(msg.deltaX, 0);
  EXPECT_EQ(msg.deltaY, 120);
}