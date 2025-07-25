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
    , m_frameTime(0)
    , m_inputFormat(MFVideoFormat_RGB32) {
}

H264Encoder::~H264Encoder() {
    Cleanup();
}

bool H264Encoder::Initialize(int width, int height, int fps) {
    if (m_initialized) {
        std::cout << "H.264: Already initialized, skipping" << std::endl;
        return true;
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
    
    // Create H.264 encoder - try software encoder first for better compatibility
    hr = CoCreateInstance(CLSID_MSH264EncoderMFT, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_encoder));
    if (FAILED(hr)) {
        std::cerr << "H.264: Failed to create encoder: " << std::hex << hr << std::endl;
        MFShutdown();
        return false;
    }
    
    // Query supported input types for debugging
    std::cout << "H.264: Querying supported input types..." << std::endl;
    for (DWORD i = 0; ; i++) {
        IMFMediaType* supportedType = nullptr;
        hr = m_encoder->GetInputAvailableType(0, i, &supportedType);
        if (FAILED(hr)) break;
        
        GUID subtype;
        if (SUCCEEDED(supportedType->GetGUID(MF_MT_SUBTYPE, &subtype))) {
            if (subtype == MFVideoFormat_NV12) {
                std::cout << "H.264: Encoder supports NV12" << std::endl;
            } else if (subtype == MFVideoFormat_YUY2) {
                std::cout << "H.264: Encoder supports YUY2" << std::endl;
            } else if (subtype == MFVideoFormat_RGB32) {
                std::cout << "H.264: Encoder supports RGB32" << std::endl;
            }
        }
        supportedType->Release();
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
        std::cerr << "H.264: Failed to create output media type: " << std::hex << hr << std::dec << std::endl;
        Cleanup();
        return false;
    }
    
    std::cout << "H.264: Output media type created successfully" << std::endl;
    
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
    std::cout << "H.264: Starting encoder with ProcessMessage calls..." << std::endl;
    hr = m_encoder->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0);
    if (FAILED(hr)) {
        std::cerr << "H.264: ProcessMessage FLUSH failed: " << std::hex << hr << std::dec << std::endl;
        Cleanup();
        return false;
    }
    
    hr = m_encoder->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
    if (FAILED(hr)) {
        std::cerr << "H.264: ProcessMessage BEGIN_STREAMING failed: " << std::hex << hr << std::dec << std::endl;
        Cleanup();
        return false;
    }
    
    hr = m_encoder->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
    if (FAILED(hr)) {
        std::cerr << "H.264: ProcessMessage START_OF_STREAM failed: " << std::hex << hr << std::dec << std::endl;
        Cleanup();
        return false;
    }
    
    std::cout << "H.264: ProcessMessage calls completed successfully" << std::endl;
    
    m_initialized = true;
    std::cout << "H.264 Encoder initialized: " << width << "x" << height << " @ " << fps << " FPS" << std::endl;
    return true;
}

HRESULT H264Encoder::CreateInputMediaType() {
    GUID formats[] = { MFVideoFormat_RGB32, MFVideoFormat_NV12, MFVideoFormat_YUY2 };
    const char* names[] = { "RGB32", "NV12", "YUY2" };

    HRESULT hr = E_FAIL;

    for (int i = 0; i < 3; ++i) {
        IMFMediaType* type = nullptr;
        hr = MFCreateMediaType(&type);
        if (FAILED(hr)) {
            return hr;
        }

        hr = type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        if (FAILED(hr)) { type->Release(); return hr; }

        hr = type->SetGUID(MF_MT_SUBTYPE, formats[i]);
        if (FAILED(hr)) { type->Release(); return hr; }

        hr = MFSetAttributeSize(type, MF_MT_FRAME_SIZE, m_width, m_height);
        if (FAILED(hr)) { type->Release(); return hr; }

        hr = MFSetAttributeRatio(type, MF_MT_FRAME_RATE, 60, 1);
        if (FAILED(hr)) { type->Release(); return hr; }

        hr = type->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
        if (FAILED(hr)) { type->Release(); return hr; }

        std::cout << "H.264: Trying " << names[i] << " input type..." << std::endl;
        hr = m_encoder->SetInputType(0, type, 0);
        if (SUCCEEDED(hr)) {
            m_inputType = type;
            m_inputFormat = formats[i];
            std::cout << "H.264: Using " << names[i] << " input format" << std::endl;
            return S_OK;
        }

        std::cout << "H.264: Failed to set " << names[i] << " input type: " << std::hex << hr << std::dec << std::endl;
        type->Release();
    }

    return hr;
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
    
    std::cout << "H.264: Setting output media type..." << std::endl;
    hr = m_encoder->SetOutputType(0, m_outputType, 0);
    if (FAILED(hr)) {
        std::cerr << "H.264: Failed to set output type: " << std::hex << hr << std::dec << std::endl;
        return hr;
    }
    std::cout << "H.264: Output media type set successfully" << std::endl;
    return S_OK;
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
        
        DWORD bufferSize = 0;
        if (m_inputFormat == MFVideoFormat_NV12) {
            bufferSize = m_width * m_height * 3 / 2;
        } else {
            // RGB32 and other formats we treat as 4 bytes per pixel
            bufferSize = m_width * m_height * 4;
        }

        hr = MFCreateMemoryBuffer(bufferSize, &m_inputBuffer);
        if (FAILED(hr)) return false;
        
        hr = m_inputSample->AddBuffer(m_inputBuffer);
        if (FAILED(hr)) return false;
    }
    
    // Copy or convert frame data depending on selected input format
    BYTE* bufferData = nullptr;
    DWORD maxLength = 0;
    hr = m_inputBuffer->Lock(&bufferData, &maxLength, nullptr);
    if (FAILED(hr)) return false;

    DWORD frameSize = 0;
    if (m_inputFormat == MFVideoFormat_NV12) {
        frameSize = m_width * m_height * 3 / 2;
        std::vector<uint8_t> temp(frameSize);
        ConvertBGRAtoNV12(frameData, temp.data(), m_width, m_height);
        memcpy(bufferData, temp.data(), frameSize);
    } else {
        // Direct copy when using RGB32
        frameSize = m_width * m_height * 4;
        memcpy(bufferData, frameData, frameSize);
    }
    
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

void H264Encoder::ConvertBGRAtoNV12(const uint8_t* bgra, uint8_t* nv12, int width, int height) {
    // NV12 format: Y plane followed by interleaved UV plane
    uint8_t* yPlane = nv12;
    uint8_t* uvPlane = nv12 + (width * height);
    
    // Convert BGRA to Y (luminance) plane
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int bgraIdx = (y * width + x) * 4;
            int yIdx = y * width + x;
            
            uint8_t b = bgra[bgraIdx + 0];
            uint8_t g = bgra[bgraIdx + 1];
            uint8_t r = bgra[bgraIdx + 2];
            
            // ITU-R BT.709 Y conversion
            yPlane[yIdx] = (uint8_t)(0.2126 * r + 0.7152 * g + 0.0722 * b + 0.5);
        }
    }
    
    // Convert BGRA to UV (chrominance) plane (NV12 = interleaved U,V at half resolution)
    for (int y = 0; y < height; y += 2) {
        for (int x = 0; x < width; x += 2) {
            // Sample 2x2 block and average
            float rSum = 0, gSum = 0, bSum = 0;
            for (int dy = 0; dy < 2 && (y + dy) < height; dy++) {
                for (int dx = 0; dx < 2 && (x + dx) < width; dx++) {
                    int bgraIdx = ((y + dy) * width + (x + dx)) * 4;
                    bSum += bgra[bgraIdx + 0];
                    gSum += bgra[bgraIdx + 1];
                    rSum += bgra[bgraIdx + 2];
                }
            }
            
            float rAvg = rSum / 4.0f;
            float gAvg = gSum / 4.0f;
            float bAvg = bSum / 4.0f;
            
            // ITU-R BT.709 UV conversion
            uint8_t u = (uint8_t)(128 + (-0.1146 * rAvg - 0.3854 * gAvg + 0.5000 * bAvg) + 0.5);
            uint8_t v = (uint8_t)(128 + (0.5000 * rAvg - 0.4542 * gAvg - 0.0458 * bAvg) + 0.5);
            
            // NV12 stores UV interleaved
            int uvIdx = (y / 2) * width + x;
            uvPlane[uvIdx + 0] = u;
            uvPlane[uvIdx + 1] = v;
        }
    }
}