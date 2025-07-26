#include <iostream>
#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <conio.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#endif
#include <vector>
#include <thread>
#include <chrono>
#include <string>
#include <algorithm>
#include "protocol.h"
#include "../shared/FrameLogger.h"
#include "../shared/NetworkReceiver.h"


#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t bfType{0};
    uint32_t bfSize{0};
    uint16_t bfReserved1{0};
    uint16_t bfReserved2{0};
    uint32_t bfOffBits{0};
};
struct BMPInfoHeader {
    uint32_t biSize{0};
    int32_t  biWidth{0};
    int32_t  biHeight{0};
    uint16_t biPlanes{1};
    uint16_t biBitCount{0};
    uint32_t biCompression{0};
    uint32_t biSizeImage{0};
    int32_t  biXPelsPerMeter{0};
    int32_t  biYPelsPerMeter{0};
    uint32_t biClrUsed{0};
    uint32_t biClrImportant{0};
};
#pragma pack(pop)

#ifndef _WIN32
static int _kbhit()
{
    int bytes = 0;
    ioctl(STDIN_FILENO, FIONREAD, &bytes);
    return bytes;
}

static int _getch()
{
    termios oldt{};
    tcgetattr(STDIN_FILENO, &oldt);
    termios newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    int ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}
#endif

void SaveFrameAsBMP(const FrameMessage &frameMsg, const std::vector<uint8_t> &frameData, const std::string &filename)
{
    BMPFileHeader fileHeader{};
    BMPInfoHeader infoHeader{};

    fileHeader.bfType = 0x4D42; // "BM"
    fileHeader.bfSize = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + frameData.size();
    fileHeader.bfOffBits = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);

    infoHeader.biSize = sizeof(BMPInfoHeader);
    infoHeader.biWidth = static_cast<int32_t>(frameMsg.width);
    infoHeader.biHeight = -static_cast<int32_t>(frameMsg.height); // top-down
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 32;
    infoHeader.biCompression = 0; // BI_RGB
    infoHeader.biSizeImage = static_cast<uint32_t>(frameData.size());

#ifdef _WIN32
    HANDLE hFile = CreateFileA(filename.c_str(), GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD written;
        WriteFile(hFile, &fileHeader, sizeof(fileHeader), &written, nullptr);
        WriteFile(hFile, &infoHeader, sizeof(infoHeader), &written, nullptr);
        WriteFile(hFile, frameData.data(), frameData.size(), &written, nullptr);
        CloseHandle(hFile);
        std::cout << "Saved frame as " << filename << std::endl;
    }
#else
    std::ofstream out(filename, std::ios::binary);
    if (out.is_open()) {
        out.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
        out.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));
        out.write(reinterpret_cast<const char*>(frameData.data()), frameData.size());
        out.close();
        std::cout << "Saved frame as " << filename << std::endl;
    }
#endif
}

void PrintUsage()
{
    std::cout << "MRDesktop Console Client - Remote Desktop Controller" << std::endl;
    std::cout << "Usage: MRDesktopConsoleClient [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --ip=<address>     Server IP address (default: 127.0.0.1)" << std::endl;
    std::cout << "  --port=<port>      Server port (default: 8080)" << std::endl;
    std::cout << "  --compression=<none|h264|h265|av1>  Preferred compression (default: h265)" << std::endl;
    std::cout << "  --debug-frames[=N] Save first N frames for debugging (default: 5)" << std::endl;
    std::cout << "  --test             Run in test mode (validate frames and exit)" << std::endl;
    std::cout << "  --help             Show this help message" << std::endl;
}

int main(int argc, char *argv[])
{
    // Parse command line arguments
    std::string serverIP = "127.0.0.1";
    int serverPort = 8080;
    CompressionType compression = COMPRESSION_H265;
    bool debugFrames = false;
    int maxDebugFrames = 5;
    bool testMode = false;

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg == "--help")
        {
            PrintUsage();
            return 0;
        }
        else if (arg.find("--ip=") == 0)
        {
            serverIP = arg.substr(5);
        }
        else if (arg.find("--port=") == 0)
        {
            serverPort = std::stoi(arg.substr(7));
        }
        else if (arg.find("--compression=") == 0)
        {
            std::string mode = arg.substr(14);
            std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);
            if (mode == "av1")
                compression = COMPRESSION_AV1;
            else if (mode == "h265")
                compression = COMPRESSION_H265;
            else if (mode == "none")
                compression = COMPRESSION_NONE;
            else
                compression = COMPRESSION_H264;
        }
        else if (arg == "--debug-frames")
        {
            debugFrames = true;
        }
        else if (arg.find("--debug-frames=") == 0)
        {
            debugFrames = true;
            maxDebugFrames = std::stoi(arg.substr(15));
        }
        else if (arg == "--test")
        {
            testMode = true;
        }
        else
        {
            std::cerr << "Unknown option: " << arg << std::endl;
            PrintUsage();
            return 1;
        }
    }

    std::cout << "MRDesktop Console Client - Remote Desktop Controller" << std::endl;
    std::cout << "====================================================" << std::endl;
    std::cout << "Server: " << serverIP << ":" << serverPort << std::endl;
    if (debugFrames)
    {
        std::cout << "Debug mode: Will save first " << maxDebugFrames << " frames" << std::endl;
    }
    if (testMode)
    {
        std::cout << "TEST MODE: Will validate frames and exit after receiving 3 frames" << std::endl;
    }
    std::cout << std::endl;

    // Initialize frame logger if debugging
    std::unique_ptr<FrameLogger> frameLogger;
    if (debugFrames)
    {
        frameLogger = std::make_unique<FrameLogger>(maxDebugFrames, "debug_frames");
        std::cout << "Frame logging enabled - will save to debug_frames/ directory" << std::endl;
    }

    std::cout << "Initializing network connection..." << std::endl;

    // Connect to server
    NetworkReceiver receiver;
    receiver.SetCompression(compression);
    if (!receiver.Connect(serverIP, serverPort))
    {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }

    std::cout << "Connected to MRDesktop Server!" << std::endl;
    std::cout << "Requested compression mode: " << compression << std::endl;
    std::cout << "Receiving desktop stream..." << std::endl;
    std::cout << std::endl;
    
    if (!testMode) {
        std::cout << "=== MOUSE CONTROL MODE ===" << std::endl;  
        std::cout << "WASD / Arrow Keys: Move mouse" << std::endl;
        std::cout << "Space: Left click" << std::endl;
        std::cout << "Enter: Right click" << std::endl;
        std::cout << "Q/E: Scroll up/down" << std::endl;
        std::cout << "ESC: Exit control mode" << std::endl;
        std::cout << "===========================" << std::endl;
    }

    // Input will be sent directly through receiver methods

    // Set console to raw input mode
#ifdef _WIN32
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD originalMode;
    GetConsoleMode(hStdin, &originalMode);
    SetConsoleMode(hStdin, 0); // Disable line input and echo
#endif

    // Variables for frame receiving and input handling
    int frameCount = 0;
    bool savedFirstFrame = false;
    auto startTime = std::chrono::high_resolution_clock::now();
    bool testPassed = true;
    bool exitRequested = false;
    
    // Track compression usage for test validation
    bool receivedCompressedFrame = false;
    bool receivedUncompressedFrame = false;
    
    // Set up callback to track raw frame types from network
    receiver.SetRawFrameCallback([&](MessageType msgType) {
        if (msgType == MSG_COMPRESSED_FRAME) {
            receivedCompressedFrame = true;
        } else if (msgType == MSG_FRAME_DATA) {
            receivedUncompressedFrame = true;
        }
    });
    
    // Set up frame callback  
    receiver.SetFrameCallback([&](const FrameMessage& frameMsg, const std::vector<uint8_t>& frameData) {
        frameCount++;

        // Test mode validation
        if (testMode) {
            // All frames in the callback have type MSG_FRAME_DATA (either original or decoded)
            // We need a different way to detect if compression was actually used
            std::cout << "TEST: Received frame " << frameCount << " - " << frameMsg.width << "x" << frameMsg.height 
                      << " (" << frameData.size() << " bytes) [Type: " << frameMsg.header.type << "]" << std::endl;
            
            // Validate expected dimensions
            if (frameMsg.width != 640 || frameMsg.height != 480) {
                std::cerr << "TEST FAILED: Expected 640x480, got " << frameMsg.width << "x" << frameMsg.height << std::endl;
                testPassed = false;
            }
            
            // Validate data size
            uint32_t expectedSize = 640 * 480 * 4;
            if (frameData.size() != expectedSize) {
                std::cerr << "TEST FAILED: Expected " << expectedSize << " bytes, got " << frameData.size() << std::endl;
                testPassed = false;
            }
            
            // Validate test pattern - check a few key pixels
            if (frameData.size() >= expectedSize) {
                // Check top-left corner (should be mostly black with blue frame counter)
                uint8_t blue = frameData[0];  // B
                uint8_t green = frameData[1]; // G  
                uint8_t red = frameData[2];   // R
                uint8_t alpha = frameData[3]; // A
                
                if (alpha != 255) {
                    std::cerr << "TEST FAILED: Alpha channel not 255 at (0,0)" << std::endl;
                    testPassed = false;
                }
                
                std::cout << "TEST: Frame " << frameCount << " pixel (0,0) = R:" << (int)red 
                          << " G:" << (int)green << " B:" << (int)blue << " A:" << (int)alpha << std::endl;
            }
            
            // Exit after 3 frames in test mode
            if (frameCount >= 3) {
                exitRequested = true;
                std::cout << "TEST: Received all 3 frames, exiting..." << std::endl;
            }
        }

        // Log frame for debugging if enabled
        if (frameLogger && frameLogger->IsLogging())
        {
            frameLogger->LogFrame(frameMsg.width, frameMsg.height, frameMsg.dataSize, frameData.data());

            // Print stats when logging is complete
            if (!frameLogger->IsLogging())
            {
                frameLogger->PrintFrameStats();
                std::cout << "Frame logging complete. Files saved to debug_frames/ directory." << std::endl;
            }
        }

        // Save first frame as BMP for verification (legacy behavior)
        if (!savedFirstFrame)
        {
            SaveFrameAsBMP(frameMsg, frameData, "first_frame.bmp");
            savedFirstFrame = true;
            std::cout << "First frame saved as first_frame.bmp" << std::endl;
        }

        // Display stats every 60 frames (less frequent to avoid spam)
        if (frameCount % 60 == 0)
        {
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime);
            double fps = (frameCount * 1000.0) / duration.count();

            std::cout << "Status: " << frameCount << " frames, FPS: " << fps
                      << ", Resolution: " << frameMsg.width << "x" << frameMsg.height << std::endl;
        }
    });

    const int MOUSE_SPEED = 10; // Pixels per keypress

    while (!exitRequested)
    {
        // Check for keyboard input (skip in test mode)
        if (!testMode && _kbhit())
        {
            int key = _getch();

            // Handle special keys (arrow keys return 224 then the actual code)
            if (key == 224)
            {
                key = _getch();
                switch (key)
                {
                case 72: // Up arrow
                    receiver.SendMouseMove(0, -MOUSE_SPEED);
                    std::cout << "Mouse up" << std::endl;
                    break;
                case 80: // Down arrow
                    receiver.SendMouseMove(0, MOUSE_SPEED);
                    std::cout << "Mouse down" << std::endl;
                    break;
                case 75: // Left arrow
                    receiver.SendMouseMove(-MOUSE_SPEED, 0);
                    std::cout << "Mouse left" << std::endl;
                    break;
                case 77: // Right arrow
                    receiver.SendMouseMove(MOUSE_SPEED, 0);
                    std::cout << "Mouse right" << std::endl;
                    break;
                }
            }
            else
            {
                // Handle regular keys
                switch (key)
                {
                case 'w':
                case 'W':
                    receiver.SendMouseMove(0, -MOUSE_SPEED);
                    std::cout << "Mouse up" << std::endl;
                    break;
                case 's':
                case 'S':
                    receiver.SendMouseMove(0, MOUSE_SPEED);
                    std::cout << "Mouse down" << std::endl;
                    break;
                case 'a':
                case 'A':
                    receiver.SendMouseMove(-MOUSE_SPEED, 0);
                    std::cout << "Mouse left" << std::endl;
                    break;
                case 'd':
                case 'D':
                    receiver.SendMouseMove(MOUSE_SPEED, 0);
                    std::cout << "Mouse right" << std::endl;
                    break;
                case ' ': // Space - left click
                    receiver.SendMouseClick(MouseClickMessage::LEFT_BUTTON, true);
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    receiver.SendMouseClick(MouseClickMessage::LEFT_BUTTON, false);
                    std::cout << "Left click" << std::endl;
                    break;
                case '\r': // Enter - right click
                    receiver.SendMouseClick(MouseClickMessage::RIGHT_BUTTON, true);
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    receiver.SendMouseClick(MouseClickMessage::RIGHT_BUTTON, false);
                    std::cout << "Right click" << std::endl;
                    break;
                case 'q':
                case 'Q': // Scroll up
                    receiver.SendMouseScroll(0, 1);
                    std::cout << "Scroll up" << std::endl;
                    break;
                case 'e':
                case 'E': // Scroll down
                    receiver.SendMouseScroll(0, -1);
                    std::cout << "Scroll down" << std::endl;
                    break;
                case 27: // ESC
                    exitRequested = true;
                    std::cout << "Exiting mouse control mode..." << std::endl;
                    break;
                }
            }
        }

        // Poll for frames (non-blocking)
        receiver.PollFrame();

        // Small delay to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

#ifdef _WIN32
    // Restore console mode
    SetConsoleMode(hStdin, originalMode);
#endif

    std::cout << "Streaming ended. Total frames received: " << frameCount << std::endl;
    
    if (testMode) {
        std::cout << std::endl << "=== COMPRESSION TEST RESULTS ===" << std::endl;
        std::cout << "Requested compression: " << compression << std::endl;
        std::cout << "Received compressed frames: " << (receivedCompressedFrame ? "YES" : "NO") << std::endl;
        std::cout << "Received uncompressed frames: " << (receivedUncompressedFrame ? "YES" : "NO") << std::endl;
        std::cout << "Total frames: " << frameCount << std::endl;
        
        // Test validation logic
        if (compression != COMPRESSION_NONE && !receivedCompressedFrame) {
            std::cout << "TEST FAILED: Requested compression type " << compression << " but server sent uncompressed frames!" << std::endl;
            std::cout << "This indicates compression encoder is not available or failed to initialize." << std::endl;
            testPassed = false;
        } else if (compression == COMPRESSION_NONE && receivedCompressedFrame) {
            std::cout << "TEST FAILED: Requested no compression but received compressed frames!" << std::endl;
            testPassed = false;
        }
        
        if (testPassed && frameCount >= 3) {
            if (compression != COMPRESSION_NONE && receivedCompressedFrame) {
                std::cout << "TEST PASSED: Compression working - " << frameCount << " frames encoded/decoded successfully!" << std::endl;
            } else if (compression == COMPRESSION_NONE && receivedUncompressedFrame) {
                std::cout << "TEST PASSED: Uncompressed mode working - " << frameCount << " frames received successfully!" << std::endl;
            } else {
                std::cout << "TEST PASSED: Frame validation passed but compression behavior unclear" << std::endl;
            }
            return 0; // Success
        } else {
            std::cout << "TEST FAILED: " << (testPassed ? "Insufficient frames received" : "Frame validation or compression test failed") << std::endl;
            return 1; // Failure
        }
    }
    
    return 0;
}