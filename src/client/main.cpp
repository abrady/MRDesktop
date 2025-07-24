#include <iostream>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

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
    std::cout << "Network layer ready for receiving desktop stream..." << std::endl;
    
    // Display system info
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    std::cout << "System Info:" << std::endl;
    std::cout << "  Processor Architecture: " << sysInfo.wProcessorArchitecture << std::endl;
    std::cout << "  Number of Processors: " << sysInfo.dwNumberOfProcessors << std::endl;
    
    // Get screen dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    std::cout << "  Screen Resolution: " << screenWidth << "x" << screenHeight << std::endl;
    
    std::cout << std::endl;
    std::cout << "Client ready to receive desktop stream..." << std::endl;
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
    
    WSACleanup();
    return 0;
}