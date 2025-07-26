#include "FrameLogger.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cstring>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

FrameLogger::FrameLogger(uint32_t maxFrames, const std::string& outputDir)
    : m_maxFrames(maxFrames)
    , m_outputDir(outputDir)
    , m_frameCounter(0) {
    EnsureOutputDirectory();
}

FrameLogger::~FrameLogger() {
    if (!m_loggedFrames.empty()) {
        std::cout << "FrameLogger: Auto-saving " << m_loggedFrames.size() << " frames to disk..." << std::endl;
        SaveFramesToDisk();
    }
}

bool FrameLogger::LogFrame(uint32_t width, uint32_t height, uint32_t dataSize, const uint8_t* frameData) {
    if (m_loggedFrames.size() >= m_maxFrames) {
        return false;
    }
    
    LoggedFrame frame;
    frame.frameIndex = m_frameCounter++;
    frame.width = width;
    frame.height = height;
    frame.dataSize = dataSize;
    frame.timestamp = std::chrono::high_resolution_clock::now();
    frame.filename = GenerateFrameFilename(frame.frameIndex, width, height);
    
    // Copy frame data
    frame.frameData.resize(dataSize);
    std::memcpy(frame.frameData.data(), frameData, dataSize);
    
    m_loggedFrames.push_back(std::move(frame));
    
    std::cout << "FrameLogger: Logged frame " << frame.frameIndex 
              << " (" << width << "x" << height << ", " << dataSize << " bytes)" << std::endl;
    
    return true;
}

bool FrameLogger::SaveFrameAsBMP(uint32_t width, uint32_t height, const std::vector<uint8_t>& frameData, const std::string& filename) {
#ifdef _WIN32
    // Create BMP file headers (assumes BGRA format)
    BITMAPFILEHEADER fileHeader = {};
    BITMAPINFOHEADER infoHeader = {};

    fileHeader.bfType = 0x4D42; // "BM"
    fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + frameData.size();
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = width;
    infoHeader.biHeight = -(int)height; // Top-down DIB
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 32;
    infoHeader.biCompression = BI_RGB;
    infoHeader.biSizeImage = frameData.size();

    HANDLE hFile = CreateFileA(filename.c_str(), GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD written;
        bool success = WriteFile(hFile, &fileHeader, sizeof(fileHeader), &written, nullptr) &&
                       WriteFile(hFile, &infoHeader, sizeof(infoHeader), &written, nullptr) &&
                       WriteFile(hFile, frameData.data(), frameData.size(), &written, nullptr);
        CloseHandle(hFile);
        return success;
    }
    return false;
#else
    // For non-Windows platforms, save as raw data for now
    std::ofstream file(filename, std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(frameData.data()), frameData.size());
        file.close();
        return true;
    }
    return false;
#endif
}

void FrameLogger::SaveFramesToDisk() {
    if (m_loggedFrames.empty()) {
        std::cout << "FrameLogger: No frames to save" << std::endl;
        return;
    }
    
    EnsureOutputDirectory();
    
    // Save frame data files as BMP
    for (const auto& frame : m_loggedFrames) {
        std::string framePath = m_outputDir + "/" + frame.filename;
        if (SaveFrameAsBMP(frame.width, frame.height, frame.frameData, framePath)) {
            std::cout << "FrameLogger: Saved " << framePath << std::endl;
        } else {
            std::cerr << "FrameLogger: Failed to save " << framePath << std::endl;
        }
    }
    
    // Save metadata file
    std::string metadataPath = m_outputDir + "/frame_metadata.txt";
    std::ofstream metaFile(metadataPath);
    if (metaFile.is_open()) {
        metaFile << "Frame Debug Log - " << m_loggedFrames.size() << " frames captured\n";
        metaFile << "Generated: " << std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count() << "ms\n\n";
        
        metaFile << std::left << std::setw(8) << "Index" 
                 << std::setw(12) << "Width" 
                 << std::setw(12) << "Height" 
                 << std::setw(12) << "DataSize" 
                 << std::setw(20) << "Filename" 
                 << "Timestamp\n";
        metaFile << std::string(80, '-') << "\n";
        
        for (const auto& frame : m_loggedFrames) {
            auto timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                frame.timestamp.time_since_epoch()).count();
            
            metaFile << std::left << std::setw(8) << frame.frameIndex
                     << std::setw(12) << frame.width
                     << std::setw(12) << frame.height  
                     << std::setw(12) << frame.dataSize
                     << std::setw(20) << frame.filename
                     << timeMs << "ms\n";
        }
        
        metaFile.close();
        std::cout << "FrameLogger: Saved metadata to " << metadataPath << std::endl;
    }
}

void FrameLogger::PrintFrameStats() const {
    if (m_loggedFrames.empty()) {
        std::cout << "FrameLogger: No frames logged" << std::endl;
        return;
    }
    
    std::cout << "\n=== Frame Logger Statistics ===" << std::endl;
    std::cout << "Total frames logged: " << m_loggedFrames.size() << "/" << m_maxFrames << std::endl;
    
    // Calculate stats
    uint32_t totalBytes = 0;
    uint32_t minWidth = UINT32_MAX, maxWidth = 0;
    uint32_t minHeight = UINT32_MAX, maxHeight = 0;
    
    for (const auto& frame : m_loggedFrames) {
        totalBytes += frame.dataSize;
        minWidth = std::min(minWidth, frame.width);
        maxWidth = std::max(maxWidth, frame.width);
        minHeight = std::min(minHeight, frame.height);
        maxHeight = std::max(maxHeight, frame.height);
    }
    
    std::cout << "Total data size: " << totalBytes << " bytes (" 
              << (totalBytes / 1024.0f / 1024.0f) << " MB)" << std::endl;
    std::cout << "Resolution range: " << minWidth << "x" << minHeight 
              << " to " << maxWidth << "x" << maxHeight << std::endl;
    std::cout << "Output directory: " << m_outputDir << std::endl;
    std::cout << "==============================\n" << std::endl;
}

void FrameLogger::Clear() {
    m_loggedFrames.clear();
    m_frameCounter = 0;
    std::cout << "FrameLogger: Cleared all logged frames" << std::endl;
}

void FrameLogger::EnsureOutputDirectory() {
    try {
        if (!std::filesystem::exists(m_outputDir)) {
            std::filesystem::create_directories(m_outputDir);
            std::cout << "FrameLogger: Created output directory: " << m_outputDir << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "FrameLogger: Failed to create output directory: " << e.what() << std::endl;
    }
}

std::string FrameLogger::GenerateFrameFilename(uint32_t frameIndex, uint32_t width, uint32_t height) {
    std::ostringstream oss;
    oss << "frame_" << std::setfill('0') << std::setw(3) << frameIndex 
        << "_" << width << "x" << height << ".bmp";
    return oss.str();
}