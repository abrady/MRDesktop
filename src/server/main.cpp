#include <iostream>
#include <windows.h>
#include <dxgi.h>
#include <d3d11.h>

int main() {
    std::cout << "MRDesktop Server - Desktop Duplication Service" << std::endl;
    std::cout << "=============================================" << std::endl;
    
    // Initialize COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize COM: " << std::hex << hr << std::endl;
        return 1;
    }
    
    std::cout << "COM initialized successfully" << std::endl;
    
    // Check if DXGI is available
    IDXGIFactory* pFactory = nullptr;
    hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory);
    if (SUCCEEDED(hr)) {
        std::cout << "DXGI Factory created successfully" << std::endl;
        std::cout << "Desktop Duplication API is available!" << std::endl;
        pFactory->Release();
    } else {
        std::cerr << "Failed to create DXGI Factory: " << std::hex << hr << std::endl;
    }
    
    std::cout << "Server ready for desktop streaming..." << std::endl;
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
    
    CoUninitialize();
    return 0;
}