#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <windows.h>
#else
#include <cstdint>
using BYTE = uint8_t;
using UINT = uint32_t;
using UINT32 = uint32_t;
using INT32 = int32_t;
#endif

#include "../shared/AsioNetworking.h"
#include "../shared/VideoEncoder.h"
#include "../shared/protocol.h"

#ifdef _WIN32
class DesktopDuplicator {
 private:
  ID3D11Device* m_Device = nullptr;
  ID3D11DeviceContext* m_Context = nullptr;
  IDXGIOutputDuplication* m_DeskDupl = nullptr;
  IDXGIOutput1* m_Output1 = nullptr;
  DXGI_OUTPUT_DESC m_OutputDesc = {};

 public:
  bool Initialize() {
    HRESULT hr = S_OK;

    // Create D3D11 device
    D3D_FEATURE_LEVEL featureLevel;
    hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &m_Device,
        &featureLevel,
        &m_Context);
    if (FAILED(hr)) {
      std::cerr << "Failed to create D3D11 device: " << std::hex << hr
                << std::endl;
      return false;
    }

    // Get DXGI device
    IDXGIDevice* dxgiDevice = nullptr;
    hr = m_Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    if (FAILED(hr)) {
      std::cerr << "Failed to get DXGI device: " << std::hex << hr << std::endl;
      return false;
    }

    // Get DXGI adapter
    IDXGIAdapter* dxgiAdapter = nullptr;
    hr = dxgiDevice->GetAdapter(&dxgiAdapter);
    dxgiDevice->Release();
    if (FAILED(hr)) {
      std::cerr << "Failed to get DXGI adapter: " << std::hex << hr
                << std::endl;
      return false;
    }

    // Get primary output
    IDXGIOutput* dxgiOutput = nullptr;
    hr = dxgiAdapter->EnumOutputs(0, &dxgiOutput);
    dxgiAdapter->Release();
    if (FAILED(hr)) {
      std::cerr << "Failed to get primary output: " << std::hex << hr
                << std::endl;
      return false;
    }

    // Get output description
    dxgiOutput->GetDesc(&m_OutputDesc);
    std::cout
        << "Primary display: "
        << m_OutputDesc.DesktopCoordinates.right -
            m_OutputDesc.DesktopCoordinates.left
        << "x"
        << m_OutputDesc.DesktopCoordinates.bottom -
            m_OutputDesc.DesktopCoordinates.top
        << std::endl;

    // Get IDXGIOutput1
    hr = dxgiOutput->QueryInterface(__uuidof(IDXGIOutput1), (void**)&m_Output1);
    dxgiOutput->Release();
    if (FAILED(hr)) {
      std::cerr << "Failed to get IDXGIOutput1: " << std::hex << hr
                << std::endl;
      return false;
    }

    // Create desktop duplication
    hr = m_Output1->DuplicateOutput(m_Device, &m_DeskDupl);
    if (FAILED(hr)) {
      std::cerr << "Failed to create desktop duplication: " << std::hex << hr
                << std::endl;
      if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE) {
        std::cerr
            << "Desktop duplication is not available (may be in use by another process)"
            << std::endl;
      }
      return false;
    }

    std::cout << "Desktop Duplication initialized successfully!" << std::endl;
    return true;
  }

  bool CaptureFrame(
      std::vector<BYTE>& pixelData,
      UINT32& width,
      UINT32& height,
      UINT32& dataSize) {
    if (!m_DeskDupl)
      return false;

    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    IDXGIResource* desktopResource = nullptr;

    HRESULT hr =
        m_DeskDupl->AcquireNextFrame(100, &frameInfo, &desktopResource);
    if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
      return false; // No new frame
    }
    if (FAILED(hr)) {
      std::cerr << "Failed to acquire next frame: " << std::hex << hr
                << std::endl;
      return false;
    }

    // Get the desktop texture
    ID3D11Texture2D* desktopTexture = nullptr;
    hr = desktopResource->QueryInterface(
        __uuidof(ID3D11Texture2D), (void**)&desktopTexture);
    desktopResource->Release();
    if (FAILED(hr)) {
      m_DeskDupl->ReleaseFrame();
      return false;
    }

    // Create a staging texture to read the data
    D3D11_TEXTURE2D_DESC textureDesc;
    desktopTexture->GetDesc(&textureDesc);

    textureDesc.Usage = D3D11_USAGE_STAGING;
    textureDesc.BindFlags = 0;
    textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    textureDesc.MiscFlags = 0;

    ID3D11Texture2D* stagingTexture = nullptr;
    hr = m_Device->CreateTexture2D(&textureDesc, nullptr, &stagingTexture);
    if (FAILED(hr)) {
      desktopTexture->Release();
      m_DeskDupl->ReleaseFrame();
      return false;
    }

    // Copy desktop texture to staging texture
    m_Context->CopyResource(stagingTexture, desktopTexture);
    desktopTexture->Release();

    // Map the staging texture to read pixel data
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    hr = m_Context->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);
    if (SUCCEEDED(hr)) {
      // Set frame dimensions and data size
      width = textureDesc.Width;
      height = textureDesc.Height;
      dataSize = mappedResource.RowPitch * textureDesc.Height;

      // Debug: Log frame capture details
      std::cout << "Capturing frame - Width: " << width << ", Height: "
                << height << ", RowPitch: " << mappedResource.RowPitch
                << ", DataSize: " << dataSize << std::endl;

      // Resize buffer for pixel data only
      pixelData.clear();
      pixelData.resize(dataSize);

      // Copy pixel data directly
      BYTE* srcData = (BYTE*)mappedResource.pData;
      BYTE* dstData = pixelData.data();

      for (UINT row = 0; row < textureDesc.Height; ++row) {
        memcpy(
            dstData + row * mappedResource.RowPitch,
            srcData + row * mappedResource.RowPitch,
            mappedResource.RowPitch);
      }

      m_Context->Unmap(stagingTexture, 0);
    }

    stagingTexture->Release();
    m_DeskDupl->ReleaseFrame();

    return SUCCEEDED(hr);
  }

  void Cleanup() {
    if (m_DeskDupl) {
      m_DeskDupl->Release();
      m_DeskDupl = nullptr;
    }
    if (m_Output1) {
      m_Output1->Release();
      m_Output1 = nullptr;
    }
    if (m_Context) {
      m_Context->Release();
      m_Context = nullptr;
    }
    if (m_Device) {
      m_Device->Release();
      m_Device = nullptr;
    }
  }

  ~DesktopDuplicator() { Cleanup(); }
};
#else
class DesktopDuplicator {
 public:
  bool Initialize() { return false; }
  bool CaptureFrame(std::vector<uint8_t>&, uint32_t&, uint32_t&, uint32_t&) {
    return false;
  }
  void Cleanup() {}
};
#endif

#ifdef _WIN32
class InputInjector {
 public:
  static bool InjectMouseMove(
      INT32 deltaX,
      INT32 deltaY,
      UINT32 absolute = 0,
      INT32 x = 0,
      INT32 y = 0) {
    INPUT input = {};
    input.type = INPUT_MOUSE;

    if (absolute) {
      // Convert to normalized coordinates (0-65535)
      int screenWidth = GetSystemMetrics(SM_CXSCREEN);
      int screenHeight = GetSystemMetrics(SM_CYSCREEN);

      input.mi.dx = (x * 65535) / screenWidth;
      input.mi.dy = (y * 65535) / screenHeight;
      input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    } else {
      input.mi.dx = deltaX;
      input.mi.dy = deltaY;
      input.mi.dwFlags = MOUSEEVENTF_MOVE;
    }

    UINT result = SendInput(1, &input, sizeof(INPUT));
    return result == 1;
  }

  static bool InjectMouseClick(
      MouseClickMessage::MouseButton button, UINT32 pressed) {
    INPUT input = {};
    input.type = INPUT_MOUSE;

    DWORD flags = 0;
    switch (button) {
      case MouseClickMessage::LEFT_BUTTON:
        flags = pressed ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
        break;
      case MouseClickMessage::RIGHT_BUTTON:
        flags = pressed ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
        break;
      case MouseClickMessage::MIDDLE_BUTTON:
        flags = pressed ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
        break;
      default:
        return false;
    }

    input.mi.dwFlags = flags;
    UINT result = SendInput(1, &input, sizeof(INPUT));
    return result == 1;
  }

  static bool InjectMouseScroll(INT32 deltaX, INT32 deltaY) {
    if (deltaY != 0) {
      INPUT input = {};
      input.type = INPUT_MOUSE;
      input.mi.dwFlags = MOUSEEVENTF_WHEEL;
      input.mi.mouseData = deltaY * WHEEL_DELTA;

      UINT result = SendInput(1, &input, sizeof(INPUT));
      if (result != 1)
        return false;
    }

    if (deltaX != 0) {
      INPUT input = {};
      input.type = INPUT_MOUSE;
      input.mi.dwFlags = MOUSEEVENTF_HWHEEL;
      input.mi.mouseData = deltaX * WHEEL_DELTA;

      UINT result = SendInput(1, &input, sizeof(INPUT));
      if (result != 1)
        return false;
    }

    return true;
  }
};
#else
class InputInjector {
 public:
  static bool InjectMouseMove(
      int32_t, int32_t, uint32_t = 0, int32_t = 0, int32_t = 0) {
    return true;
  }
  static bool InjectMouseClick(MouseClickMessage::MouseButton, uint32_t) {
    return true;
  }
  static bool InjectMouseScroll(int32_t, int32_t) { return true; }
};
#endif

// Helper function to format bytes in human-readable format
std::string formatBytes(uint64_t bytes) {
  const char* units[] = {"B", "KB", "MB", "GB"};
  int unitIndex = 0;
  double size = static_cast<double>(bytes);

  while (size >= 1024.0 && unitIndex < 3) {
    size /= 1024.0;
    unitIndex++;
  }

  char buffer[32];
  if (unitIndex == 0) {
    snprintf(buffer, sizeof(buffer), "%.0f %s", size, units[unitIndex]);
  } else {
    snprintf(buffer, sizeof(buffer), "%.2f %s", size, units[unitIndex]);
  }
  return std::string(buffer);
}

/**
 * MRDesktop Server using Asio networking
 * Much cleaner than the previous raw socket implementation
 */
class MRDesktopServer {
 private:
  AsioServer m_server;
  DesktopDuplicator m_duplicator;
  std::shared_ptr<AsioConnection> m_currentClient;

  // Streaming state
  std::atomic<bool> m_streaming{false};
  std::thread m_streamingThread;
  std::unique_ptr<VideoEncoder> m_encoder;
  CompressionType m_clientCompression = COMPRESSION_NONE;

  // Frame statistics
  int m_frameCount = 0;
  std::chrono::high_resolution_clock::time_point m_startTime;

  bool m_testMode = false;

 public:
  MRDesktopServer(bool testMode = false) : m_testMode(testMode) {
    m_startTime = std::chrono::high_resolution_clock::now();
  }

  ~MRDesktopServer() { Stop(); }

  bool Start(int port = 8080) {
    std::cout << "MRDesktop Server - Desktop Duplication Service" << std::endl;
    std::cout << "=============================================" << std::endl;

    if (m_testMode) {
      std::cout << "RUNNING IN TEST MODE" << std::endl;
    }

    // Initialize platform networking (done automatically by Asio)
#ifdef _WIN32
    // Initialize COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
      std::cerr << "Failed to initialize COM: " << std::hex << hr << std::endl;
      return false;
    }
    std::cout << "COM initialized successfully" << std::endl;
#endif

    // Initialize desktop duplicator (skip in test mode)
    if (!m_testMode && !m_duplicator.Initialize()) {
      std::cerr << "Failed to initialize desktop duplicator" << std::endl;
#ifdef _WIN32
      CoUninitialize();
#endif
      return false;
    }

    // Setup server callbacks
    m_server.SetClientConnectedCallback(
        [this](std::shared_ptr<AsioConnection> client) {
          OnClientConnected(client);
        });

    // Start server
    if (!m_server.Start(port)) {
      std::cerr << "Failed to start server on port " << port << std::endl;
#ifdef _WIN32
      CoUninitialize();
#endif
      return false;
    }

    std::cout << "Server listening on port " << port << "..." << std::endl;
    std::cout << "Waiting for client connection..." << std::endl;

    return true;
  }

  void Stop() {
    m_streaming = false;

    if (m_streamingThread.joinable()) {
      m_streamingThread.join();
    }

    m_server.Stop();

#ifdef _WIN32
    CoUninitialize();
#endif
  }

  void WaitForExit() {
    // Simple wait loop - in a real app you'd handle signals properly
    std::cout << "Press Ctrl+C to exit..." << std::endl;
    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

 private:
  void OnClientConnected(std::shared_ptr<AsioConnection> client) {
    std::cout << "Client connected! Starting desktop streaming..." << std::endl;

    m_currentClient = client;

    // Setup client callbacks
    client->SetInputCallback(
        [this](const MessageHeader& header, const std::vector<uint8_t>& data) {
          HandleInputMessage(header, data);
        });

    client->SetErrorCallback([this](const std::string& error) {
      std::cerr << "Client error: " << error << std::endl;
    });

    client->SetDisconnectCallback([this]() {
      std::cout << "Client disconnected" << std::endl;
      m_streaming = false;
      m_currentClient.reset();
    });

    // Start streaming
    m_streaming = true;
    m_streamingThread = std::thread([this]() { StreamingLoop(); });
  }

  void HandleInputMessage(
      const MessageHeader& header, const std::vector<uint8_t>& data) {
    std::cout << "HandleInputMessage: type=" << header.type << " size="
              << header.size << " data.size()=" << data.size() << std::endl;
    switch (header.type) {
      case MSG_COMPRESSION_REQUEST: {
        std::cout << "Processing compression request" << std::endl;
        if (data.size() >= sizeof(CompressionType)) {
          // Extract compression type from the data payload
          m_clientCompression =
              *reinterpret_cast<const CompressionType*>(data.data());
          std::cout << "Client requested compression type: "
                    << m_clientCompression << std::endl;

          // Send acknowledgment back to client (optional but good practice)
          std::cout << "Server now configured for compression type: "
                    << m_clientCompression << std::endl;
        } else {
          std::cout << "Invalid compression request size: " << data.size()
                    << " expected: " << sizeof(CompressionType) << std::endl;
        }
        break;
      }
      case MSG_MOUSE_MOVE: {
        if (data.size() >= sizeof(MouseMoveMessage) - sizeof(MessageHeader)) {
          // Create a complete MouseMoveMessage by combining header and data
          MouseMoveMessage msg;
          msg.header = header;
          memcpy(
              reinterpret_cast<char*>(&msg) + sizeof(MessageHeader),
              data.data(),
              data.size());
          InputInjector::InjectMouseMove(
              msg.deltaX, msg.deltaY, msg.absolute, msg.x, msg.y);
          std::cout << "Mouse move: dx=" << msg.deltaX << " dy=" << msg.deltaY
                    << std::endl;
        }
        break;
      }
      case MSG_MOUSE_CLICK: {
        if (data.size() >= sizeof(MouseClickMessage) - sizeof(MessageHeader)) {
          // Create a complete MouseClickMessage by combining header and data
          MouseClickMessage msg;
          msg.header = header;
          memcpy(
              reinterpret_cast<char*>(&msg) + sizeof(MessageHeader),
              data.data(),
              data.size());
          InputInjector::InjectMouseClick(msg.button, msg.pressed);
          std::cout << "Mouse " << (msg.pressed ? "press" : "release")
                    << " button " << msg.button << std::endl;
        }
        break;
      }
      case MSG_MOUSE_SCROLL: {
        if (data.size() >= sizeof(MouseScrollMessage) - sizeof(MessageHeader)) {
          // Create a complete MouseScrollMessage by combining header and data
          MouseScrollMessage msg;
          msg.header = header;
          memcpy(
              reinterpret_cast<char*>(&msg) + sizeof(MessageHeader),
              data.data(),
              data.size());
          InputInjector::InjectMouseScroll(msg.deltaX, msg.deltaY);
          std::cout << "Mouse scroll: dx=" << msg.deltaX << " dy=" << msg.deltaY
                    << std::endl;
        }
        break;
      }
    }
  }

  void StreamingLoop() {
    std::vector<BYTE> pixelData;
    UINT32 frameWidth, frameHeight, frameDataSize;

    std::cout
        << "Starting streaming loop - compression will be checked per frame"
        << std::endl;

    while (m_streaming && m_currentClient && m_currentClient->IsConnected()) {
      // Capture or generate test frame
      bool frameReady = false;
      if (m_testMode) {
        // Generate test frame data
        frameWidth = 640;
        frameHeight = 480;
        frameDataSize = frameWidth * frameHeight * 4; // BGRA format

        pixelData.clear();
        pixelData.resize(frameDataSize);

        // Create test pattern: red-green gradient with frame counter
        for (uint32_t y = 0; y < frameHeight; y++) {
          for (uint32_t x = 0; x < frameWidth; x++) {
            uint32_t index = (y * frameWidth + x) * 4;
            uint8_t red = static_cast<uint8_t>((x * 255) / frameWidth);
            uint8_t green = static_cast<uint8_t>((y * 255) / frameHeight);
            uint8_t blue = static_cast<uint8_t>(m_frameCount % 256);

            pixelData[index + 0] = blue; // B
            pixelData[index + 1] = green; // G
            pixelData[index + 2] = red; // R
            pixelData[index + 3] = 255; // A
          }
        }
        frameReady = true;
        std::cout << "Generated test frame " << m_frameCount + 1 << " (640x480)"
                  << std::endl;
      } else {
        frameReady = m_duplicator.CaptureFrame(
            pixelData, frameWidth, frameHeight, frameDataSize);
      }

      if (frameReady) {
        // Validate frame dimensions are reasonable
        if (frameWidth == 0 || frameHeight == 0 || frameWidth > 10000 ||
            frameHeight > 10000 || frameDataSize > 100000000) {
          std::cerr << "Invalid frame data - Width: " << frameWidth
                    << ", Height: " << frameHeight
                    << ", DataSize: " << frameDataSize << std::endl;
          continue;
        }

        // Check if compression is requested (dynamically check each frame)
        bool useCompression = (m_clientCompression != COMPRESSION_NONE);

        if (useCompression) {
          // Initialize encoder with first frame dimensions if needed
          if (!m_encoder) {
            m_encoder = std::make_unique<VideoEncoder>();
            std::cout
                << "Compression enabled, encoder will be initialized with first frame"
                << std::endl;
          }

          if (!m_encoder->IsInitialized()) {
            if (!m_encoder->Initialize(
                    frameWidth, frameHeight, m_clientCompression)) {
              std::cerr << "Failed to initialize video encoder" << std::endl;
              useCompression =
                  false; // Fall back to uncompressed for this frame
            } else {
              std::cout << "Video encoder initialized successfully"
                        << std::endl;
            }
          }

          if (useCompression && m_encoder->IsInitialized()) {
            // Encode frame
            std::vector<uint8_t> compressedData;
            bool isKeyframe = false;

            if (m_encoder->EncodeFrame(
                    pixelData.data(), compressedData, isKeyframe)) {
              // Send compressed frame
              CompressedFrameMessage compFrameMsg;
              compFrameMsg.header.type = MSG_COMPRESSED_FRAME;
              compFrameMsg.header.size = sizeof(CompressedFrameMessage);
              compFrameMsg.width = frameWidth;
              compFrameMsg.height = frameHeight;
              compFrameMsg.compressedSize = compressedData.size();
              compFrameMsg.isKeyframe = isKeyframe ? 1 : 0;

              std::cout
                  << "SERVER SEND: Frame " << m_frameCount + 1
                  << " - Compressed: " << compressedData.size() << " bytes ("
                  << (isKeyframe ? "KEY" : "DELTA") << ")" << std::endl;

              if (!m_currentClient->SendCompressedFrame(
                      compFrameMsg, compressedData)) {
                std::cerr << "Failed to send compressed frame" << std::endl;
                break;
              }
            } else {
              // Skip this frame if encoding failed
              continue;
            }
          } else {
            useCompression = false; // Fall back to uncompressed for this frame
          }
        }

        if (!useCompression) {
          // Send uncompressed frame
          FrameMessage frameMsg;
          frameMsg.header.type = MSG_FRAME_DATA;
          frameMsg.header.size = sizeof(FrameMessage);
          frameMsg.width = frameWidth;
          frameMsg.height = frameHeight;
          frameMsg.dataSize = frameDataSize;

          std::cout << "SERVER SEND: Frame " << m_frameCount + 1
                    << " - Uncompressed: " << frameDataSize << " bytes"
                    << std::endl;

          if (!m_currentClient->SendFrame(frameMsg, pixelData)) {
            std::cerr << "Failed to send frame" << std::endl;
            break;
          }
        }

        std::cout << "SERVER SEND: Frame " << m_frameCount + 1 << " - COMPLETE"
                  << std::endl;

        m_frameCount++;
        if (m_frameCount % 30 == 0) {
          auto currentTime = std::chrono::high_resolution_clock::now();
          auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
              currentTime - m_startTime);
          double fps = (m_frameCount * 1000.0) / duration.count();
          std::cout << "Sent " << m_frameCount << " frames, FPS: " << fps
                    << ", Frame size: " << formatBytes(frameDataSize)
                    << std::endl;
        }

        // In test mode, exit after sending 3 frames
        if (m_testMode && m_frameCount >= 3) {
          std::cout << "TEST MODE: Sent 3 frames, exiting successfully"
                    << std::endl;
          break;
        }
      }

      std::this_thread::sleep_for(
          std::chrono::milliseconds(16)); // ~60fps target
    }
  }
};

int main(int argc, char* argv[]) {
  bool testMode = false;

  // Parse command line arguments
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--test") == 0) {
      testMode = true;
    }
  }

  MRDesktopServer server(testMode);

  if (!server.Start(8080)) {
    std::cerr << "Failed to start server" << std::endl;
    return 1;
  }

  if (testMode) {
    // In test mode, wait for completion
    std::this_thread::sleep_for(std::chrono::seconds(5));
  } else {
    server.WaitForExit();
  }

  return 0;
}