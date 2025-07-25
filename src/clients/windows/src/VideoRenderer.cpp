#include "VideoRenderer.h"
#include <iostream>

VideoRenderer::VideoRenderer()
    : m_hwnd(nullptr)
    , m_pD2DFactory(nullptr)
    , m_pRenderTarget(nullptr)
    , m_pFrameBitmap(nullptr)
    , m_pDWriteFactory(nullptr)
    , m_pTextFormat(nullptr)
    , m_pTextBrush(nullptr)
    , m_frameCount(0)
    , m_currentWidth(0)
    , m_currentHeight(0)
    , m_fps(0.0) {
    
    QueryPerformanceFrequency(&m_frequency);
    QueryPerformanceCounter(&m_lastFrameTime);
}

VideoRenderer::~VideoRenderer() {
    Cleanup();
}

HRESULT VideoRenderer::Initialize(HWND hwnd) {
    m_hwnd = hwnd;
    
    HRESULT hr = CreateDeviceIndependentResources();
    if (SUCCEEDED(hr)) {
        hr = CreateDeviceResources();
    }
    
    return hr;
}

void VideoRenderer::Cleanup() {
    DiscardDeviceResources();
    
    if (m_pDWriteFactory) {
        m_pDWriteFactory->Release();
        m_pDWriteFactory = nullptr;
    }
    
    if (m_pD2DFactory) {
        m_pD2DFactory->Release();
        m_pD2DFactory = nullptr;
    }
}

HRESULT VideoRenderer::CreateDeviceIndependentResources() {
    // Create Direct2D factory
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
    if (FAILED(hr)) {
        std::cout << "Failed to create Direct2D factory. HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
        return hr;
    }
    std::cout << "Direct2D factory created successfully" << std::endl;
    
    // Create DirectWrite factory
    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(m_pDWriteFactory),
        reinterpret_cast<IUnknown**>(&m_pDWriteFactory)
    );
    if (FAILED(hr)) {
        std::cout << "Failed to create DirectWrite factory. HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
        return hr;
    }
    std::cout << "DirectWrite factory created successfully" << std::endl;
    
    // Create text format
    hr = m_pDWriteFactory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        12.0f,
        L"",
        &m_pTextFormat
    );
    if (FAILED(hr)) {
        std::cout << "Failed to create text format. HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
        return hr;
    }
    std::cout << "Text format created successfully" << std::endl;
    
    return hr;
}

HRESULT VideoRenderer::CreateDeviceResources() {
    if (m_pRenderTarget) return S_OK;
    
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    
    D2D1_SIZE_U size = D2D1::SizeU(
        rc.right - rc.left,
        rc.bottom - rc.top
    );
    
    std::cout << "Creating render target for window size: " << std::dec << size.width << "x" << size.height << std::endl;
    
    // Check for invalid window size
    if (size.width == 0 || size.height == 0) {
        std::cout << "Window has zero size, cannot create render target yet" << std::endl;
        return E_FAIL;
    }
    
    // Create render target
    HRESULT hr = m_pD2DFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(m_hwnd, size),
        &m_pRenderTarget
    );
    
    if (FAILED(hr)) {
        std::cout << "Failed to create HWND render target. HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
        return hr;
    }
    std::cout << "HWND render target created successfully" << std::endl;
    
    // Create text brush
    hr = m_pRenderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::White),
        &m_pTextBrush
    );
    if (FAILED(hr)) {
        std::cout << "Failed to create text brush. HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
        return hr;
    }
    std::cout << "Text brush created successfully" << std::endl;
    
    return hr;
}

void VideoRenderer::DiscardDeviceResources() {
    if (m_pFrameBitmap) {
        m_pFrameBitmap->Release();
        m_pFrameBitmap = nullptr;
    }
    
    if (m_pTextBrush) {
        m_pTextBrush->Release();
        m_pTextBrush = nullptr;
    }
    
    if (m_pRenderTarget) {
        m_pRenderTarget->Release();
        m_pRenderTarget = nullptr;
    }
    
    if (m_pTextFormat) {
        m_pTextFormat->Release();
        m_pTextFormat = nullptr;
    }
}

HRESULT VideoRenderer::RenderFrame(const FrameMessage& frameMsg, const std::vector<BYTE>& frameData) {
    HRESULT hr = CreateDeviceResources();
    if (FAILED(hr)) return hr;
    
    // Update frame statistics
    m_frameCount++;
    m_currentWidth = frameMsg.width;
    m_currentHeight = frameMsg.height;
    CalculateFPS();
    
    // Create or update bitmap if needed
    if (!m_pFrameBitmap || 
        m_pFrameBitmap->GetSize().width != frameMsg.width ||
        m_pFrameBitmap->GetSize().height != frameMsg.height) {
        
        if (m_pFrameBitmap) {
            m_pFrameBitmap->Release();
            m_pFrameBitmap = nullptr;
        }
        
        // Create bitmap with BGRA format (matches desktop duplication format)
        D2D1_BITMAP_PROPERTIES bitmapProps = D2D1::BitmapProperties(
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)
        );
        
        hr = m_pRenderTarget->CreateBitmap(
            D2D1::SizeU(frameMsg.width, frameMsg.height),
            bitmapProps,
            &m_pFrameBitmap
        );
        
        if (FAILED(hr)) return hr;
    }
    
    // Calculate stride (bytes per row)
    UINT stride = frameMsg.width * 4; // 4 bytes per pixel (BGRA)
    
    // Update bitmap with new frame data
    D2D1_RECT_U updateRect = D2D1::RectU(0, 0, frameMsg.width, frameMsg.height);
    hr = m_pFrameBitmap->CopyFromMemory(&updateRect, frameData.data(), stride);
    if (FAILED(hr)) return hr;
    
    // Get render target size
    D2D1_SIZE_F renderTargetSize = m_pRenderTarget->GetSize();
    
    // Calculate scaling to fit the frame in the window while maintaining aspect ratio
    float scaleX = renderTargetSize.width / frameMsg.width;
    float scaleY = renderTargetSize.height / frameMsg.height;
    float scale = std::min(scaleX, scaleY);
    
    float scaledWidth = frameMsg.width * scale;
    float scaledHeight = frameMsg.height * scale;
    
    // Center the image
    float offsetX = (renderTargetSize.width - scaledWidth) / 2.0f;
    float offsetY = (renderTargetSize.height - scaledHeight) / 2.0f;
    
    // Begin drawing
    m_pRenderTarget->BeginDraw();
    
    // Clear background to black
    m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));
    
    // Draw the frame
    D2D1_RECT_F destRect = D2D1::RectF(
        offsetX,
        offsetY,
        offsetX + scaledWidth,
        offsetY + scaledHeight
    );
    
    m_pRenderTarget->DrawBitmap(
        m_pFrameBitmap,
        destRect,
        1.0f,
        D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
    );
    
    // Draw statistics overlay
    wchar_t statsText[256];
    swprintf_s(statsText, L"FPS: %.1f\nFrames: %u\nResolution: %ux%u\nData: %.1f MB/s",
               m_fps, m_frameCount, m_currentWidth, m_currentHeight,
               (m_fps * frameMsg.dataSize) / (1024.0 * 1024.0));
    
    D2D1_RECT_F textRect = D2D1::RectF(10, 10, 300, 100);
    
    // Draw text background
    ID2D1SolidColorBrush* bgBrush = nullptr;
    m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 0.7f), &bgBrush);
    if (bgBrush) {
        m_pRenderTarget->FillRectangle(textRect, bgBrush);
        bgBrush->Release();
    }
    
    // Draw text
    m_pRenderTarget->DrawText(
        statsText,
        static_cast<UINT32>(wcslen(statsText)),
        m_pTextFormat,
        textRect,
        m_pTextBrush
    );
    
    // End drawing
    hr = m_pRenderTarget->EndDraw();
    
    // Handle device lost
    if (hr == D2DERR_RECREATE_TARGET) {
        DiscardDeviceResources();
        hr = S_OK;
    }
    
    return hr;
}

void VideoRenderer::OnResize(UINT width, UINT height) {
    if (m_pRenderTarget) {
        m_pRenderTarget->Resize(D2D1::SizeU(width, height));
    }
}

void VideoRenderer::CalculateFPS() {
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    
    // Calculate FPS every 10 frames for more responsive updates
    if (m_frameCount % 10 == 0 && m_frameCount > 0) {
        double elapsedSeconds = static_cast<double>(currentTime.QuadPart - m_lastFrameTime.QuadPart) / m_frequency.QuadPart;
        m_fps = 10.0 / elapsedSeconds;
        m_lastFrameTime = currentTime;
    }
}