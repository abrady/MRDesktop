#pragma once

#include <atomic>
#include <vector>
#include <cstdint>

class CrashSafeFrameHandler {
private:
    static std::atomic<bool> s_frameRenderingEnabled;
    static std::atomic<int> s_consecutiveCrashes;
    static const int MAX_CRASHES = 3;
    
public:
    static bool IsFrameRenderingEnabled() {
        return s_frameRenderingEnabled.load();
    }
    
    static void DisableFrameRendering() {
        s_frameRenderingEnabled.store(false);
        s_consecutiveCrashes.fetch_add(1);
    }
    
    static void ResetCrashCounter() {
        s_consecutiveCrashes.store(0);
    }
    
    static bool ShouldSkipFrameProcessing() {
        return s_consecutiveCrashes.load() >= MAX_CRASHES;
    }
    
    static void LogFrameInfo(uint32_t width, uint32_t height, size_t dataSize);
};