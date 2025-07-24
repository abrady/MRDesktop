#pragma once
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <vector>
#include <memory>
#include <algorithm>
#include "Protocol.h"

class VideoRenderer {
private:
    HWND m_hwnd;
    ID2D1Factory* m_pD2DFactory;
    ID2D1HwndRenderTarget* m_pRenderTarget;
    ID2D1Bitmap* m_pFrameBitmap;
    IDWriteFactory* m_pDWriteFactory;
    IDWriteTextFormat* m_pTextFormat;
    ID2D1SolidColorBrush* m_pTextBrush;
    
    // Frame statistics
    uint32_t m_frameCount;
    uint32_t m_currentWidth;
    uint32_t m_currentHeight;
    double m_fps;
    LARGE_INTEGER m_lastFrameTime;
    LARGE_INTEGER m_frequency;
    
    HRESULT CreateDeviceIndependentResources();
    HRESULT CreateDeviceResources();
    void DiscardDeviceResources();
    void CalculateFPS();
    
public:
    VideoRenderer();
    ~VideoRenderer();
    
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