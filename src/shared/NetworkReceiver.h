#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "protocol.h"

class IVideoDecoder;

class NetworkReceiver {
 private:
  class Impl;
  std::unique_ptr<Impl> pImpl;

 public:
  NetworkReceiver();
  NetworkReceiver(std::unique_ptr<IVideoDecoder> decoder);
  ~NetworkReceiver();

  // Connection management
  bool Connect(const std::string& serverIP, int port);
  void Disconnect();
  bool IsConnected() const;

  // Frame receiving (polling-based)
  bool PollFrame(); // Returns true if frame was received and processed
  void SetCompression(CompressionType compression);

  // Input message sending methods
  bool SendCompressionRequest(CompressionType compression);
  bool SendMouseMove(
      int32_t deltaX,
      int32_t deltaY,
      bool absolute = false,
      int32_t x = 0,
      int32_t y = 0);
  bool SendMouseClick(MouseClickMessage::MouseButton button, bool pressed);
  bool SendMouseScroll(int32_t deltaX, int32_t deltaY);

  // Callback setters
  void SetFrameCallback(
      std::function<void(const FrameMessage&, const std::vector<uint8_t>&)>
          callback);
  void SetErrorCallback(std::function<void(const std::string&)> callback);
  void SetDisconnectedCallback(std::function<void()> callback);
  void SetRawFrameCallback(std::function<void(MessageType)> callback);
};
