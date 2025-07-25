#pragma once
#include <windows.h>
#include <vector>
#include <algorithm>
#include "protocol.h"

class SimpleVideoRenderer {
private:
    HWND m_hwnd;
    HDC m_hdc;
    HDC m_memDC;
    HBITMAP m_bitmap;
    HBITMAP m_oldBitmap;
    
    // Bitmap dimensions
    uint32_t m_bitmapWidth = 0;
    uint32_t m_bitmapHeight = 0;
    
    // Frame statistics
    uint32_t m_frameCount;
    uint32_t m_currentWidth;
    uint32_t m_currentHeight;
    double m_fps;
    LARGE_INTEGER m_lastFrameTime;
    LARGE_INTEGER m_frequency;
    
    void CalculateFPS();
    
public:
    SimpleVideoRenderer();
    ~SimpleVideoRenderer();
    
    HRESULT Initialize(HWND hwnd);
    void Cleanup();
    
    HRESULT RenderFrame(const FrameMessage& frameMsg, const std::vector<BYTE>& frameData);
    void OnResize(UINT width, UINT height);
    
    // Statistics
    double GetFPS() const { return m_fps; }
    uint32_t GetFrameCount() const { return m_frameCount; }
    void GetCurrentResolution(uint32_t& width, uint32_t& height) const {
        width = m_currentWidth;
        height = m_currentHeight;
    }
};