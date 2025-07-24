#include <iostream>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <thread>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

struct FrameHeader {
    UINT32 width;
    UINT32 height;
    UINT32 dataSize;
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
    
    bool ReceiveFrame(FrameHeader& header, std::vector<BYTE>& frameData) {
        if (m_Socket == INVALID_SOCKET) return false;
        
        // Receive frame size
        UINT32 frameSize;
        int received = recv(m_Socket, (char*)&frameSize, sizeof(frameSize), 0);
        if (received != sizeof(frameSize)) {
            if (received == 0) {
                std::cout << "Server disconnected" << std::endl;
            } else {
                std::cerr << "Failed to receive frame size: " << WSAGetLastError() << std::endl;
            }
            return false;
        }
        
        // Resize buffer if needed
        if (m_FrameBuffer.size() < frameSize) {
            m_FrameBuffer.resize(frameSize);
        }
        
        // Receive frame data
        UINT32 totalReceived = 0;
        while (totalReceived < frameSize) {
            received = recv(m_Socket, (char*)m_FrameBuffer.data() + totalReceived, 
                           frameSize - totalReceived, 0);
            if (received <= 0) {
                std::cerr << "Failed to receive frame data: " << WSAGetLastError() << std::endl;
                return false;
            }
            totalReceived += received;
        }
        
        // Parse header
        if (frameSize < sizeof(FrameHeader)) {
            std::cerr << "Invalid frame size" << std::endl;
            return false;
        }
        
        memcpy(&header, m_FrameBuffer.data(), sizeof(FrameHeader));
        
        // Copy frame data
        frameData.clear();
        frameData.resize(header.dataSize);
        memcpy(frameData.data(), m_FrameBuffer.data() + sizeof(FrameHeader), header.dataSize);
        
        return true;
    }
    
    void Disconnect() {
        if (m_Socket != INVALID_SOCKET) {
            closesocket(m_Socket);
            m_Socket = INVALID_SOCKET;
        }
    }
    
    ~FrameReceiver() {
        Disconnect();
    }
};

void SaveFrameAsBMP(const FrameHeader& header, const std::vector<BYTE>& frameData, const std::string& filename) {
    // Simple BMP save for debugging (assumes BGRA format)
    BITMAPFILEHEADER fileHeader = {};
    BITMAPINFOHEADER infoHeader = {};
    
    fileHeader.bfType = 0x4D42; // "BM"
    fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + frameData.size();
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    
    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = header.width;
    infoHeader.biHeight = -(int)header.height; // Top-down DIB
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
    std::cout << "Press Ctrl+C to exit" << std::endl;
    
    // Receive and process frames
    FrameHeader header;
    std::vector<BYTE> frameData;
    int frameCount = 0;
    bool savedFirstFrame = false;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    while (receiver.ReceiveFrame(header, frameData)) {
        frameCount++;
        
        // Save first frame as BMP for verification
        if (!savedFirstFrame) {
            SaveFrameAsBMP(header, frameData, "first_frame.bmp");
            savedFirstFrame = true;
        }
        
        // Display stats every 30 frames
        if (frameCount % 30 == 0) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime);
            double fps = (frameCount * 1000.0) / duration.count();
            
            std::cout << "Received " << frameCount << " frames" << std::endl;
            std::cout << "  Resolution: " << header.width << "x" << header.height << std::endl;
            std::cout << "  FPS: " << fps << std::endl;
            std::cout << "  Frame size: " << header.dataSize << " bytes" << std::endl;
            std::cout << "  Total data: " << (frameCount * header.dataSize) / (1024 * 1024) << " MB" << std::endl;
        }
        
        // Add small delay to prevent overwhelming the output
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    std::cout << "Streaming ended. Total frames received: " << frameCount << std::endl;
    
    WSACleanup();
    return 0;
}