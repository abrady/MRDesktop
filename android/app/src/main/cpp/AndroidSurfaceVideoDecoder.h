#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>
#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>
#include "protocol.h"
#include "IVideoDecoder.h"

class AndroidSurfaceVideoDecoder : public IVideoDecoder {
private:
    AMediaCodec* m_codec = nullptr;
    AMediaFormat* m_format = nullptr;
    ANativeWindow* m_surface = nullptr;
    std::mutex m_decoderMutex;

    uint32_t m_width = 0;
    uint32_t m_height = 0;
    CompressionType m_compressionType = COMPRESSION_NONE;
    bool m_isInitialized = false;
    bool m_isShuttingDown = false;

    // JNI references for Surface creation
    JavaVM* m_javaVM = nullptr;
    jobject m_surfaceTexture = nullptr;
    jobject m_surface_java = nullptr;

    const char* GetMimeType(CompressionType type);

public:
    AndroidSurfaceVideoDecoder();
    ~AndroidSurfaceVideoDecoder() override;

    // IVideoDecoder interface
    bool Initialize(uint32_t width, uint32_t height, CompressionType compression) override;
    bool DecodeFrame(
        const uint8_t* compressedData,
        size_t dataSize,
        std::vector<uint8_t>& rgbaData) override;
    void Cleanup() override;
    bool IsInitialized() const override { return m_isInitialized; }
    
    // IVideoDecoder interface getters
    uint32_t GetWidth() const override { return m_width; }
    uint32_t GetHeight() const override { return m_height; }
    CompressionType GetCompressionType() const override { return m_compressionType; }

    // Surface-specific methods
    bool InitializeWithSurface(uint32_t width, uint32_t height, CompressionType compression, ANativeWindow* surface);
    bool CreateSurfaceTexture(JNIEnv* env);
    ANativeWindow* GetSurface() const { return m_surface; }
    
    // New Surface-based decode method that doesn't return pixel data
    bool DecodeFrameToSurface(const uint8_t* compressedData, size_t dataSize);
    
    // Get texture ID for GL_TEXTURE_EXTERNAL_OES
    uint32_t GetTextureId() const;
    void UpdateTexImage();
};