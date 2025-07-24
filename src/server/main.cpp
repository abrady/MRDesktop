#include <iostream>
#include <windows.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3d11.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <thread>
#include <chrono>
#include "protocol.h"

#pragma comment(lib, "ws2_32.lib")

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
    
    bool CaptureFrame(std::vector<BYTE>& frameData) {
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
            // Simple frame header: width, height, data size
            struct FrameHeader {
                UINT32 width;
                UINT32 height;
                UINT32 dataSize;
            };
            
            FrameHeader header;
            header.width = textureDesc.Width;
            header.height = textureDesc.Height;
            header.dataSize = mappedResource.RowPitch * textureDesc.Height;
            
            // Debug: Log frame capture details
            std::cout << "Capturing frame - Width: " << header.width 
                     << ", Height: " << header.height 
                     << ", RowPitch: " << mappedResource.RowPitch
                     << ", DataSize: " << header.dataSize << std::endl;
            
            frameData.clear();
            frameData.resize(sizeof(FrameHeader) + header.dataSize);
            
            // Copy header
            memcpy(frameData.data(), &header, sizeof(FrameHeader));
            
            // Copy pixel data
            BYTE* srcData = (BYTE*)mappedResource.pData;
            BYTE* dstData = frameData.data() + sizeof(FrameHeader);
            
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
};

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

class InputInjector {
public:
    static bool InjectMouseMove(INT32 deltaX, INT32 deltaY, BOOL absolute = FALSE, INT32 x = 0, INT32 y = 0) {
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
    
    static bool InjectMouseClick(MouseClickMessage::MouseButton button, BOOL pressed) {
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
};

int main() {
    std::cout << "MRDesktop Server - Desktop Duplication Service" << std::endl;
    std::cout << "=============================================" << std::endl;
    
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
    
    // Initialize desktop duplicator
    DesktopDuplicator duplicator;
    if (!duplicator.Initialize()) {
        std::cerr << "Failed to initialize desktop duplicator" << std::endl;
        WSACleanup();
        CoUninitialize();
        return 1;
    }
    
    // Create server socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket: " << WSAGetLastError() << std::endl;
        WSACleanup();
        CoUninitialize();
        return 1;
    }
    
    // Bind to localhost port 8080
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);
    
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        CoUninitialize();
        return 1;
    }
    
    if (listen(serverSocket, 1) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        CoUninitialize();
        return 1;
    }
    
    std::cout << "Server listening on port 8080..." << std::endl;
    std::cout << "Waiting for client connection..." << std::endl;
    
    // Accept client connection
    SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        CoUninitialize();
        return 1;
    }
    
    std::cout << "Client connected! Starting desktop streaming..." << std::endl;
    
    // Set socket to non-blocking for input checking
    u_long mode = 1;
    ioctlsocket(clientSocket, FIONBIO, &mode);
    
    // Streaming and input handling loop
    std::vector<BYTE> frameData;
    int frameCount = 0;
    auto startTime = std::chrono::high_resolution_clock::now();
    
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
        
        // Capture and send frame
        if (duplicator.CaptureFrame(frameData)) {
            // Create frame message
            FrameMessage frameMsg;
            frameMsg.header.type = MSG_FRAME_DATA;
            frameMsg.header.size = sizeof(FrameMessage) + frameData.size();
            
            // Extract frame info from existing data
            struct OldFrameHeader {
                UINT32 width;
                UINT32 height;
                UINT32 dataSize;
            };
            
            // Validate frameData has minimum required size
            if (frameData.size() < sizeof(OldFrameHeader)) {
                std::cerr << "Invalid frameData size: " << frameData.size() << std::endl;
                continue;
            }
            
            OldFrameHeader* oldHeader = (OldFrameHeader*)frameData.data();
            
            // Validate frame dimensions are reasonable
            if (oldHeader->width == 0 || oldHeader->height == 0 ||
                oldHeader->width > 10000 || oldHeader->height > 10000 ||
                oldHeader->dataSize > 100000000) {
                std::cerr << "Invalid frame data - Width: " << oldHeader->width 
                         << ", Height: " << oldHeader->height 
                         << ", DataSize: " << oldHeader->dataSize << std::endl;
                continue;
            }
            
            frameMsg.width = oldHeader->width;
            frameMsg.height = oldHeader->height;
            frameMsg.dataSize = oldHeader->dataSize;
            
            // Send frame message header
            int sent = send(clientSocket, (char*)&frameMsg, sizeof(FrameMessage), 0);
            if (sent == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
                std::cerr << "Failed to send frame header: " << WSAGetLastError() << std::endl;
                break;
            }
            
            // Send frame pixel data
            if (sent == sizeof(FrameMessage)) {
                sent = send(clientSocket, (char*)frameData.data() + sizeof(OldFrameHeader), 
                           frameMsg.dataSize, 0);
                if (sent == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
                    std::cerr << "Failed to send frame data: " << WSAGetLastError() << std::endl;
                    break;
                }
            }
            
            frameCount++;
            if (frameCount % 30 == 0) {
                auto currentTime = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime);
                double fps = (frameCount * 1000.0) / duration.count();
                std::cout << "Sent " << frameCount << " frames, FPS: " << fps << ", Frame size: " << formatBytes(frameMsg.dataSize) << std::endl;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60fps target
    }
    
    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();
    CoUninitialize();
    return 0;
}