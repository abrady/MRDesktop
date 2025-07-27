#include "NetworkReceiver.h"
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include "AsioNetworking.h"
#include "IVideoDecoder.h"
#include "VideoDecoder.h"

class NetworkReceiver::Impl {
 public:
  AsioConnection connection;
  std::unique_ptr<IVideoDecoder> decoder;
  CompressionType compression = COMPRESSION_H265;

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

  Impl() {
    SetupCallbacks();
  }
  
  Impl(std::unique_ptr<IVideoDecoder> customDecoder) : decoder(std::move(customDecoder)) {
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
    {
      std::lock_guard<std::mutex> lock(frameMutex);
      currentFrame = frameMsg;
      currentFrameData = frameData;
      frameReady = true;
    }

    // Call callback immediately if set
    if (onFrameReceived) {
      onFrameReceived(frameMsg, frameData);
    }
  }

  void HandleCompressedFrame(
      const CompressedFrameMessage& frameMsg,
      const std::vector<uint8_t>& compressedData) {
    if (onRawFrameReceived)
      onRawFrameReceived(MSG_COMPRESSED_FRAME);

    // Initialize decoder if needed
    if (!decoder) {
      decoder = std::make_unique<VideoDecoder>();
    }

    if (!decoder->IsInitialized()) {
      if (!decoder->Initialize(frameMsg.width, frameMsg.height, compression)) {
        if (onError)
          onError("Failed to initialize video decoder");
        return;
      }
    }

    // Decode frame
    std::vector<uint8_t> decodedData;
    if (decoder->DecodeFrame(
            compressedData.data(), compressedData.size(), decodedData)) {
      // Convert to FrameMessage format for compatibility
      FrameMessage decodedFrame;
      decodedFrame.header.type = MSG_FRAME_DATA;
      decodedFrame.header.size = sizeof(FrameMessage);
      decodedFrame.width = frameMsg.width;
      decodedFrame.height = frameMsg.height;
      decodedFrame.dataSize = decodedData.size();

      // Store frame for polling
      {
        std::lock_guard<std::mutex> lock(frameMutex);
        currentFrame = decodedFrame;
        currentFrameData = decodedData;
        frameReady = true;
      }

      // Call callback immediately if set
      if (onFrameReceived) {
        onFrameReceived(decodedFrame, decodedData);
      }
    } else {
      if (onError)
        onError("Failed to decode compressed frame");
    }
  }
};

NetworkReceiver::NetworkReceiver() : pImpl(std::make_unique<Impl>()) {}

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
  std::lock_guard<std::mutex> lock(pImpl->frameMutex);
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