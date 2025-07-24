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
    
    // Streaming loop
    std::vector<BYTE> frameData;
    int frameCount = 0;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    while (true) {
        if (duplicator.CaptureFrame(frameData)) {
            // Send frame size first
            UINT32 frameSize = static_cast<UINT32>(frameData.size());
            int sent = send(clientSocket, (char*)&frameSize, sizeof(frameSize), 0);
            if (sent == SOCKET_ERROR) {
                std::cerr << "Failed to send frame size: " << WSAGetLastError() << std::endl;
                break;
            }
            
            // Send frame data
            sent = send(clientSocket, (char*)frameData.data(), frameSize, 0);
            if (sent == SOCKET_ERROR) {
                std::cerr << "Failed to send frame data: " << WSAGetLastError() << std::endl;
                break;
            }
            
            frameCount++;
            if (frameCount % 30 == 0) {
                auto currentTime = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime);
                double fps = (frameCount * 1000.0) / duration.count();
                std::cout << "Sent " << frameCount << " frames, FPS: " << fps << ", Frame size: " << frameSize << " bytes" << std::endl;
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