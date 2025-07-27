#include "CrashSafeFrameHandler.h"
#include <android/log.h>

#define LOG_TAG "CrashSafeFrameHandler"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

std::atomic<bool> CrashSafeFrameHandler::s_frameRenderingEnabled{true};
std::atomic<int> CrashSafeFrameHandler::s_consecutiveCrashes{0};

void CrashSafeFrameHandler::LogFrameInfo(uint32_t width, uint32_t height, size_t dataSize) {
    LOGI("Frame received: %ux%u, %zu bytes, rendering %s", 
         width, height, dataSize, 
         s_frameRenderingEnabled.load() ? "enabled" : "disabled");
         
    if (s_consecutiveCrashes.load() > 0) {
        LOGE("Previous crashes detected: %d", s_consecutiveCrashes.load());
    }
}