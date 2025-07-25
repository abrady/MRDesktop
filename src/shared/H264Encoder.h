#pragma once
#include <vector>
#include <memory>
#include <windows.h>
#include <mfapi.h>
#include <mftransform.h>
#include <mfidl.h>
#include <mferror.h>
#include <wmcodecdsp.h>
#include <combaseapi.h>

class H264Encoder {
public:
    H264Encoder();
    ~H264Encoder();

    // Initialize encoder with given resolution
    bool Initialize(int width, int height, int fps = 60);
    
    // Encode a frame (BGRA format)
    bool EncodeFrame(const uint8_t* frameData, std::vector<uint8_t>& compressedData, bool& isKeyframe);
    
    // Force next frame to be keyframe
    void ForceKeyframe();
    
    // Cleanup
    void Cleanup();

private:
    IMFTransform* m_encoder;
    IMFMediaType* m_inputType;
    IMFMediaType* m_outputType;
    IMFSample* m_inputSample;
    IMFMediaBuffer* m_inputBuffer;
    
    int m_width;
    int m_height;
    int m_frameCount;
    bool m_initialized;
    bool m_forceKeyframe;
    LONGLONG m_frameTime;
    GUID m_inputFormat;
    
    // Helper methods
    HRESULT CreateInputMediaType();
    HRESULT CreateOutputMediaType();
    HRESULT ProcessFrame(const uint8_t* frameData, std::vector<uint8_t>& compressedData, bool& isKeyframe);
    void ConvertBGRAtoNV12(const uint8_t* bgra, uint8_t* nv12, int width, int height);
};