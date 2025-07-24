#include <iostream>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <thread>
#include <chrono>
#include <conio.h>
#include "protocol.h"

#pragma comment(lib, "ws2_32.lib")

class InputSender {
private:
    SOCKET m_Socket = INVALID_SOCKET;
    
public:
    InputSender(SOCKET socket) : m_Socket(socket) {}
    
    bool SendMouseMove(INT32 deltaX, INT32 deltaY, BOOL absolute = FALSE, INT32 x = 0, INT32 y = 0) {
        MouseMoveMessage msg;
        msg.header.type = MSG_MOUSE_MOVE;
        msg.header.size = sizeof(MouseMoveMessage);
        msg.deltaX = deltaX;
        msg.deltaY = deltaY;
        msg.absolute = absolute;
        msg.x = x;
        msg.y = y;
        
        int sent = send(m_Socket, (char*)&msg, sizeof(msg), 0);
        return sent == sizeof(msg);
    }
    
    bool SendMouseClick(MouseClickMessage::MouseButton button, BOOL pressed) {
        MouseClickMessage msg;
        msg.header.type = MSG_MOUSE_CLICK;
        msg.header.size = sizeof(MouseClickMessage);
        msg.button = button;
        msg.pressed = pressed;
        
        int sent = send(m_Socket, (char*)&msg, sizeof(msg), 0);
        return sent == sizeof(msg);
    }
    
    bool SendMouseScroll(INT32 deltaX, INT32 deltaY) {
        MouseScrollMessage msg;
        msg.header.type = MSG_MOUSE_SCROLL;
        msg.header.size = sizeof(MouseScrollMessage);
        msg.deltaX = deltaX;
        msg.deltaY = deltaY;
        
        int sent = send(m_Socket, (char*)&msg, sizeof(msg), 0);
        return sent == sizeof(msg);
    }
};

class FrameReceiver {
private:
    SOCKET m_Socket = INVALID_SOCKET;
    std::vector<BYTE> m_FrameBuffer;
    
public:
    bool Connect(const char* serverIP, int port) {
        m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_Socket == INVALID_SOCKET) {
            std::cerr << "Failed to create socket: " << WSAGetLastError() << std::endl;
            return false;
        }
        
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        
        if (inet_pton(AF_INET, serverIP, &serverAddr.sin_addr) <= 0) {
            std::cerr << "Invalid server IP address" << std::endl;
            closesocket(m_Socket);
            m_Socket = INVALID_SOCKET;
            return false;
        }
        
        if (connect(m_Socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Failed to connect to server: " << WSAGetLastError() << std::endl;
            closesocket(m_Socket);
            m_Socket = INVALID_SOCKET;
            return false;
        }
        
        std::cout << "Connected to server successfully!" << std::endl;
        return true;
    }
    
    bool ReceiveFrame(FrameMessage& frameMsg, std::vector<BYTE>& frameData) {
        if (m_Socket == INVALID_SOCKET) return false;
        
        // Receive frame message header
        int received = recv(m_Socket, (char*)&frameMsg, sizeof(FrameMessage), 0);
        if (received != sizeof(FrameMessage)) {
            if (received == 0) {
                std::cout << "Server disconnected" << std::endl;
            } else if (received > 0) {
                std::cerr << "Partial frame header received: " << received << std::endl;
            }
            return false;
        }
        
        // Verify this is a frame message
        if (frameMsg.header.type != MSG_FRAME_DATA) {
            std::cerr << "Unexpected message type: " << frameMsg.header.type << std::endl;
            return false;
        }
        
        // Resize buffer if needed
        if (m_FrameBuffer.size() < frameMsg.dataSize) {
            m_FrameBuffer.resize(frameMsg.dataSize);
        }
        
        // Receive frame pixel data
        UINT32 totalReceived = 0;
        while (totalReceived < frameMsg.dataSize) {
            received = recv(m_Socket, (char*)m_FrameBuffer.data() + totalReceived, 
                           frameMsg.dataSize - totalReceived, 0);
            if (received <= 0) {
                std::cerr << "Failed to receive frame data: " << WSAGetLastError() << std::endl;
                return false;
            }
            totalReceived += received;
        }
        
        // Copy frame data
        frameData.clear();
        frameData.resize(frameMsg.dataSize);
        memcpy(frameData.data(), m_FrameBuffer.data(), frameMsg.dataSize);
        
        return true;
    }
    
    void Disconnect() {
        if (m_Socket != INVALID_SOCKET) {
            closesocket(m_Socket);
            m_Socket = INVALID_SOCKET;
        }
    }
    
    SOCKET GetSocket() const { return m_Socket; }
    
    ~FrameReceiver() {
        Disconnect();
    }
};

void SaveFrameAsBMP(const FrameMessage& frameMsg, const std::vector<BYTE>& frameData, const std::string& filename) {
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
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteFile(hFile, &fileHeader, sizeof(fileHeader), &written, nullptr);
        WriteFile(hFile, &infoHeader, sizeof(infoHeader), &written, nullptr);
        WriteFile(hFile, frameData.data(), frameData.size(), &written, nullptr);
        CloseHandle(hFile);
        std::cout << "Saved frame as " << filename << std::endl;
    }
}

int main() {
    std::cout << "MRDesktop Client - Remote Desktop Viewer" << std::endl;
    std::cout << "=======================================" << std::endl;
    
    // Initialize Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }
    
    std::cout << "Winsock initialized successfully" << std::endl;
    
    // Connect to server
    FrameReceiver receiver;
    if (!receiver.Connect("127.0.0.1", 8080)) {
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
    std::vector<BYTE> frameData;
    int frameCount = 0;
    bool savedFirstFrame = false;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    const int MOUSE_SPEED = 10; // Pixels per keypress
    bool exitRequested = false;
    
    while (!exitRequested) {
        // Check for keyboard input
        if (_kbhit()) {
            int key = _getch();
            
            // Handle special keys (arrow keys return 224 then the actual code)
            if (key == 224) {
                key = _getch();
                switch (key) {
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
            } else {
                // Handle regular keys
                switch (key) {
                    case 'w': case 'W':
                        inputSender.SendMouseMove(0, -MOUSE_SPEED);
                        std::cout << "Mouse up" << std::endl;
                        break;
                    case 's': case 'S':
                        inputSender.SendMouseMove(0, MOUSE_SPEED);
                        std::cout << "Mouse down" << std::endl;
                        break;
                    case 'a': case 'A':
                        inputSender.SendMouseMove(-MOUSE_SPEED, 0);
                        std::cout << "Mouse left" << std::endl;
                        break;
                    case 'd': case 'D':
                        inputSender.SendMouseMove(MOUSE_SPEED, 0);
                        std::cout << "Mouse right" << std::endl;
                        break;
                    case ' ': // Space - left click
                        inputSender.SendMouseClick(MouseClickMessage::LEFT_BUTTON, TRUE);
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        inputSender.SendMouseClick(MouseClickMessage::LEFT_BUTTON, FALSE);
                        std::cout << "Left click" << std::endl;
                        break;
                    case '\r': // Enter - right click
                        inputSender.SendMouseClick(MouseClickMessage::RIGHT_BUTTON, TRUE);
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        inputSender.SendMouseClick(MouseClickMessage::RIGHT_BUTTON, FALSE);
                        std::cout << "Right click" << std::endl;
                        break;
                    case 'q': case 'Q': // Scroll up
                        inputSender.SendMouseScroll(0, 1);
                        std::cout << "Scroll up" << std::endl;
                        break;
                    case 'e': case 'E': // Scroll down
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
        if (receiver.ReceiveFrame(frameMsg, frameData)) {
            frameCount++;
            
            // Save first frame as BMP for verification
            if (!savedFirstFrame) {
                SaveFrameAsBMP(frameMsg, frameData, "first_frame.bmp");
                savedFirstFrame = true;
                std::cout << "First frame saved as first_frame.bmp" << std::endl;
            }
            
            // Display stats every 60 frames (less frequent to avoid spam)
            if (frameCount % 60 == 0) {
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