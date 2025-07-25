#include "SimpleVideoRenderer.h"
#include <iostream>

SimpleVideoRenderer::SimpleVideoRenderer()
    : m_hwnd(nullptr), m_hdc(nullptr), m_memDC(nullptr), m_bitmap(nullptr), m_oldBitmap(nullptr), m_frameCount(0), m_currentWidth(0), m_currentHeight(0), m_fps(0.0)
{

    QueryPerformanceFrequency(&m_frequency);
    QueryPerformanceCounter(&m_lastFrameTime);
}

SimpleVideoRenderer::~SimpleVideoRenderer()
{
    Cleanup();
}

HRESULT SimpleVideoRenderer::Initialize(HWND hwnd)
{
    m_hwnd = hwnd;

    m_hdc = GetDC(hwnd);
    if (!m_hdc)
    {
        return E_FAIL;
    }

    m_memDC = CreateCompatibleDC(m_hdc);
    if (!m_memDC)
    {
        ReleaseDC(hwnd, m_hdc);
        return E_FAIL;
    }

    return S_OK;
}

void SimpleVideoRenderer::Cleanup()
{
    if (m_oldBitmap)
    {
        SelectObject(m_memDC, m_oldBitmap);
        m_oldBitmap = nullptr;
    }

    if (m_bitmap)
    {
        DeleteObject(m_bitmap);
        m_bitmap = nullptr;
    }

    if (m_memDC)
    {
        DeleteDC(m_memDC);
        m_memDC = nullptr;
    }

    if (m_hdc)
    {
        ReleaseDC(m_hwnd, m_hdc);
        m_hdc = nullptr;
    }
}

HRESULT SimpleVideoRenderer::RenderFrame(const FrameMessage &frameMsg, const std::vector<BYTE> &frameData)
{
    if (!m_hdc || !m_memDC)
        return E_FAIL;

    // Update frame statistics
    m_frameCount++;
    m_currentWidth = frameMsg.width;
    m_currentHeight = frameMsg.height;
    CalculateFPS();

    // Create or recreate bitmap if size changed
    if (!m_bitmap || m_bitmapWidth != frameMsg.width || m_bitmapHeight != frameMsg.height)
    {
        if (m_oldBitmap)
        {
            SelectObject(m_memDC, m_oldBitmap);
            m_oldBitmap = nullptr;
        }

        if (m_bitmap)
        {
            DeleteObject(m_bitmap);
            m_bitmap = nullptr;
        }

        // Create DIB for BGRA data
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = frameMsg.width;
        bmi.bmiHeader.biHeight = -(int)frameMsg.height; // Negative for top-down
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        void *pBits = nullptr;
        m_bitmap = CreateDIBSection(m_memDC, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
        if (!m_bitmap)
        {
            return E_FAIL;
        }

        m_oldBitmap = (HBITMAP)SelectObject(m_memDC, m_bitmap);
        m_bitmapWidth = frameMsg.width;
        m_bitmapHeight = frameMsg.height;
    }

    // Copy frame data to bitmap
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = frameMsg.width;
    bmi.bmiHeader.biHeight = -(int)frameMsg.height; // Negative for top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    SetDIBits(m_memDC, m_bitmap, 0, frameMsg.height, frameData.data(), &bmi, DIB_RGB_COLORS);

    // Get client area
    RECT clientRect;
    GetClientRect(m_hwnd, &clientRect);
    int clientWidth = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;

    // Calculate scaling to maintain aspect ratio
    float scaleX = (float)clientWidth / frameMsg.width;
    float scaleY = (float)clientHeight / frameMsg.height;
    float scale = std::min(scaleX, scaleY);

    int scaledWidth = (int)(frameMsg.width * scale);
    int scaledHeight = (int)(frameMsg.height * scale);

    int offsetX = (clientWidth - scaledWidth) / 2;
    int offsetY = (clientHeight - scaledHeight) / 2;

    // Clear background
    HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(m_hdc, &clientRect, blackBrush);
    DeleteObject(blackBrush);

    // Draw scaled image
    StretchBlt(m_hdc, offsetX, offsetY, scaledWidth, scaledHeight,
               m_memDC, 0, 0, frameMsg.width, frameMsg.height, SRCCOPY);

    // Draw statistics
    char statsText[256];
    sprintf_s(statsText, "FPS: %.1f | Frames: %u | Resolution: %ux%u | Data: %.1f MB/s",
              m_fps, m_frameCount, m_currentWidth, m_currentHeight,
              (m_fps * frameMsg.dataSize) / (1024.0 * 1024.0));

    SetBkColor(m_hdc, RGB(0, 0, 0));
    SetTextColor(m_hdc, RGB(255, 255, 255));

    RECT textRect = {10, 10, 400, 80};
    DrawTextA(m_hdc, statsText, -1, &textRect, DT_LEFT | DT_TOP | DT_WORDBREAK);

    return S_OK;
}

void SimpleVideoRenderer::OnResize(UINT /*width*/, UINT /*height*/)
{
    // Handle window resize if needed
    // GDI handles resize automatically
}

void SimpleVideoRenderer::CalculateFPS()
{
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);

    // Calculate FPS every 10 frames for more responsive updates
    if (m_frameCount % 10 == 0 && m_frameCount > 0)
    {
        double elapsedSeconds = static_cast<double>(currentTime.QuadPart - m_lastFrameTime.QuadPart) / m_frequency.QuadPart;
        m_fps = 10.0 / elapsedSeconds;
        m_lastFrameTime = currentTime;
    }
}