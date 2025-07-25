#pragma once
#include <windows.h>
#include <objbase.h>
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <thread>

// Forward declarations
class VideoRenderer;
class SimpleVideoRenderer;
class NetworkReceiver;
class InputHandler;

class WindowManager {
private:
    static constexpr const wchar_t* WINDOW_CLASS_NAME = L"MRDesktopWindowsClient";
    static constexpr const wchar_t* WINDOW_TITLE = L"MRDesktop Windows Client";
    
    HINSTANCE m_hInstance;
    HWND m_hwnd;
    bool m_isFullscreen;
    RECT m_windowedRect;
    DWORD m_windowedStyle;
    
    // Components
    std::unique_ptr<VideoRenderer> m_videoRenderer;
    std::unique_ptr<SimpleVideoRenderer> m_simpleVideoRenderer;
    std::unique_ptr<NetworkReceiver> m_networkReceiver;
    std::unique_ptr<InputHandler> m_inputHandler;
    bool m_usingSimpleRenderer;
    
    // Connection state
    enum class ConnectionState {
        Disconnected,
        Connecting,
        Connected,
        Error
    } m_connectionState;
    
    std::string m_serverIP;
    int m_serverPort;
    std::string m_statusMessage;
    
    // Window procedures
    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    
    // Event handlers
    void OnCreate();
    void OnDestroy();
    void OnPaint();
    void OnSize(UINT width, UINT height);
    void OnKeyDown(WPARAM key);
    void OnCommand(WPARAM wParam);
    
    // Connection management
    void ShowConnectionDialog();
    void ConnectToServer(const std::string& ip, int port);
    void DisconnectFromServer();
    void OnFrameReceived(const struct FrameMessage& frameMsg, const std::vector<uint8_t>& frameData);
    void OnNetworkError(const std::string& error);
    void OnNetworkDisconnected();
    
    // UI helpers
    void UpdateWindowTitle();
    void ToggleFullscreen();
    RECT GetCenteredRect(int width, int height);
    
public:
    WindowManager();
    ~WindowManager();
    
    bool Initialize(HINSTANCE hInstance);
    void Cleanup();
    
    HWND GetHwnd() const { return m_hwnd; }
    int Run();
    
    // Connection interface
    void SetServer(const std::string& ip, int port) {
        m_serverIP = ip;
        m_serverPort = port;
    }
};