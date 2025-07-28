#include <gtest/gtest.h>
#include "FrameLogger.h"

class FrameLoggerTest : public ::testing::Test {};

TEST_F(FrameLoggerTest, LogsValidFrame) {
    FrameLogger logger(5, "");
    uint32_t width = 640;
    uint32_t height = 480;
    std::vector<uint8_t> data(width * height * 4, 0);
    EXPECT_TRUE(logger.LogFrame(width, height, data.size(), data.data()));
    EXPECT_EQ(logger.GetFrameCount(), 1u);
}

TEST_F(FrameLoggerTest, RejectsZeroDimensions) {
    FrameLogger logger;
    std::vector<uint8_t> data(4, 0);
    EXPECT_FALSE(logger.LogFrame(0, 480, data.size(), data.data()));
    EXPECT_EQ(logger.GetFrameCount(), 0u);
}

TEST_F(FrameLoggerTest, RejectsTooLargeDimensions) {
    FrameLogger logger;
    std::vector<uint8_t> data(4, 0);
    EXPECT_FALSE(logger.LogFrame(10001, 1, data.size(), data.data()));
    EXPECT_EQ(logger.GetFrameCount(), 0u);
}
