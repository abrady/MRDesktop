#include <iostream>
#ifdef _WIN32
#include <windows.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3d11.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <errno.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1
inline int closesocket(int fd) { return close(fd); }
using SOCKET = int;
#endif
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>
#include "protocol.h"
#include "VideoEncoder.h"

#ifdef _WIN32
class DesktopDuplicator {
private:
    ID3D11Device* m_Device = nullptr;
    ID3D11DeviceContext* m_Context = nullptr;
    IDXGIOutputDuplication* m_DeskDupl = nullptr;
    IDXGIOutput1* m_Output1 = nullptr;
    DXGI_OUTPUT_DESC m_OutputDesc = {};
    
public:
    bool Initialize() {
        HRESULT hr = S_OK;
        
        // Create D3D11 device
        D3D_FEATURE_LEVEL featureLevel;
        hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, 
                              nullptr, 0, D3D11_SDK_VERSION, &m_Device, &featureLevel, &m_Context);
        if (FAILED(hr)) {
            std::cerr << "Failed to create D3D11 device: " << std::hex << hr << std::endl;
            return false;
        }
        
        // Get DXGI device
        IDXGIDevice* dxgiDevice = nullptr;
        hr = m_Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
        if (FAILED(hr)) {
            std::cerr << "Failed to get DXGI device: " << std::hex << hr << std::endl;
            return false;
        }
        
        // Get DXGI adapter
        IDXGIAdapter* dxgiAdapter = nullptr;
        hr = dxgiDevice->GetAdapter(&dxgiAdapter);
        dxgiDevice->Release();
        if (FAILED(hr)) {
            std::cerr << "Failed to get DXGI adapter: " << std::hex << hr << std::endl;
            return false;
        }
        
        // Get primary output
        IDXGIOutput* dxgiOutput = nullptr;
        hr = dxgiAdapter->EnumOutputs(0, &dxgiOutput);
        dxgiAdapter->Release();
        if (FAILED(hr)) {
            std::cerr << "Failed to get primary output: " << std::hex << hr << std::endl;
            return false;
        }
        
        // Get output description
        dxgiOutput->GetDesc(&m_OutputDesc);
        std::cout << "Primary display: " << m_OutputDesc.DesktopCoordinates.right - m_OutputDesc.DesktopCoordinates.left 
                  << "x" << m_OutputDesc.DesktopCoordinates.bottom - m_OutputDesc.DesktopCoordinates.top << std::endl;
        
        // Get IDXGIOutput1
        hr = dxgiOutput->QueryInterface(__uuidof(IDXGIOutput1), (void**)&m_Output1);
        dxgiOutput->Release();
        if (FAILED(hr)) {
            std::cerr << "Failed to get IDXGIOutput1: " << std::hex << hr << std::endl;
            return false;
        }
        
        // Create desktop duplication
        hr = m_Output1->DuplicateOutput(m_Device, &m_DeskDupl);
        if (FAILED(hr)) {
            std::cerr << "Failed to create desktop duplication: " << std::hex << hr << std::endl;
            if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE) {
                std::cerr << "Desktop duplication is not available (may be in use by another process)" << std::endl;
            }
            return false;
        }
        
        std::cout << "Desktop Duplication initialized successfully!" << std::endl;
        return true;
    }
    
    bool CaptureFrame(std::vector<BYTE>& pixelData, UINT32& width, UINT32& height, UINT32& dataSize) {
        if (!m_DeskDupl) return false;
        
        DXGI_OUTDUPL_FRAME_INFO frameInfo;
        IDXGIResource* desktopResource = nullptr;
        
        HRESULT hr = m_DeskDupl->AcquireNextFrame(100, &frameInfo, &desktopResource);
        if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
            return false; // No new frame
        }
        if (FAILED(hr)) {
            std::cerr << "Failed to acquire next frame: " << std::hex << hr << std::endl;
            return false;
        }
        
        // Get the desktop texture
        ID3D11Texture2D* desktopTexture = nullptr;
        hr = desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&desktopTexture);
        desktopResource->Release();
        if (FAILED(hr)) {
            m_DeskDupl->ReleaseFrame();
            return false;
        }
        
        // Create a staging texture to read the data
        D3D11_TEXTURE2D_DESC textureDesc;
        desktopTexture->GetDesc(&textureDesc);
        
        textureDesc.Usage = D3D11_USAGE_STAGING;
        textureDesc.BindFlags = 0;
        textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        textureDesc.MiscFlags = 0;
        
        ID3D11Texture2D* stagingTexture = nullptr;
        hr = m_Device->CreateTexture2D(&textureDesc, nullptr, &stagingTexture);
        if (FAILED(hr)) {
            desktopTexture->Release();
            m_DeskDupl->ReleaseFrame();
            return false;
        }
        
        // Copy desktop texture to staging texture
        m_Context->CopyResource(stagingTexture, desktopTexture);
        desktopTexture->Release();
        
        // Map the staging texture to read pixel data
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        hr = m_Context->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);
        if (SUCCEEDED(hr)) {
            // Set frame dimensions and data size
            width = textureDesc.Width;
            height = textureDesc.Height;
            dataSize = mappedResource.RowPitch * textureDesc.Height;
            
            // Debug: Log frame capture details
            std::cout << "Capturing frame - Width: " << width 
                     << ", Height: " << height 
                     << ", RowPitch: " << mappedResource.RowPitch
                     << ", DataSize: " << dataSize << std::endl;
            
            // Resize buffer for pixel data only
            pixelData.clear();
            pixelData.resize(dataSize);
            
            // Copy pixel data directly
            BYTE* srcData = (BYTE*)mappedResource.pData;
            BYTE* dstData = pixelData.data();
            
            for (UINT row = 0; row < textureDesc.Height; ++row) {
                memcpy(dstData + row * mappedResource.RowPitch, 
                       srcData + row * mappedResource.RowPitch, 
                       mappedResource.RowPitch);
            }
            
            m_Context->Unmap(stagingTexture, 0);
        }
        
        stagingTexture->Release();
        m_DeskDupl->ReleaseFrame();
        
        return SUCCEEDED(hr);
    }
    
    void Cleanup() {
        if (m_DeskDupl) { m_DeskDupl->Release(); m_DeskDupl = nullptr; }
        if (m_Output1) { m_Output1->Release(); m_Output1 = nullptr; }
        if (m_Context) { m_Context->Release(); m_Context = nullptr; }
        if (m_Device) { m_Device->Release(); m_Device = nullptr; }
    }
    
    ~DesktopDuplicator() {
        Cleanup();
    }
#else
class DesktopDuplicator {
public:
    bool Initialize() { return false; }
    bool CaptureFrame(std::vector<uint8_t>&, uint32_t&, uint32_t&, uint32_t&) { return false; }
    void Cleanup() {}
};
#endif

// Helper function to format bytes in human-readable format
std::string formatBytes(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unitIndex < 3) {
        size /= 1024.0;
        unitIndex++;
    }
    
    char buffer[32];
    if (unitIndex == 0) {
        snprintf(buffer, sizeof(buffer), "%.0f %s", size, units[unitIndex]);
    } else {
        snprintf(buffer, sizeof(buffer), "%.2f %s", size, units[unitIndex]);
    }
    return std::string(buffer);
}

#ifdef _WIN32
class InputInjector {
public:
    static bool InjectMouseMove(INT32 deltaX, INT32 deltaY, UINT32 absolute = 0, INT32 x = 0, INT32 y = 0) {
        INPUT input = {};
        input.type = INPUT_MOUSE;
        
        if (absolute) {
            // Convert to normalized coordinates (0-65535)
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            
            input.mi.dx = (x * 65535) / screenWidth;
            input.mi.dy = (y * 65535) / screenHeight;
            input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
        } else {
            input.mi.dx = deltaX;
            input.mi.dy = deltaY;
            input.mi.dwFlags = MOUSEEVENTF_MOVE;
        }
        
        UINT result = SendInput(1, &input, sizeof(INPUT));
        return result == 1;
    }
    
    static bool InjectMouseClick(MouseClickMessage::MouseButton button, UINT32 pressed) {
        INPUT input = {};
        input.type = INPUT_MOUSE;
        
        DWORD flags = 0;
        switch (button) {
            case MouseClickMessage::LEFT_BUTTON:
                flags = pressed ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
                break;
            case MouseClickMessage::RIGHT_BUTTON:
                flags = pressed ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
                break;
            case MouseClickMessage::MIDDLE_BUTTON:
                flags = pressed ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
                break;
            default:
                return false;
        }
        
        input.mi.dwFlags = flags;
        UINT result = SendInput(1, &input, sizeof(INPUT));
        return result == 1;
    }
    
    static bool InjectMouseScroll(INT32 deltaX, INT32 deltaY) {
        if (deltaY != 0) {
            INPUT input = {};
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_WHEEL;
            input.mi.mouseData = deltaY * WHEEL_DELTA;
            
            UINT result = SendInput(1, &input, sizeof(INPUT));
            if (result != 1) return false;
        }
        
        if (deltaX != 0) {
            INPUT input = {};
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_HWHEEL;
            input.mi.mouseData = deltaX * WHEEL_DELTA;
            
            UINT result = SendInput(1, &input, sizeof(INPUT));
            if (result != 1) return false;
        }
        
        return true;
    }
#else
class InputInjector {
public:
    static bool InjectMouseMove(int32_t, int32_t, uint32_t = 0, int32_t = 0, int32_t = 0) { return true; }
    static bool InjectMouseClick(MouseClickMessage::MouseButton, uint32_t) { return true; }
    static bool InjectMouseScroll(int32_t, int32_t) { return true; }
};
#endif

// Helper function to dump hex data for debugging
void HexDump(const char* data, size_t size, const std::string& label) {
    std::cout << label << " (size=" << size << "):" << std::endl;
    size_t maxBytes = (size < 32) ? size : 32;
    for (size_t i = 0; i < maxBytes; i++) {
        printf("%02X ", (unsigned char)data[i]);
        if ((i + 1) % 16 == 0) std::cout << std::endl;
    }
    if (size > 32) std::cout << "... (truncated)";
    std::cout << std::endl;
}

// Helper function to ensure all data is sent over the network
bool SendAllData(SOCKET socket, const char* data, size_t size) {
    size_t totalSent = 0;
    
    while (totalSent < size) {
        int sent = send(socket, data + totalSent, static_cast<int>(size - totalSent), 0);
        
        if (sent == SOCKET_ERROR) {
#ifdef _WIN32
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) {
#else
            int error = errno;
            if (error == EWOULDBLOCK || error == EAGAIN) {
#endif
                // Non-blocking socket would block, try again
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            // Real error
            std::cerr << "Send error: " << error << std::endl;
            return false;
        }
        
        if (sent == 0) {
            // Connection closed
            std::cerr << "Connection closed during send" << std::endl;
            return false;
        }
        
        totalSent += sent;
    }
    
    return true;
}

int main(int argc, char* argv[]) {
    bool testMode = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--test") == 0) {
            testMode = true;
        }
    }
    
    std::cout << "MRDesktop Server - Desktop Duplication Service" << std::endl;
    std::cout << "=============================================" << std::endl;
    
    if (testMode) {
        std::cout << "RUNNING IN TEST MODE" << std::endl;
    }
    
    // Initialize platform networking
#ifdef _WIN32
    // Initialize COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize COM: " << std::hex << hr << std::endl;
        return 1;
    }

    // Initialize Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        CoUninitialize();
        return 1;
    }
    std::cout << "Network and COM initialized successfully" << std::endl;
#else
    int result = 0; // nothing needed on POSIX
    std::cout << "Network initialized" << std::endl;
#endif
    
    // Initialize desktop duplicator (skip in test mode)
    DesktopDuplicator duplicator;
    if (!testMode && !duplicator.Initialize()) {
        std::cerr << "Failed to initialize desktop duplicator" << std::endl;
#ifdef _WIN32
        WSACleanup();
        CoUninitialize();
#endif
        return 1;
    }
    
    // Create server socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
#ifdef _WIN32
        std::cerr << "Failed to create socket: " << WSAGetLastError() << std::endl;
#else
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
#endif
#ifdef _WIN32
        WSACleanup();
        CoUninitialize();
#endif
        return 1;
    }
    
    // Bind to localhost port 8080
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);
    
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
#ifdef _WIN32
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
#else
        std::cerr << "Bind failed: " << strerror(errno) << std::endl;
#endif
        closesocket(serverSocket);
#ifdef _WIN32
        WSACleanup();
        CoUninitialize();
#endif
        return 1;
    }
    
    if (listen(serverSocket, 1) == SOCKET_ERROR) {
#ifdef _WIN32
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
#else
        std::cerr << "Listen failed: " << strerror(errno) << std::endl;
#endif
        closesocket(serverSocket);
#ifdef _WIN32
        WSACleanup();
        CoUninitialize();
#endif
        return 1;
    }
    
    std::cout << "Server listening on port 8080..." << std::endl;
    std::cout << "Waiting for client connection..." << std::endl;
    
    // Accept client connection
    SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
    if (clientSocket == INVALID_SOCKET) {
#ifdef _WIN32
        std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
#else
        std::cerr << "Accept failed: " << strerror(errno) << std::endl;
#endif
        closesocket(serverSocket);
#ifdef _WIN32
        WSACleanup();
        CoUninitialize();
#endif
        return 1;
    }
    
    std::cout << "Client connected! Starting desktop streaming..." << std::endl;

    // Wait for compression negotiation from client
    CompressionType clientCompression = COMPRESSION_NONE;
    CompressionRequestMessage compressionRequest;
    int received = recv(clientSocket, (char*)&compressionRequest, sizeof(compressionRequest), 0);
    if (received == sizeof(compressionRequest) && compressionRequest.header.type == MSG_COMPRESSION_REQUEST) {
        clientCompression = compressionRequest.compression;
        std::cout << "Client requested compression type: " << clientCompression << std::endl;
    } else {
        std::cout << "No compression request received, using uncompressed frames" << std::endl;
    }

    // Set socket to non-blocking for input checking
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(clientSocket, FIONBIO, &mode);
#else
    int flags = fcntl(clientSocket, F_GETFL, 0);
    if (flags >= 0) fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);
#endif
    
    // Streaming and input handling loop
    std::vector<BYTE> pixelData;
    UINT32 frameWidth, frameHeight, frameDataSize;
    int frameCount = 0;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Initialize video encoder if compression is requested
    std::unique_ptr<VideoEncoder> encoder;
    bool useCompression = (clientCompression != COMPRESSION_NONE);
    
    if (useCompression) {
        encoder = std::make_unique<VideoEncoder>();
        std::cout << "Compression enabled, encoder will be initialized with first frame" << std::endl;
    } else {
        std::cout << "Ready to stream uncompressed frames" << std::endl;
    }
    
    while (true) {
        // Check for incoming input messages
        MessageHeader msgHeader;
        int received = recv(clientSocket, (char*)&msgHeader, sizeof(msgHeader), 0);
        if (received == sizeof(msgHeader)) {
            // Process input message based on type
            switch (msgHeader.type) {
                case MSG_MOUSE_MOVE: {
                    MouseMoveMessage mouseMsg;
                    received = recv(clientSocket, (char*)&mouseMsg + sizeof(MessageHeader), 
                                   sizeof(MouseMoveMessage) - sizeof(MessageHeader), 0);
                    if (received == sizeof(MouseMoveMessage) - sizeof(MessageHeader)) {
                        InputInjector::InjectMouseMove(mouseMsg.deltaX, mouseMsg.deltaY, 
                                                     mouseMsg.absolute, mouseMsg.x, mouseMsg.y);
                        std::cout << "Mouse move: dx=" << mouseMsg.deltaX << " dy=" << mouseMsg.deltaY << std::endl;
                    }
                    break;
                }
                case MSG_MOUSE_CLICK: {
                    MouseClickMessage clickMsg;
                    received = recv(clientSocket, (char*)&clickMsg + sizeof(MessageHeader), 
                                   sizeof(MouseClickMessage) - sizeof(MessageHeader), 0);
                    if (received == sizeof(MouseClickMessage) - sizeof(MessageHeader)) {
                        InputInjector::InjectMouseClick(clickMsg.button, clickMsg.pressed);
                        std::cout << "Mouse " << (clickMsg.pressed ? "press" : "release") 
                                 << " button " << clickMsg.button << std::endl;
                    }
                    break;
                }
                case MSG_MOUSE_SCROLL: {
                    MouseScrollMessage scrollMsg;
                    received = recv(clientSocket, (char*)&scrollMsg + sizeof(MessageHeader), 
                                   sizeof(MouseScrollMessage) - sizeof(MessageHeader), 0);
                    if (received == sizeof(MouseScrollMessage) - sizeof(MessageHeader)) {
                        InputInjector::InjectMouseScroll(scrollMsg.deltaX, scrollMsg.deltaY);
                        std::cout << "Mouse scroll: dx=" << scrollMsg.deltaX << " dy=" << scrollMsg.deltaY << std::endl;
                    }
                    break;
                }
            }
        }
        
        // Capture or generate test frame
        bool frameReady = false;
        if (testMode) {
            // Generate test frame data
            frameWidth = 640;
            frameHeight = 480;
            frameDataSize = frameWidth * frameHeight * 4; // BGRA format
            
            pixelData.clear();
            pixelData.resize(frameDataSize);
            
            // Create test pattern: red-green gradient with frame counter
            for (uint32_t y = 0; y < frameHeight; y++) {
                for (uint32_t x = 0; x < frameWidth; x++) {
                    uint32_t index = (y * frameWidth + x) * 4;
                    uint8_t red = static_cast<uint8_t>((x * 255) / frameWidth);
                    uint8_t green = static_cast<uint8_t>((y * 255) / frameHeight);
                    uint8_t blue = static_cast<uint8_t>(frameCount % 256);
                    
                    pixelData[index + 0] = blue;   // B
                    pixelData[index + 1] = green;  // G
                    pixelData[index + 2] = red;    // R
                    pixelData[index + 3] = 255;    // A
                }
            }
            frameReady = true;
            std::cout << "Generated test frame " << frameCount + 1 << " (640x480)" << std::endl;
        } else {
            frameReady = duplicator.CaptureFrame(pixelData, frameWidth, frameHeight, frameDataSize);
        }
        
        if (frameReady) {
            // Validate frame dimensions are reasonable
            if (frameWidth == 0 || frameHeight == 0 ||
                frameWidth > 10000 || frameHeight > 10000 ||
                frameDataSize > 100000000) {
                std::cerr << "Invalid frame data - Width: " << frameWidth 
                         << ", Height: " << frameHeight 
                         << ", DataSize: " << frameDataSize << std::endl;
                continue;
            }
            
            if (useCompression) {
                // Initialize encoder with first frame dimensions
                if (!encoder->IsInitialized()) {
                    if (!encoder->Initialize(frameWidth, frameHeight, clientCompression)) {
                        std::cerr << "Failed to initialize video encoder" << std::endl;
                        useCompression = false; // Fall back to uncompressed
                    } else {
                        std::cout << "Video encoder initialized successfully" << std::endl;
                    }
                }
                
                if (encoder->IsInitialized()) {
                    // Encode frame
                    std::vector<uint8_t> compressedData;
                    bool isKeyframe = false;
                    
                    if (encoder->EncodeFrame(pixelData.data(), compressedData, isKeyframe)) {
                        // Send compressed frame
                        CompressedFrameMessage compFrameMsg;
                        compFrameMsg.header.type = MSG_COMPRESSED_FRAME;
                        compFrameMsg.header.size = sizeof(CompressedFrameMessage);
                        compFrameMsg.width = frameWidth;
                        compFrameMsg.height = frameHeight;
                        compFrameMsg.compressedSize = compressedData.size();
                        compFrameMsg.isKeyframe = isKeyframe ? 1 : 0;
                        
                        std::cout << "SERVER SEND: Frame " << frameCount + 1 << " - Compressed: " << compressedData.size() 
                                  << " bytes (" << (isKeyframe ? "KEY" : "DELTA") << ")" << std::endl;
                        
                        if (!SendAllData(clientSocket, (char*)&compFrameMsg, sizeof(CompressedFrameMessage))) {
                            std::cerr << "Failed to send compressed frame header" << std::endl;
                            break;
                        }
                        
                        if (!SendAllData(clientSocket, (char*)compressedData.data(), compressedData.size())) {
                            std::cerr << "Failed to send compressed frame data" << std::endl;
                            break;
                        }
                    } else {
                        // Skip this frame if encoding failed
                        continue;
                    }
                } else {
                    useCompression = false; // Fall back to uncompressed
                }
            }
            
            if (!useCompression) {
                // Send uncompressed frame
                FrameMessage frameMsg;
                frameMsg.header.type = MSG_FRAME_DATA;
                frameMsg.header.size = sizeof(FrameMessage);
                frameMsg.width = frameWidth;
                frameMsg.height = frameHeight;
                frameMsg.dataSize = frameDataSize;
                
                std::cout << "SERVER SEND: Frame " << frameCount + 1 << " - Uncompressed: " << frameDataSize << " bytes" << std::endl;
                
                if (!SendAllData(clientSocket, (char*)&frameMsg, sizeof(FrameMessage))) {
                    std::cerr << "Failed to send frame header" << std::endl;
                    break;
                }
                
                if (!SendAllData(clientSocket, (char*)pixelData.data(), frameDataSize)) {
                    std::cerr << "Failed to send frame data" << std::endl;
                    break;
                }
            }
            
            std::cout << "SERVER SEND: Frame " << frameCount + 1 << " - COMPLETE" << std::endl;
            
            frameCount++;
            if (frameCount % 30 == 0) {
                auto currentTime = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime);
                double fps = (frameCount * 1000.0) / duration.count();
                std::cout << "Sent " << frameCount << " frames, FPS: " << fps << ", Frame size: " << formatBytes(frameDataSize) << std::endl;
            }
            
            // In test mode, exit after sending 3 frames
            if (testMode && frameCount >= 3) {
                std::cout << "TEST MODE: Sent 3 frames, exiting successfully" << std::endl;
                break;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60fps target
    }
    
    closesocket(clientSocket);
    closesocket(serverSocket);
#ifdef _WIN32
    WSACleanup();
    CoUninitialize();
#endif
    return 0;
}