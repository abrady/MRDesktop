#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <chrono>

struct LoggedFrame {
    uint32_t frameIndex;
    uint32_t width;
    uint32_t height;
    uint32_t dataSize;
    std::vector<uint8_t> frameData;
    std::chrono::high_resolution_clock::time_point timestamp;
    std::string filename;
};

class FrameLogger {
public:
    FrameLogger(uint32_t maxFrames = 5, const std::string& outputDir = "debug_frames");
    ~FrameLogger();

    // Log a frame (returns true if frame was logged, false if max frames reached)
    bool LogFrame(uint32_t width, uint32_t height, uint32_t dataSize, const uint8_t* frameData);
    
    // Save all logged frames to disk
    void SaveFramesToDisk();
    
    // Get statistics about logged frames
    void PrintFrameStats() const;
    
    // Clear all logged frames
    void Clear();
    
    // Check if logging is still active
    bool IsLogging() const { return m_loggedFrames.size() < m_maxFrames; }
    
    // Get number of frames logged
    uint32_t GetFrameCount() const { return static_cast<uint32_t>(m_loggedFrames.size()); }

private:
    uint32_t m_maxFrames;
    std::string m_outputDir;
    std::vector<LoggedFrame> m_loggedFrames;
    uint32_t m_frameCounter;
    
    void EnsureOutputDirectory();
    std::string GenerateFrameFilename(uint32_t frameIndex, uint32_t width, uint32_t height);
};