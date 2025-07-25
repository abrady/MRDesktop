#include <iostream>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <thread>
#include <chrono>
#include <conio.h>
#include <string>
#include <algorithm>
#include "protocol.h"
#include "../shared/FrameLogger.h"
#include "../shared/FrameReceiver.h"

#pragma comment(lib, "ws2_32.lib")

class InputSender
{
private:
    SOCKET m_Socket = INVALID_SOCKET;

public:
    InputSender(SOCKET socket) : m_Socket(socket) {}

    bool SendMouseMove(INT32 deltaX, INT32 deltaY, UINT32 absolute = 0, INT32 x = 0, INT32 y = 0)
    {
        MouseMoveMessage msg;
        msg.header.type = MSG_MOUSE_MOVE;
        msg.header.size = sizeof(MouseMoveMessage);
        msg.deltaX = deltaX;
        msg.deltaY = deltaY;
        msg.absolute = absolute;
        msg.x = x;
        msg.y = y;

        int sent = send(m_Socket, (char *)&msg, sizeof(msg), 0);
        return sent == sizeof(msg);
    }

    bool SendMouseClick(MouseClickMessage::MouseButton button, UINT32 pressed)
    {
        MouseClickMessage msg;
        msg.header.type = MSG_MOUSE_CLICK;
        msg.header.size = sizeof(MouseClickMessage);
        msg.button = button;
        msg.pressed = pressed;

        int sent = send(m_Socket, (char *)&msg, sizeof(msg), 0);
        return sent == sizeof(msg);
    }

    bool SendMouseScroll(INT32 deltaX, INT32 deltaY)
    {
        MouseScrollMessage msg;
        msg.header.type = MSG_MOUSE_SCROLL;
        msg.header.size = sizeof(MouseScrollMessage);
        msg.deltaX = deltaX;
        msg.deltaY = deltaY;

        int sent = send(m_Socket, (char *)&msg, sizeof(msg), 0);
        return sent == sizeof(msg);
    }
};

void SaveFrameAsBMP(const FrameMessage &frameMsg, const std::vector<uint8_t> &frameData, const std::string &filename)
{
    // Simple BMP save for debugging (assumes BGRA format)
    BITMAPFILEHEADER fileHeader = {};
    BITMAPINFOHEADER infoHeader = {};

    fileHeader.bfType = 0x4D42; // "BM"
    fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + frameData.size();
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = frameMsg.width;
    infoHeader.biHeight = -(int)frameMsg.height; // Top-down DIB
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 32;
    infoHeader.biCompression = BI_RGB;
    infoHeader.biSizeImage = frameData.size();

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
}

void PrintUsage()
{
    std::cout << "MRDesktop Console Client - Remote Desktop Controller" << std::endl;
    std::cout << "Usage: MRDesktopConsoleClient [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --ip=<address>     Server IP address (default: 127.0.0.1)" << std::endl;
    std::cout << "  --port=<port>      Server port (default: 8080)" << std::endl;
    std::cout << "  --debug-frames[=N] Save first N frames for debugging (default: 5)" << std::endl;
    std::cout << "  --help             Show this help message" << std::endl;
}

int main(int argc, char *argv[])
{
    // Parse command line arguments
    std::string serverIP = "127.0.0.1";
    int serverPort = 8080;
    bool debugFrames = false;
    int maxDebugFrames = 5;

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
        else if (arg == "--debug-frames")
        {
            debugFrames = true;
        }
        else if (arg.find("--debug-frames=") == 0)
        {
            debugFrames = true;
            maxDebugFrames = std::stoi(arg.substr(15));
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
    std::cout << std::endl;

    // Initialize frame logger if debugging
    std::unique_ptr<FrameLogger> frameLogger;
    if (debugFrames)
    {
        frameLogger = std::make_unique<FrameLogger>(maxDebugFrames, "debug_frames");
        std::cout << "Frame logging enabled - will save to debug_frames/ directory" << std::endl;
    }

    // Initialize Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }

    std::cout << "Winsock initialized successfully" << std::endl;

    // Connect to server
    FrameReceiver receiver;
    if (!receiver.Connect(serverIP.c_str(), serverPort))
    {
        std::cerr << "Failed to connect to server" << std::endl;
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to MRDesktop Server!" << std::endl;
    std::cout << "Receiving desktop stream..." << std::endl;
    std::cout << std::endl;
    std::cout << "=== MOUSE CONTROL MODE ===" << std::endl;
    std::cout << "WASD / Arrow Keys: Move mouse" << std::endl;
    std::cout << "Space: Left click" << std::endl;
    std::cout << "Enter: Right click" << std::endl;
    std::cout << "Q/E: Scroll up/down" << std::endl;
    std::cout << "ESC: Exit control mode" << std::endl;
    std::cout << "===========================" << std::endl;

    // Create input sender
    InputSender inputSender(receiver.GetSocket());

    // Set console to raw input mode
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD originalMode;
    GetConsoleMode(hStdin, &originalMode);
    SetConsoleMode(hStdin, 0); // Disable line input and echo

    // Variables for frame receiving and input handling
    FrameMessage frameMsg;
    std::vector<uint8_t> frameData;
    int frameCount = 0;
    bool savedFirstFrame = false;
    auto startTime = std::chrono::high_resolution_clock::now();

    const int MOUSE_SPEED = 10; // Pixels per keypress
    bool exitRequested = false;

    while (!exitRequested)
    {
        // Check for keyboard input
        if (_kbhit())
        {
            int key = _getch();

            // Handle special keys (arrow keys return 224 then the actual code)
            if (key == 224)
            {
                key = _getch();
                switch (key)
                {
                case 72: // Up arrow
                    inputSender.SendMouseMove(0, -MOUSE_SPEED);
                    std::cout << "Mouse up" << std::endl;
                    break;
                case 80: // Down arrow
                    inputSender.SendMouseMove(0, MOUSE_SPEED);
                    std::cout << "Mouse down" << std::endl;
                    break;
                case 75: // Left arrow
                    inputSender.SendMouseMove(-MOUSE_SPEED, 0);
                    std::cout << "Mouse left" << std::endl;
                    break;
                case 77: // Right arrow
                    inputSender.SendMouseMove(MOUSE_SPEED, 0);
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
                    inputSender.SendMouseMove(0, -MOUSE_SPEED);
                    std::cout << "Mouse up" << std::endl;
                    break;
                case 's':
                case 'S':
                    inputSender.SendMouseMove(0, MOUSE_SPEED);
                    std::cout << "Mouse down" << std::endl;
                    break;
                case 'a':
                case 'A':
                    inputSender.SendMouseMove(-MOUSE_SPEED, 0);
                    std::cout << "Mouse left" << std::endl;
                    break;
                case 'd':
                case 'D':
                    inputSender.SendMouseMove(MOUSE_SPEED, 0);
                    std::cout << "Mouse right" << std::endl;
                    break;
                case ' ': // Space - left click
                    inputSender.SendMouseClick(MouseClickMessage::LEFT_BUTTON, 1);
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    inputSender.SendMouseClick(MouseClickMessage::LEFT_BUTTON, 0);
                    std::cout << "Left click" << std::endl;
                    break;
                case '\r': // Enter - right click
                    inputSender.SendMouseClick(MouseClickMessage::RIGHT_BUTTON, 1);
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    inputSender.SendMouseClick(MouseClickMessage::RIGHT_BUTTON, 0);
                    std::cout << "Right click" << std::endl;
                    break;
                case 'q':
                case 'Q': // Scroll up
                    inputSender.SendMouseScroll(0, 1);
                    std::cout << "Scroll up" << std::endl;
                    break;
                case 'e':
                case 'E': // Scroll down
                    inputSender.SendMouseScroll(0, -1);
                    std::cout << "Scroll down" << std::endl;
                    break;
                case 27: // ESC
                    exitRequested = true;
                    std::cout << "Exiting mouse control mode..." << std::endl;
                    break;
                }
            }
        }

        // Try to receive frame (non-blocking)
        if (receiver.ReceiveFrame(frameMsg, frameData, frameCount + 1))
        {
            frameCount++;

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
        }

        // Small delay to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Restore console mode
    SetConsoleMode(hStdin, originalMode);

    std::cout << "Streaming ended. Total frames received: " << frameCount << std::endl;

    WSACleanup();
    return 0;
}