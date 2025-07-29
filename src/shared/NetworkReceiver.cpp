#include "NetworkReceiver.h"
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include "AsioNetworking.h"
#include "IVideoDecoder.h"
#include "Logging.h"

class NetworkReceiver::Impl {
 public:
  AsioConnection connection;
  std::unique_ptr<IVideoDecoder> decoder;
  CompressionType compression = COMPRESSION_H265;

  // Performance timing
  std::shared_ptr<spdlog::logger> logger;
  std::chrono::steady_clock::time_point frameReceiveStart;
  std::chrono::steady_clock::time_point lastPerfLog =
      std::chrono::steady_clock::now();
  int framesSinceLastPerfLog = 0;
  double totalNetworkTime = 0.0;
  double totalDecodeTime = 0.0;
  const std::chrono::seconds PERF_LOG_INTERVAL{10};

  // Callbacks
  std::function<void(const FrameMessage&, const std::vector<uint8_t>&)>
      onFrameReceived;
  std::function<void(const std::string&)> onError;
  std::function<void()> onDisconnected;
  std::function<void(MessageType)> onRawFrameReceived;

  // Frame polling support
  std::mutex frameMutex;
  FrameMessage currentFrame;
  std::vector<uint8_t> currentFrameData;
  bool frameReady = false;

  // Impl() { SetupCallbacks(); }

  Impl(std::unique_ptr<IVideoDecoder> customDecoder)
      : decoder(std::move(customDecoder)) {
    logger = MRDesk::GetLogger("MRDesk.NetworkReceiver");
    SetupCallbacks();
  }

  void SetupCallbacks() {
    // Set up AsioConnection callbacks
    connection.SetFrameCallback(
        [this](
            const FrameMessage& frameMsg,
            const std::vector<uint8_t>& frameData) {
          HandleUncompressedFrame(frameMsg, frameData);
        });

    connection.SetCompressedFrameCallback(
        [this](
            const CompressedFrameMessage& frameMsg,
            const std::vector<uint8_t>& frameData) {
          HandleCompressedFrame(frameMsg, frameData);
        });

    connection.SetErrorCallback([this](const std::string& error) {
      if (onError)
        onError(error);
    });

    connection.SetDisconnectCallback([this]() {
      if (onDisconnected)
        onDisconnected();
    });
  }

 private:
  void HandleUncompressedFrame(
      const FrameMessage& frameMsg, const std::vector<uint8_t>& frameData) {
    if (onRawFrameReceived)
      onRawFrameReceived(MSG_FRAME_DATA);

    // Store frame for polling
    currentFrame = frameMsg;
    currentFrameData = frameData;
    frameReady = true;

    // Call callback immediately if set
    if (onFrameReceived) {
      onFrameReceived(frameMsg, frameData);
    }
  }

  void HandleCompressedFrame(
      const CompressedFrameMessage& frameMsg,
      const std::vector<uint8_t>& compressedData) {
    auto frameStartTime = std::chrono::steady_clock::now();

    if (onRawFrameReceived)
      onRawFrameReceived(MSG_COMPRESSED_FRAME);

    if (!decoder->IsInitialized()) {
      if (!decoder->Initialize(frameMsg.width, frameMsg.height, compression)) {
        if (onError)
          onError("Failed to initialize video decoder");
        return;
      }
    }

    // Time the decode operation
    auto decodeStartTime = std::chrono::steady_clock::now();
    std::vector<uint8_t> decodedData;
    bool decodeSuccess = decoder->DecodeFrame(
        compressedData.data(), compressedData.size(), decodedData);
    auto decodeEndTime = std::chrono::steady_clock::now();

    if (decodeSuccess) {
      // Convert to FrameMessage format for compatibility
      FrameMessage decodedFrame;
      decodedFrame.header.type = MSG_FRAME_DATA;
      decodedFrame.header.size = sizeof(FrameMessage);
      decodedFrame.width = frameMsg.width;
      decodedFrame.height = frameMsg.height;
      decodedFrame.dataSize = decodedData.size();

      // Store frame for polling
      currentFrame = decodedFrame;
      currentFrameData = decodedData;
      frameReady = true;

      // Call callback immediately if set
      if (onFrameReceived) {
        onFrameReceived(decodedFrame, decodedData);
      }

      // Calculate and log performance metrics
      auto frameEndTime = std::chrono::steady_clock::now();

      double networkTimeMs =
          std::chrono::duration<double, std::milli>(
              decodeStartTime - frameStartTime)
              .count();
      double decodeTimeMs =
          std::chrono::duration<double, std::milli>(
              decodeEndTime - decodeStartTime)
              .count();
      double totalTimeMs =
          std::chrono::duration<double, std::milli>(
              frameEndTime - frameStartTime)
              .count();

      totalNetworkTime += networkTimeMs;
      totalDecodeTime += decodeTimeMs;
      framesSinceLastPerfLog++;

      auto now = std::chrono::steady_clock::now();
      auto timeSinceLastLog =
          std::chrono::duration_cast<std::chrono::seconds>(now - lastPerfLog);

      if (timeSinceLastLog >= PERF_LOG_INTERVAL) {
        double avgNetworkTime = totalNetworkTime / framesSinceLastPerfLog;
        double avgDecodeTime = totalDecodeTime / framesSinceLastPerfLog;
        double fps = static_cast<double>(framesSinceLastPerfLog) /
            timeSinceLastLog.count();

        logger->info(
            "PERFORMANCE ANALYSIS - FPS: {:.2f} | "
            "Avg Network: {:.2f}ms | Avg Decode: {:.2f}ms | "
            "Network/Decode Ratio: {:.2f} | "
            "Data: {}KB compressed -> {}KB decoded",
            fps,
            avgNetworkTime,
            avgDecodeTime,
            avgNetworkTime / avgDecodeTime,
            compressedData.size() / 1024,
            decodedData.size() / 1024);

        // Determine bottleneck
        if (avgDecodeTime > avgNetworkTime * 2) {
          logger->warn(
              "BOTTLENECK: Video decoding is slow ({:.2f}ms vs {:.2f}ms network)",
              avgDecodeTime,
              avgNetworkTime);
        } else if (avgNetworkTime > avgDecodeTime * 2) {
          logger->warn(
              "BOTTLENECK: Network download is slow ({:.2f}ms vs {:.2f}ms decode)",
              avgNetworkTime,
              avgDecodeTime);
        } else {
          logger->info("BALANCED: Network and decode times are similar");
        }

        // Reset counters
        totalNetworkTime = 0.0;
        totalDecodeTime = 0.0;
        framesSinceLastPerfLog = 0;
        lastPerfLog = now;
      }

    } else {
      if (onError)
        onError("Failed to decode compressed frame");
    }
  }
};

NetworkReceiver::NetworkReceiver(std::unique_ptr<IVideoDecoder> decoder)
    : pImpl(std::make_unique<Impl>(std::move(decoder))) {}

NetworkReceiver::~NetworkReceiver() {
  Disconnect();
}

bool NetworkReceiver::Connect(const std::string& serverIP, int port) {
  std::cout << "NetworkReceiver::Connect starting with compression="
            << pImpl->compression << std::endl;

  // Set compression before connecting
  pImpl->connection.SetCompression(pImpl->compression);

  // Connect first
  bool connected = pImpl->connection.Connect(serverIP, port);
  if (connected) {
    // Send compression request after connection is established
    if (pImpl->compression != COMPRESSION_NONE) {
      std::cout << "Sending compression request: " << pImpl->compression
                << std::endl;
      if (!pImpl->connection.SendCompressionRequest(pImpl->compression)) {
        std::cout << "Failed to send compression request" << std::endl;
        return false;
      }
    }
    std::cout << "NetworkReceiver::Connect completed successfully" << std::endl;
  } else {
    std::cout << "AsioConnection failed" << std::endl;
  }
  return connected;
}

void NetworkReceiver::Disconnect() {
  pImpl->connection.Disconnect();
}

bool NetworkReceiver::PollFrame() {
  // Poll the connection for any incoming data/events
  pImpl->connection.Poll();

  // Check if a frame is ready
  if (pImpl->frameReady) {
    pImpl->frameReady = false;
    return true;
  }
  return false;
}

bool NetworkReceiver::IsConnected() const {
  return pImpl->connection.IsConnected();
}

void NetworkReceiver::SetCompression(CompressionType compression) {
  pImpl->compression = compression;
}

bool NetworkReceiver::SendCompressionRequest(CompressionType compression) {
  return pImpl->connection.SendCompressionRequest(compression);
}

bool NetworkReceiver::SendMouseMove(
    int32_t deltaX, int32_t deltaY, bool absolute, int32_t x, int32_t y) {
  return pImpl->connection.SendMouseMove(deltaX, deltaY, absolute, x, y);
}

bool NetworkReceiver::SendMouseClick(
    MouseClickMessage::MouseButton button, bool pressed) {
  return pImpl->connection.SendMouseClick(button, pressed);
}

bool NetworkReceiver::SendMouseScroll(int32_t deltaX, int32_t deltaY) {
  return pImpl->connection.SendMouseScroll(deltaX, deltaY);
}

void NetworkReceiver::SetFrameCallback(
    std::function<void(const FrameMessage&, const std::vector<uint8_t>&)>
        callback) {
  pImpl->onFrameReceived = callback;
}

void NetworkReceiver::SetErrorCallback(
    std::function<void(const std::string&)> callback) {
  pImpl->onError = callback;
}

void NetworkReceiver::SetDisconnectedCallback(std::function<void()> callback) {
  pImpl->onDisconnected = callback;
}

void NetworkReceiver::SetRawFrameCallback(
    std::function<void(MessageType)> callback) {
  pImpl->onRawFrameReceived = callback;
}