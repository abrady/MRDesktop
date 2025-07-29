#pragma once

#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif
#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include <vector>
#include <asio.hpp>
#include "protocol.h"

using asio::ip::tcp;

/**
 * Simple Asio-based networking classes to replace platform-specific socket code
 * Much cleaner than the previous raw socket implementations
 */

class AsioConnection {
 public:
  using FrameCallback =
      std::function<void(const FrameMessage&, const std::vector<uint8_t>&)>;
  using CompressedFrameCallback = std::function<void(
      const CompressedFrameMessage&, const std::vector<uint8_t>&)>;
  using MouseMoveCallback = std::function<void(const MouseMoveMessage&)>;
  using MouseClickCallback = std::function<void(const MouseClickMessage&)>;
  using MouseScrollCallback = std::function<void(const MouseScrollMessage&)>;
  using CompressionRequestCallback =
      std::function<void(const CompressionRequestMessage&)>;
  using PixelFormatRequestCallback =
      std::function<void(const PixelFormatRequestMessage&)>;
  using ErrorCallback = std::function<void(const std::string&)>;
  using DisconnectCallback = std::function<void()>;

 private:
  asio::io_context m_ioContext;
  std::unique_ptr<tcp::socket> m_socket;
  std::atomic<bool> m_running{false};

  // Callbacks
  FrameCallback m_onFrame;
  CompressedFrameCallback m_onCompressedFrame;
  MouseMoveCallback m_onMouseMove;
  MouseClickCallback m_onMouseClick;
  MouseScrollCallback m_onMouseScroll;
  CompressionRequestCallback m_onCompressionRequest;
  PixelFormatRequestCallback m_onPixelFormatRequest;
  ErrorCallback m_onError;
  DisconnectCallback m_onDisconnect;

  // Receive state - accumulating buffer approach
  std::vector<uint8_t> m_accumBuffer;
  enum ReadingState {
    READING_HEADER,
    READING_DATAHEADER,
    READING_DATA
  } m_readingState = READING_HEADER;
  size_t m_bytesNeeded = sizeof(MessageHeader); // Start by needing a header

  // Connection settings
  CompressionType m_compression = COMPRESSION_NONE;

 public:
  AsioConnection();
  ~AsioConnection();

  // Client operations
  void SetCompression(CompressionType compression) {
    m_compression = compression;
  }
  bool Connect(const std::string& host, int port);

  // Server operations
  bool Listen(int port);

  // Send operations
  bool SendFrame(
      const FrameMessage& frameMsg, const std::vector<uint8_t>& frameData);
  bool SendCompressedFrame(
      const CompressedFrameMessage& frameMsg,
      const std::vector<uint8_t>& frameData);
  bool SendMouseMove(
      int32_t deltaX,
      int32_t deltaY,
      bool absolute = false,
      int32_t x = 0,
      int32_t y = 0);
  bool SendMouseClick(MouseClickMessage::MouseButton button, bool pressed);
  bool SendMouseScroll(int32_t deltaX, int32_t deltaY);
  bool SendCompressionRequest(CompressionType compression);
  bool SendPixelFormatRequest(PixelFormat format);

  // Raw data sending (for testing/debugging)
  bool SendData(const void* data, size_t size);

  // Connection management
  void Disconnect();
  void Poll(); // Process pending async operations
  bool IsConnected() const {
    return m_running && m_socket && m_socket->is_open();
  }

  // Callback setters
  void SetFrameCallback(FrameCallback callback) { m_onFrame = callback; }
  void SetCompressedFrameCallback(CompressedFrameCallback callback) {
    m_onCompressedFrame = callback;
  }
  void SetMouseMoveCallback(MouseMoveCallback callback) {
    m_onMouseMove = callback;
  }
  void SetMouseClickCallback(MouseClickCallback callback) {
    m_onMouseClick = callback;
  }
  void SetMouseScrollCallback(MouseScrollCallback callback) {
    m_onMouseScroll = callback;
  }
  void SetCompressionRequestCallback(CompressionRequestCallback callback) {
    m_onCompressionRequest = callback;
  }
  void SetPixelFormatRequestCallback(PixelFormatRequestCallback callback) {
    m_onPixelFormatRequest = callback;
  }
  void SetErrorCallback(ErrorCallback callback) { m_onError = callback; }
  void SetDisconnectCallback(DisconnectCallback callback) {
    m_onDisconnect = callback;
  }

 private:
  void ReadAvailableData();
  void ProcessAccumulatedData();
  void ProcessCompleteMessage();
  void NotifyError(const std::string& message);
  void RunIoContext();

  template <typename T>
  bool SendMessage(const T& message);

  friend class AsioServer;
};

/**
 * Simple server that accepts connections and creates AsioConnection for each
 * client
 */
class AsioServer {
 public:
  using ClientConnectedCallback =
      std::function<void(std::shared_ptr<AsioConnection>)>;

 private:
  asio::io_context m_ioContext;
  std::unique_ptr<tcp::acceptor> m_acceptor;
  std::atomic<bool> m_running{false};
  ClientConnectedCallback m_onClientConnected;

 public:
  AsioServer();
  ~AsioServer();

  bool Start(int port);
  void Stop();
  void Poll(); // Process pending async operations

  void SetClientConnectedCallback(ClientConnectedCallback callback) {
    m_onClientConnected = callback;
  }

 private:
  void StartAccept();
  void HandleAccept(
      std::shared_ptr<AsioConnection> newConnection,
      const asio::error_code& error);
};