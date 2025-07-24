#include "InputHandler.h"

InputHandler::InputHandler()
    : m_hwnd(nullptr)
    , m_isCapturingMouse(false)
    , m_lastMousePos({0, 0}) {
}

InputHandler::~InputHandler() {
    Cleanup();
}

void InputHandler::Initialize(HWND hwnd) {
    m_hwnd = hwnd;
}

void InputHandler::Cleanup() {
    StopMouseCapture();
}

void InputHandler::StartMouseCapture() {
    if (m_isCapturingMouse || !m_hwnd) return;
    
    // Capture mouse to this window
    SetCapture(m_hwnd);
    
    // Hide cursor and confine to window
    ShowCursor(FALSE);
    
    RECT clientRect;
    GetClientRect(m_hwnd, &clientRect);
    ClientToScreen(m_hwnd, reinterpret_cast<POINT*>(&clientRect.left));
    ClientToScreen(m_hwnd, reinterpret_cast<POINT*>(&clientRect.right));
    ClipCursor(&clientRect);
    
    // Get initial mouse position
    GetCursorPos(&m_lastMousePos);
    ScreenToClient(m_hwnd, &m_lastMousePos);
    
    m_isCapturingMouse = true;
}

void InputHandler::StopMouseCapture() {
    if (!m_isCapturingMouse) return;
    
    ReleaseCapture();
    ShowCursor(TRUE);
    ClipCursor(nullptr);
    
    m_isCapturingMouse = false;
}

LRESULT InputHandler::HandleMouseMove(WPARAM wParam, LPARAM lParam) {
    if (!m_isCapturingMouse || !m_onMouseMove) return 0;
    
    POINT currentPos = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    
    // Calculate delta movement
    int32_t deltaX = currentPos.x - m_lastMousePos.x;
    int32_t deltaY = currentPos.y - m_lastMousePos.y;
    
    // Only send if there's actual movement
    if (deltaX != 0 || deltaY != 0) {
        m_onMouseMove(deltaX, deltaY);
        m_lastMousePos = currentPos;
    }
    
    return 0;
}

LRESULT InputHandler::HandleMouseButton(UINT message, WPARAM wParam, LPARAM lParam) {
    if (!m_isCapturingMouse || !m_onMouseClick) return 0;
    
    int button = 0;
    bool pressed = false;
    
    switch (message) {
        case WM_LBUTTONDOWN:
            button = 1; // LEFT_BUTTON
            pressed = true;
            break;
        case WM_LBUTTONUP:
            button = 1;
            pressed = false;
            break;
        case WM_RBUTTONDOWN:
            button = 2; // RIGHT_BUTTON
            pressed = true;
            break;
        case WM_RBUTTONUP:
            button = 2;
            pressed = false;
            break;
        case WM_MBUTTONDOWN:
            button = 4; // MIDDLE_BUTTON
            pressed = true;
            break;
        case WM_MBUTTONUP:
            button = 4;
            pressed = false;
            break;
        default:
            return 0;
    }
    
    m_onMouseClick(button, pressed);
    return 0;
}

LRESULT InputHandler::HandleMouseWheel(WPARAM wParam, LPARAM lParam) {
    if (!m_isCapturingMouse || !m_onMouseScroll) return 0;
    
    int32_t delta = GET_WHEEL_DELTA_WPARAM(wParam);
    
    // Convert to scroll units (typically 120 units = 1 scroll step)
    int32_t scrollY = delta / WHEEL_DELTA;
    
    m_onMouseScroll(0, scrollY);
    return 0;
}

LRESULT InputHandler::HandleKeyboard(UINT message, WPARAM wParam, LPARAM lParam) {
    if (!m_onKeyPress) return 0;
    
    bool pressed = (message == WM_KEYDOWN || message == WM_SYSKEYDOWN);
    int key = static_cast<int>(wParam);
    
    // Handle special keys that affect mouse capture
    if (pressed) {
        switch (key) {
            case VK_ESCAPE:
                // ESC key toggles mouse capture
                if (m_isCapturingMouse) {
                    StopMouseCapture();
                } else {
                    StartMouseCapture();
                }
                break;
                
            case VK_F11:
                // F11 for fullscreen toggle (handled by window manager)
                break;
        }
    }
    
    m_onKeyPress(key, pressed);
    return 0;
}