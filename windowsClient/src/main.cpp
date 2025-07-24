#include <windows.h>
#include <iostream>
#include "WindowManager.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);
    
    // Initialize COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        MessageBoxA(nullptr, "Failed to initialize COM", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    int exitCode = 0;
    
    {
        // Create and initialize window manager
        WindowManager windowManager;
        
        if (!windowManager.Initialize(hInstance)) {
            MessageBoxA(nullptr, "Failed to initialize window manager", "Error", MB_OK | MB_ICONERROR);
            exitCode = 1;
        } else {
            // Parse command line arguments
            std::string serverIP = "127.0.0.1";
            int serverPort = 8080;
            
            // Simple command line parsing
            std::string cmdLine = lpCmdLine;
            size_t ipPos = cmdLine.find("--ip=");
            if (ipPos != std::string::npos) {
                size_t start = ipPos + 5;
                size_t end = cmdLine.find(' ', start);
                if (end == std::string::npos) end = cmdLine.length();
                serverIP = cmdLine.substr(start, end - start);
            }
            
            size_t portPos = cmdLine.find("--port=");
            if (portPos != std::string::npos) {
                size_t start = portPos + 7;
                size_t end = cmdLine.find(' ', start);
                if (end == std::string::npos) end = cmdLine.length();
                std::string portStr = cmdLine.substr(start, end - start);
                serverPort = std::stoi(portStr);
            }
            
            // Set server parameters
            windowManager.SetServer(serverIP, serverPort);
            
            // Run the application
            exitCode = windowManager.Run();
        }
    }
    
    CoUninitialize();
    return exitCode;
}