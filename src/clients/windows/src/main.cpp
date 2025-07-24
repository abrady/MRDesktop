#include <windows.h>
#include <iostream>
#include <io.h>
#include <fcntl.h>
#include "WindowManager.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);
    
    // Allocate console for debug output
    AllocConsole();
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
    freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
    
    // Initialize COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        MessageBoxA(nullptr, "Failed to initialize COM", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    int exitCode = 0;
    
    {
        // Parse command line arguments FIRST
        std::string serverIP = "127.0.0.1";
        int serverPort = 8080;
        
        std::cout << "Raw lpCmdLine: '" << lpCmdLine << "'" << std::endl;
        
        std::string cmdLine = lpCmdLine;
        if (!cmdLine.empty()) {
            size_t ipPos = cmdLine.find("--ip=");
            if (ipPos != std::string::npos) {
                size_t start = ipPos + 5;
                size_t end = cmdLine.find(' ', start);
                if (end == std::string::npos) end = cmdLine.length();
                serverIP = cmdLine.substr(start, end - start);
                std::cout << "Found IP: " << serverIP << std::endl;
            }
            
            size_t portPos = cmdLine.find("--port=");
            if (portPos != std::string::npos) {
                size_t start = portPos + 7;
                size_t end = cmdLine.find(' ', start);
                if (end == std::string::npos) end = cmdLine.length();
                std::string portStr = cmdLine.substr(start, end - start);
                serverPort = std::stoi(portStr);
                std::cout << "Found port: " << serverPort << std::endl;
            }
        }
        
        // Create and initialize window manager
        WindowManager windowManager;
        
        // Set server parameters BEFORE initialization
        windowManager.SetServer(serverIP, serverPort);
        
        if (!windowManager.Initialize(hInstance)) {
            MessageBoxA(nullptr, "Failed to initialize window manager", "Error", MB_OK | MB_ICONERROR);
            exitCode = 1;
        } else {
            
            // Run the application
            exitCode = windowManager.Run();
        }
    }
    
    CoUninitialize();
    return exitCode;
}