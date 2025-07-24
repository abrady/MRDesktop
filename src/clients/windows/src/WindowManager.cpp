#include "WindowManager.h"
#include "VideoRenderer.h"
#include "SimpleVideoRenderer.h"
#include "NetworkClient.h"
#include "InputHandler.h"
#include "Protocol.h"
#include <commdlg.h>
#include <sstream>
#include <iostream>

WindowManager::WindowManager()
    : m_hInstance(nullptr)
    , m_hwnd(nullptr)
    , m_isFullscreen(false)
    , m_usingSimpleRenderer(false)
    , m_connectionState(ConnectionState::Disconnected)
    , m_serverIP("127.0.0.1")
    , m_serverPort(8080)
    , m_statusMessage("Disconnected") {
}

WindowManager::~WindowManager() {
    Cleanup();
}

bool WindowManager::Initialize(HINSTANCE hInstance) {
    m_hInstance = hInstance;
    
    // Register window class
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = StaticWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = WINDOW_CLASS_NAME;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    
    if (!RegisterClassExW(&wcex)) {
        return false;
    }
    
    // Create window
    RECT windowRect = GetCenteredRect(1280, 720);
    
    m_hwnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        WINDOW_CLASS_NAME,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        windowRect.left, windowRect.top,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        hInstance,
        this
    );
    
    if (!m_hwnd) {
        return false;
    }
    
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);
    
    return true;
}

void WindowManager::Cleanup() {
    DisconnectFromServer();
    
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
    
    if (m_hInstance) {
        UnregisterClassW(WINDOW_CLASS_NAME, m_hInstance);
    }
}

int WindowManager::Run() {
    MSG msg;
    
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WindowManager::StaticWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    WindowManager* pThis = nullptr;
    
    if (message == WM_NCCREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<WindowManager*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    } else {
        pThis = reinterpret_cast<WindowManager*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    if (pThis) {
        return pThis->WndProc(hwnd, message, wParam, lParam);
    }
    
    return DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT WindowManager::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            OnCreate();
            return 0;
            
        case WM_DESTROY:
            OnDestroy();
            return 0;
            
        case WM_PAINT:
            OnPaint();
            return 0;
            
        case WM_SIZE:
            OnSize(LOWORD(lParam), HIWORD(lParam));
            return 0;
            
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
            if (m_inputHandler) {
                m_inputHandler->HandleKeyboard(message, wParam, lParam);
            }
            if (message == WM_KEYDOWN) {
                OnKeyDown(wParam);
            }
            return 0;
            
        case WM_MOUSEMOVE:
            if (m_inputHandler) {
                m_inputHandler->HandleMouseMove(wParam, lParam);
            }
            return 0;
            
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            if (m_inputHandler) {
                m_inputHandler->HandleMouseButton(message, wParam, lParam);
            }
            return 0;
            
        case WM_MOUSEWHEEL:
            if (m_inputHandler) {
                m_inputHandler->HandleMouseWheel(wParam, lParam);
            }
            return 0;
            
        case WM_COMMAND:
            OnCommand(wParam);
            return 0;
    }
    
    return DefWindowProc(hwnd, message, wParam, lParam);
}

void WindowManager::OnCreate() {
    // Initialize components
    m_videoRenderer = std::make_unique<VideoRenderer>();
    m_simpleVideoRenderer = std::make_unique<SimpleVideoRenderer>();
    m_networkClient = std::make_unique<NetworkClient>();
    m_inputHandler = std::make_unique<InputHandler>();
    
    // Try to initialize Direct2D video renderer first
    HRESULT hr = m_videoRenderer->Initialize(m_hwnd);
    if (FAILED(hr)) {
        // Fall back to simple GDI renderer
        hr = m_simpleVideoRenderer->Initialize(m_hwnd);
        if (FAILED(hr)) {
            MessageBoxW(m_hwnd, L"Failed to initialize both Direct2D and GDI video renderers.\nThis is a critical error.", L"Video Renderer Error", MB_OK | MB_ICONERROR);
            PostQuitMessage(1);
            return;
        }
        m_usingSimpleRenderer = true;
        std::cout << "Direct2D not available - using GDI fallback renderer. Performance may be reduced." << std::endl;
    } else {
        m_usingSimpleRenderer = false;
    }
    
    // Initialize input handler
    m_inputHandler->Initialize(m_hwnd);
    
    // Set up network callbacks
    m_networkClient->SetFrameCallback([this](const FrameMessage& frameMsg, const std::vector<BYTE>& frameData) {
        OnFrameReceived(frameMsg, frameData);
    });
    
    m_networkClient->SetErrorCallback([this](const std::string& error) {
        OnNetworkError(error);
    });
    
    m_networkClient->SetDisconnectedCallback([this]() {
        OnNetworkDisconnected();
    });
    
    // Set up input callbacks
    m_inputHandler->SetMouseMoveCallback([this](int32_t deltaX, int32_t deltaY) {
        if (m_networkClient && m_networkClient->IsConnected()) {
            m_networkClient->SendMouseMove(deltaX, deltaY);
        }
    });
    
    m_inputHandler->SetMouseClickCallback([this](int button, bool pressed) {
        if (m_networkClient && m_networkClient->IsConnected()) {
            MouseClickMessage::MouseButton btn = static_cast<MouseClickMessage::MouseButton>(button);
            m_networkClient->SendMouseClick(btn, pressed);
        }
    });
    
    m_inputHandler->SetMouseScrollCallback([this](int32_t deltaX, int32_t deltaY) {
        if (m_networkClient && m_networkClient->IsConnected()) {
            m_networkClient->SendMouseScroll(deltaX, deltaY);
        }
    });
    
    // Connect automatically
    ConnectToServer(m_serverIP, m_serverPort);
}

void WindowManager::OnDestroy() {
    PostQuitMessage(0);
}

void WindowManager::OnPaint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);
    
    if (m_connectionState == ConnectionState::Connected && m_videoRenderer) {
        // Video renderer handles its own drawing
    } else {
        // Draw connection status
        RECT clientRect;
        GetClientRect(m_hwnd, &clientRect);
        
        SetBkColor(hdc, RGB(0, 0, 0));
        SetTextColor(hdc, RGB(255, 255, 255));
        
        std::string statusText = "MRDesktop Windows Client\\n\\nStatus: " + m_statusMessage;
        if (m_connectionState == ConnectionState::Disconnected) {
            statusText += "\\n\\nPress F11 for fullscreen\\nPress ESC to toggle mouse capture";
        }
        
        DrawTextA(hdc, statusText.c_str(), -1, &clientRect, 
                 DT_CENTER | DT_VCENTER | DT_WORDBREAK);
    }
    
    EndPaint(m_hwnd, &ps);
}

void WindowManager::OnSize(UINT width, UINT height) {
    if (m_usingSimpleRenderer && m_simpleVideoRenderer) {
        m_simpleVideoRenderer->OnResize(width, height);
    } else if (!m_usingSimpleRenderer && m_videoRenderer) {
        m_videoRenderer->OnResize(width, height);
    }
}

void WindowManager::OnKeyDown(WPARAM key) {
    switch (key) {
        case VK_F11:
            ToggleFullscreen();
            break;
            
        case VK_ESCAPE:
            if (m_connectionState == ConnectionState::Connected) {
                DisconnectFromServer();
            }
            break;
    }
}

void WindowManager::OnCommand(WPARAM wParam) {
    // Handle menu commands if needed
}

void WindowManager::ShowConnectionDialog() {
    // Simple input dialog using Windows API
    // For now, use default values and connect immediately
    // In a full implementation, you'd create a proper dialog
    
    m_statusMessage = "Press SPACE to connect to " + m_serverIP + ":" + std::to_string(m_serverPort);
    UpdateWindowTitle();
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void WindowManager::ConnectToServer(const std::string& ip, int port) {
    if (m_connectionState != ConnectionState::Disconnected) return;
    
    std::cout << "Connecting to " << ip << ":" << port << "..." << std::endl;
    
    m_connectionState = ConnectionState::Connecting;
    m_statusMessage = "Connecting to " + ip + ":" + std::to_string(port) + "...";
    UpdateWindowTitle();
    InvalidateRect(m_hwnd, nullptr, TRUE);
    
    // Connect in background thread to avoid blocking UI
    std::thread connectThread([this, ip, port]() {
        bool success = m_networkClient->Connect(ip, port);
        
        // Post result back to main thread
        PostMessage(m_hwnd, WM_USER + 1, success ? 1 : 0, 0);
    });
    connectThread.detach();
}

void WindowManager::DisconnectFromServer() {
    if (m_connectionState == ConnectionState::Disconnected) return;
    
    m_inputHandler->StopMouseCapture();
    m_networkClient->Disconnect();
    
    m_connectionState = ConnectionState::Disconnected;
    m_statusMessage = "Disconnected";
    UpdateWindowTitle();
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void WindowManager::OnFrameReceived(const FrameMessage& frameMsg, const std::vector<BYTE>& frameData) {
    if (m_usingSimpleRenderer && m_simpleVideoRenderer) {
        m_simpleVideoRenderer->RenderFrame(frameMsg, frameData);
    } else if (!m_usingSimpleRenderer && m_videoRenderer) {
        m_videoRenderer->RenderFrame(frameMsg, frameData);
    }
}

void WindowManager::OnNetworkError(const std::string& error) {
    m_connectionState = ConnectionState::Error;
    m_statusMessage = "Error: " + error;
    UpdateWindowTitle();
    InvalidateRect(m_hwnd, nullptr, TRUE);
    
    MessageBoxA(m_hwnd, error.c_str(), "Network Error", MB_OK | MB_ICONERROR);
}

void WindowManager::OnNetworkDisconnected() {
    DisconnectFromServer();
}

void WindowManager::UpdateWindowTitle() {
    std::wstring title = WINDOW_TITLE;
    
    if (m_connectionState == ConnectionState::Connected) {
        double fps = 0.0;
        uint32_t frames = 0;
        uint32_t width = 0, height = 0;
        
        if (m_usingSimpleRenderer && m_simpleVideoRenderer) {
            fps = m_simpleVideoRenderer->GetFPS();
            frames = m_simpleVideoRenderer->GetFrameCount();
            m_simpleVideoRenderer->GetCurrentResolution(width, height);
        } else if (!m_usingSimpleRenderer && m_videoRenderer) {
            fps = m_videoRenderer->GetFPS();
            frames = m_videoRenderer->GetFrameCount();
            m_videoRenderer->GetCurrentResolution(width, height);
        }
        
        wchar_t stats[256];
        swprintf_s(stats, L" - Connected%s - %.1f FPS - %ux%u - %u frames", 
                   m_usingSimpleRenderer ? L" (GDI)" : L" (D2D)", fps, width, height, frames);
        title += stats;
    } else {
        std::wstring status(m_statusMessage.begin(), m_statusMessage.end());
        title += L" - " + status;
    }
    
    SetWindowTextW(m_hwnd, title.c_str());
}

void WindowManager::ToggleFullscreen() {
    if (!m_isFullscreen) {
        // Save current window state
        GetWindowRect(m_hwnd, &m_windowedRect);
        m_windowedStyle = GetWindowLong(m_hwnd, GWL_STYLE);
        
        // Get monitor info
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfo(MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST), &mi);
        
        // Set fullscreen style
        SetWindowLong(m_hwnd, GWL_STYLE, m_windowedStyle & ~(WS_CAPTION | WS_THICKFRAME));
        SetWindowPos(m_hwnd, HWND_TOP,
                    mi.rcMonitor.left, mi.rcMonitor.top,
                    mi.rcMonitor.right - mi.rcMonitor.left,
                    mi.rcMonitor.bottom - mi.rcMonitor.top,
                    SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        
        m_isFullscreen = true;
    } else {
        // Restore windowed mode
        SetWindowLong(m_hwnd, GWL_STYLE, m_windowedStyle);
        SetWindowPos(m_hwnd, nullptr,
                    m_windowedRect.left, m_windowedRect.top,
                    m_windowedRect.right - m_windowedRect.left,
                    m_windowedRect.bottom - m_windowedRect.top,
                    SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        
        m_isFullscreen = false;
    }
}

RECT WindowManager::GetCenteredRect(int width, int height) {
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    int x = (screenWidth - width) / 2;
    int y = (screenHeight - height) / 2;
    
    return { x, y, x + width, y + height };
}