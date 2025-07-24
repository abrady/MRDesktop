#pragma once
#include <windows.h>
#include <windowsx.h>
#include <functional>

class InputHandler {
private:
    HWND m_hwnd;
    bool m_isCapturingMouse;
    POINT m_lastMousePos;
    
    // Callbacks for input events
    std::function<void(int32_t, int32_t)> m_onMouseMove;
    std::function<void(int, bool)> m_onMouseClick;
    std::function<void(int32_t, int32_t)> m_onMouseScroll;
    std::function<void(int, bool)> m_onKeyPress;
    
public:
    InputHandler();
    ~InputHandler();
    
    void Initialize(HWND hwnd);
    void Cleanup();
    
    // Input capture control
    void StartMouseCapture();
    void StopMouseCapture();
    bool IsCapturingMouse() const { return m_isCapturingMouse; }
    
    // Window message handlers
    LRESULT HandleMouseMove(WPARAM wParam, LPARAM lParam);
    LRESULT HandleMouseButton(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMouseWheel(WPARAM wParam, LPARAM lParam);
    LRESULT HandleKeyboard(UINT message, WPARAM wParam, LPARAM lParam);
    
    // Set callbacks
    void SetMouseMoveCallback(std::function<void(int32_t, int32_t)> callback) {
        m_onMouseMove = callback;
    }
    
    void SetMouseClickCallback(std::function<void(int, bool)> callback) {
        m_onMouseClick = callback;
    }
    
    void SetMouseScrollCallback(std::function<void(int32_t, int32_t)> callback) {
        m_onMouseScroll = callback;
    }
    
    void SetKeyPressCallback(std::function<void(int, bool)> callback) {
        m_onKeyPress = callback;
    }
};