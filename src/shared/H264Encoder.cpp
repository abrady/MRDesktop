#include "H264Encoder.h"
#include <iostream>
#include <codecapi.h>

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "wmcodecdspuuid.lib")

H264Encoder::H264Encoder()
    : m_encoder(nullptr)
    , m_inputType(nullptr)
    , m_outputType(nullptr)
    , m_inputSample(nullptr)
    , m_inputBuffer(nullptr)
    , m_width(0)
    , m_height(0)
    , m_frameCount(0)
    , m_initialized(false)
    , m_forceKeyframe(false)
    , m_frameTime(0) {
}

H264Encoder::~H264Encoder() {
    Cleanup();
}

bool H264Encoder::Initialize(int width, int height, int fps) {
    if (m_initialized) {
        Cleanup();
    }
    
    m_width = width;
    m_height = height;
    m_frameCount = 0;
    m_frameTime = 0;
    
    HRESULT hr = S_OK;
    
    // Initialize Media Foundation
    hr = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
    if (FAILED(hr)) {
        std::cerr << "H.264: Failed to initialize Media Foundation: " << std::hex << hr << std::endl;
        return false;
    }
    
    // Create H.264 encoder
    hr = CoCreateInstance(CLSID_CMSH264EncoderMFT, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_encoder));
    if (FAILED(hr)) {
        std::cerr << "H.264: Failed to create encoder: " << std::hex << hr << std::endl;
        MFShutdown();
        return false;
    }
    
    // Create input media type
    hr = CreateInputMediaType();
    if (FAILED(hr)) {
        std::cerr << "H.264: Failed to create input media type: " << std::hex << hr << std::endl;
        Cleanup();
        return false;
    }
    
    // Create output media type
    hr = CreateOutputMediaType();
    if (FAILED(hr)) {
        std::cerr << "H.264: Failed to create output media type: " << std::hex << hr << std::endl;
        Cleanup();
        return false;
    }
    
    // Set encoder properties for low latency
    ICodecAPI* codecAPI = nullptr;
    hr = m_encoder->QueryInterface(IID_PPV_ARGS(&codecAPI));
    if (SUCCEEDED(hr)) {
        VARIANT var;
        VariantInit(&var);
        
        // Set CBR mode
        var.vt = VT_UI4;
        var.ulVal = eAVEncCommonRateControlMode_CBR;
        codecAPI->SetValue(&CODECAPI_AVEncCommonRateControlMode, &var);
        
        // Set bitrate (5 Mbps)
        var.ulVal = 5000000;
        codecAPI->SetValue(&CODECAPI_AVEncCommonMeanBitRate, &var);
        
        // Low latency mode
        var.vt = VT_BOOL;
        var.boolVal = VARIANT_TRUE;
        codecAPI->SetValue(&CODECAPI_AVLowLatencyMode, &var);
        
        // Set GOP size (2 seconds worth of frames)
        var.vt = VT_UI4;
        var.ulVal = fps * 2;
        codecAPI->SetValue(&CODECAPI_AVEncMPVGOPSize, &var);
        
        VariantClear(&var);
        codecAPI->Release();
    }
    
    // Process messages to start encoder
    hr = m_encoder->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0);
    hr = m_encoder->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
    hr = m_encoder->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
    
    m_initialized = true;
    std::cout << "H.264 Encoder initialized: " << width << "x" << height << " @ " << fps << " FPS" << std::endl;
    return true;
}

HRESULT H264Encoder::CreateInputMediaType() {
    HRESULT hr = MFCreateMediaType(&m_inputType);
    if (FAILED(hr)) return hr;
    
    hr = m_inputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (FAILED(hr)) return hr;
    
    hr = m_inputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    if (FAILED(hr)) return hr;
    
    hr = MFSetAttributeSize(m_inputType, MF_MT_FRAME_SIZE, m_width, m_height);
    if (FAILED(hr)) return hr;
    
    hr = MFSetAttributeRatio(m_inputType, MF_MT_FRAME_RATE, 60, 1);
    if (FAILED(hr)) return hr;
    
    hr = m_inputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    if (FAILED(hr)) return hr;
    
    return m_encoder->SetInputType(0, m_inputType, 0);
}

HRESULT H264Encoder::CreateOutputMediaType() {
    HRESULT hr = MFCreateMediaType(&m_outputType);
    if (FAILED(hr)) return hr;
    
    hr = m_outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (FAILED(hr)) return hr;
    
    hr = m_outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    if (FAILED(hr)) return hr;
    
    hr = MFSetAttributeSize(m_outputType, MF_MT_FRAME_SIZE, m_width, m_height);
    if (FAILED(hr)) return hr;
    
    hr = MFSetAttributeRatio(m_outputType, MF_MT_FRAME_RATE, 60, 1);
    if (FAILED(hr)) return hr;
    
    hr = m_outputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    if (FAILED(hr)) return hr;
    
    return m_encoder->SetOutputType(0, m_outputType, 0);
}

bool H264Encoder::EncodeFrame(const uint8_t* frameData, std::vector<uint8_t>& compressedData, bool& isKeyframe) {
    if (!m_initialized) {
        return false;
    }
    
    HRESULT hr = S_OK;
    
    // Create input sample if needed
    if (!m_inputSample) {
        hr = MFCreateSample(&m_inputSample);
        if (FAILED(hr)) return false;
        
        DWORD bufferSize = m_width * m_height * 4; // BGRA
        hr = MFCreateMemoryBuffer(bufferSize, &m_inputBuffer);
        if (FAILED(hr)) return false;
        
        hr = m_inputSample->AddBuffer(m_inputBuffer);
        if (FAILED(hr)) return false;
    }
    
    // Copy frame data to buffer
    BYTE* bufferData = nullptr;
    DWORD maxLength = 0;
    hr = m_inputBuffer->Lock(&bufferData, &maxLength, nullptr);
    if (FAILED(hr)) return false;
    
    DWORD frameSize = m_width * m_height * 4;
    memcpy(bufferData, frameData, frameSize);
    
    hr = m_inputBuffer->Unlock();
    if (FAILED(hr)) return false;
    
    hr = m_inputBuffer->SetCurrentLength(frameSize);
    if (FAILED(hr)) return false;
    
    // Set timestamp
    hr = m_inputSample->SetSampleTime(m_frameTime);
    if (FAILED(hr)) return false;
    
    hr = m_inputSample->SetSampleDuration(333333); // ~30fps duration
    if (FAILED(hr)) return false;
    
    // Force keyframe if requested
    if (m_forceKeyframe || m_frameCount == 0) {
        hr = m_inputSample->SetUINT32(MFSampleExtension_CleanPoint, TRUE);
        m_forceKeyframe = false;
    }
    
    // Process input
    hr = m_encoder->ProcessInput(0, m_inputSample, 0);
    if (FAILED(hr)) {
        std::cerr << "H.264: Failed to process input: " << std::hex << hr << std::endl;
        return false;
    }
    
    // Get output
    return ProcessFrame(frameData, compressedData, isKeyframe);
}

HRESULT H264Encoder::ProcessFrame(const uint8_t* frameData, std::vector<uint8_t>& compressedData, bool& isKeyframe) {
    MFT_OUTPUT_DATA_BUFFER outputData = {};
    outputData.dwStreamID = 0;
    
    HRESULT hr = MFCreateSample(&outputData.pSample);
    if (FAILED(hr)) return hr;
    
    IMFMediaBuffer* outputBuffer = nullptr;
    hr = MFCreateMemoryBuffer(m_width * m_height, &outputBuffer);
    if (FAILED(hr)) {
        outputData.pSample->Release();
        return hr;
    }
    
    hr = outputData.pSample->AddBuffer(outputBuffer);
    if (FAILED(hr)) {
        outputBuffer->Release();
        outputData.pSample->Release();
        return hr;
    }
    
    DWORD status = 0;
    hr = m_encoder->ProcessOutput(0, 1, &outputData, &status);
    
    if (SUCCEEDED(hr)) {
        // Extract compressed data
        BYTE* bufferData = nullptr;
        DWORD dataLength = 0;
        hr = outputBuffer->Lock(&bufferData, nullptr, &dataLength);
        if (SUCCEEDED(hr)) {
            compressedData.resize(dataLength);
            memcpy(compressedData.data(), bufferData, dataLength);
            outputBuffer->Unlock();
            
            // Check if it's a keyframe
            UINT32 cleanPoint = 0;
            hr = outputData.pSample->GetUINT32(MFSampleExtension_CleanPoint, &cleanPoint);
            isKeyframe = (SUCCEEDED(hr) && cleanPoint);
        }
    }
    
    outputBuffer->Release();
    outputData.pSample->Release();
    
    m_frameCount++;
    m_frameTime += 333333; // Increment timestamp
    
    return hr;
}

void H264Encoder::ForceKeyframe() {
    m_forceKeyframe = true;
}

void H264Encoder::Cleanup() {
    if (m_initialized) {
        if (m_encoder) {
            m_encoder->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0);
            m_encoder->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, 0);
        }
        
        if (m_inputBuffer) {
            m_inputBuffer->Release();
            m_inputBuffer = nullptr;
        }
        
        if (m_inputSample) {
            m_inputSample->Release();
            m_inputSample = nullptr;
        }
        
        if (m_inputType) {
            m_inputType->Release();
            m_inputType = nullptr;
        }
        
        if (m_outputType) {
            m_outputType->Release();
            m_outputType = nullptr;
        }
        
        if (m_encoder) {
            m_encoder->Release();
            m_encoder = nullptr;
        }
        
        MFShutdown();
        m_initialized = false;
    }
}