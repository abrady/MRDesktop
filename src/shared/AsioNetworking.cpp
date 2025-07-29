#include "AsioNetworking.h"
#include <iostream>
#define LOG_TAG "MRDesk.AsioNetworking"
#include "Logging.h"

// AsioConnection Implementation
AsioConnection::AsioConnection() {
  m_socket = std::make_unique<tcp::socket>(m_ioContext);
}

AsioConnection::~AsioConnection() {
  Disconnect();
}

bool AsioConnection::Connect(const std::string& host, int port) {
  // Single-use connection: prevent reuse after disconnect
  if (m_running) {
    NotifyError(
        "Connection object cannot be reused. Create a new AsioConnection.");
    return false;
  }

  try {
    tcp::resolver resolver(m_ioContext);
    auto endpoints = resolver.resolve(host, std::to_string(port));

    asio::connect(*m_socket, endpoints);

    m_running = true;
    return true;
  } catch (const std::exception& e) {
    NotifyError("Connect failed: " + std::string(e.what()));
    return false;
  }
}

bool AsioConnection::Listen(int port) {
  // Single-use connection: prevent reuse after disconnect
  if (m_socket && !m_socket->is_open()) {
    NotifyError(
        "Connection object cannot be reused. Create a new AsioConnection.");
    return false;
  }

  // This is for single-connection listening (simplified)
  try {
    tcp::acceptor acceptor(
        m_ioContext,
        tcp::endpoint(tcp::v4(), static_cast<asio::ip::port_type>(port)));
    acceptor.accept(*m_socket);

    m_running = true;
    return true;
  } catch (const std::exception& e) {
    NotifyError("Listen failed: " + std::string(e.what()));
    return false;
  }
}

void AsioConnection::Disconnect() {
  // Early return if already disconnected
  if (!m_running) {
    return;
  }

  // Mark as disconnected first to prevent re-entry
  m_running = false;

  // Close socket safely
  if (m_socket && m_socket->is_open()) {
    asio::error_code ec;
    m_socket->close(ec);
    // Note: We ignore errors here since we're already disconnecting
  }

  m_ioContext.stop();

  // Call disconnect callback exactly once
  if (m_onDisconnect) {
    auto callback = std::move(m_onDisconnect); // Move out to prevent re-entry
    m_onDisconnect = nullptr; // Clear to prevent double-call
    callback(); // Call the moved callback
  }
}

void AsioConnection::Poll() {
  if (m_running) {
    // First process any completed operations
    m_ioContext.poll();

    // Check if socket is still connected
    if (m_socket && !m_socket->is_open()) {
      // Socket was closed, trigger disconnect
      Disconnect();
      return;
    }

    // Then try to read any available data
    ReadAvailableData();

    // Process any complete messages we now have
    ProcessAccumulatedData();
  }
}

bool AsioConnection::SendFrame(
    const FrameMessage& frameMsg, const std::vector<uint8_t>& frameData) {
  if (!SendMessage(frameMsg)) {
    return false;
  }
  return SendData(frameData.data(), frameData.size());
}

bool AsioConnection::SendCompressedFrame(
    const CompressedFrameMessage& frameMsg,
    const std::vector<uint8_t>& frameData) {
  if (!SendMessage(frameMsg)) {
    return false;
  }
  return SendData(frameData.data(), frameData.size());
}

bool AsioConnection::SendMouseMove(
    int32_t deltaX, int32_t deltaY, bool absolute, int32_t x, int32_t y) {
  MouseMoveMessage msg;
  msg.header.type = MSG_MOUSE_MOVE;
  msg.header.size = sizeof(MouseMoveMessage);
  msg.deltaX = deltaX;
  msg.deltaY = deltaY;
  msg.absolute = absolute ? 1 : 0;
  msg.x = x;
  msg.y = y;

  return SendMessage(msg);
}

bool AsioConnection::SendMouseClick(
    MouseClickMessage::MouseButton button, bool pressed) {
  MouseClickMessage msg;
  msg.header.type = MSG_MOUSE_CLICK;
  msg.header.size = sizeof(MouseClickMessage);
  msg.button = button;
  msg.pressed = pressed ? 1 : 0;

  return SendMessage(msg);
}

bool AsioConnection::SendMouseScroll(int32_t deltaX, int32_t deltaY) {
  MouseScrollMessage msg;
  msg.header.type = MSG_MOUSE_SCROLL;
  msg.header.size = sizeof(MouseScrollMessage);
  msg.deltaX = deltaX;
  msg.deltaY = deltaY;

  return SendMessage(msg);
}

bool AsioConnection::SendCompressionRequest(CompressionType compression) {
  CompressionRequestMessage msg;
  msg.header.type = MSG_COMPRESSION_REQUEST;
  msg.header.size = sizeof(CompressionRequestMessage);
  msg.compression = compression;

  LOGD(
      "SendCompressionRequest: sending type={} size={} compression={}",
      static_cast<uint32_t>(msg.header.type),
      msg.header.size,
      static_cast<uint32_t>(compression));

  return SendMessage(msg);
}

template <typename T>
bool AsioConnection::SendMessage(const T& message) {
  return SendData(&message, sizeof(T));
}

bool AsioConnection::SendData(const void* data, size_t size) {
  if (!IsConnected()) {
    return false;
  }

  try {
    asio::write(*m_socket, asio::buffer(data, size));
    return true;
  } catch (const std::exception& e) {
    NotifyError("Send failed: " + std::string(e.what()));
    return false;
  }
}

void AsioConnection::ReadAvailableData() {
  if (!IsConnected()) {
    return;
  }

  try {
    // Check how many bytes are available
    size_t available = m_socket->available();
    if (available == 0) {
      return;
    }

    // Read up to available bytes into a temp buffer
    std::vector<uint8_t> tempBuffer(available);
    size_t bytesRead = m_socket->read_some(asio::buffer(tempBuffer));

    // Append to our accumulating buffer
    m_accumBuffer.insert(
        m_accumBuffer.end(),
        tempBuffer.begin(),
        tempBuffer.begin() + bytesRead);

  } catch (const std::exception& e) {
    if (e.what() !=
        std::string("read_some: Resource temporarily unavailable")) {
      NotifyError("Read error: " + std::string(e.what()));
    }
  }
}

void AsioConnection::ProcessAccumulatedData() {
  while (m_accumBuffer.size() >= m_bytesNeeded) {
    MessageHeader header;
    switch (m_readingState) {
      case READING_HEADER:
        memcpy(&header, m_accumBuffer.data(), sizeof(MessageHeader));

        // The header.size field contains the total message size
        m_bytesNeeded = header.size;
        m_readingState = READING_DATAHEADER;
        break;

      case READING_DATAHEADER:
        memcpy(&header, m_accumBuffer.data(), sizeof(MessageHeader));

        switch (header.type) {
          case MSG_FRAME_DATA:
            FrameMessage frameMsg;
            memcpy(&frameMsg, m_accumBuffer.data(), sizeof(FrameMessage));
            m_bytesNeeded += frameMsg.dataSize;
            break;
          case MSG_COMPRESSED_FRAME:
            CompressedFrameMessage compMsg;
            memcpy(
                &compMsg, m_accumBuffer.data(), sizeof(CompressedFrameMessage));
            m_bytesNeeded += compMsg.compressedSize;
            break;
          case MSG_MOUSE_MOVE:
          case MSG_MOUSE_CLICK:
          case MSG_MOUSE_SCROLL:
            // For mouse messages, we just need the header size
            break;
          case MSG_COMPRESSION_REQUEST:
          case MSG_PIXEL_FORMAT_REQUEST:
            // Compression request only needs the header size
            break;
        }
        m_readingState = READING_DATA;
        break;
      case READING_DATA:
        // We have enough data to process a complete message
        ProcessCompleteMessage();

        // Remove processed bytes from buffer
        m_accumBuffer.erase(
            m_accumBuffer.begin(), m_accumBuffer.begin() + m_bytesNeeded);

        // Reset to read next header
        m_bytesNeeded = sizeof(MessageHeader);
        m_readingState = READING_HEADER;
        break;
    }
  }
}

void AsioConnection::ProcessCompleteMessage() {
  if (m_accumBuffer.size() < m_bytesNeeded) {
    // This should never happen - indicates a bug in our state machine
    NotifyError(
        "INTERNAL ERROR: ProcessCompleteMessage called without enough data");
    Disconnect();
    return;
  }

  MessageHeader header;
  memcpy(&header, m_accumBuffer.data(), sizeof(MessageHeader));

  LOGD("ProcessCompleteMessage: type={}", static_cast<uint32_t>(header.type));

  switch (header.type) {
    case MSG_FRAME_DATA: {
      FrameMessage frameMsg;
      memcpy(&frameMsg, m_accumBuffer.data(), sizeof(FrameMessage));

      // Calculate frame data size from total message size
      size_t frameDataSize = m_bytesNeeded - sizeof(FrameMessage);
      std::vector<uint8_t> frameData(
          m_accumBuffer.begin() + sizeof(FrameMessage),
          m_accumBuffer.begin() + sizeof(FrameMessage) + frameDataSize);

      LOGD("Processing frame data, size={}", frameDataSize);
      if (m_onFrame) {
        m_onFrame(frameMsg, frameData);
      }
      break;
    }
    case MSG_COMPRESSED_FRAME: {
      CompressedFrameMessage compMsg;
      memcpy(&compMsg, m_accumBuffer.data(), sizeof(CompressedFrameMessage));

      // Calculate compressed data size from total message size
      size_t compDataSize = m_bytesNeeded - sizeof(CompressedFrameMessage);
      std::vector<uint8_t> compData(
          m_accumBuffer.begin() + sizeof(CompressedFrameMessage),
          m_accumBuffer.begin() + sizeof(CompressedFrameMessage) +
              compDataSize);

      LOGD("Processing compressed frame, size={}", compDataSize);
      if (m_onCompressedFrame) {
        m_onCompressedFrame(compMsg, compData);
      }
      break;
    }
    case MSG_MOUSE_MOVE: {
      MouseMoveMessage msg;
      memcpy(&msg, m_accumBuffer.data(), sizeof(MouseMoveMessage));

      LOGD("Processing mouse move message");
      if (m_onMouseMove) {
        m_onMouseMove(msg);
      }
      break;
    }
    case MSG_MOUSE_CLICK: {
      MouseClickMessage msg;
      memcpy(&msg, m_accumBuffer.data(), sizeof(MouseClickMessage));

      LOGD("Processing mouse click message");
      if (m_onMouseClick) {
        m_onMouseClick(msg);
      }
      break;
    }
    case MSG_MOUSE_SCROLL: {
      MouseScrollMessage msg;
      memcpy(&msg, m_accumBuffer.data(), sizeof(MouseScrollMessage));

      LOGD("Processing mouse scroll message");
      if (m_onMouseScroll) {
        m_onMouseScroll(msg);
      }
      break;
    }
    case MSG_COMPRESSION_REQUEST: {
      CompressionRequestMessage msg;
      memcpy(&msg, m_accumBuffer.data(), sizeof(CompressionRequestMessage));

      LOGD("Processing compression request message");
      if (m_onCompressionRequest) {
        m_onCompressionRequest(msg);
      }
      break;
    }
    case MSG_PIXEL_FORMAT_REQUEST: {
      PixelFormatRequestMessage msg;
      memcpy(&msg, m_accumBuffer.data(), sizeof(PixelFormatRequestMessage));

      LOGD("Processing pixel format request message");
      if (m_onPixelFormatRequest) {
        m_onPixelFormatRequest(msg);
      }
      break;
    }
  }
}

void AsioConnection::NotifyError(const std::string& message) {
  if (m_onError) {
    // Post the error callback to avoid deadlocks
    std::thread([this, message]() {
      if (m_onError) {
        m_onError(message);
      }
    }).detach();
  }
}

void AsioConnection::RunIoContext() {
  try {
    m_ioContext.run();
  } catch (const std::exception& e) {
    NotifyError("IO context error: " + std::string(e.what()));
  }
}

// AsioServer Implementation
AsioServer::AsioServer() {
  m_acceptor = std::make_unique<tcp::acceptor>(m_ioContext);
}

AsioServer::~AsioServer() {
  Stop();
}

bool AsioServer::Start(int port) {
  try {
    m_acceptor->open(tcp::v4());
    m_acceptor->set_option(tcp::acceptor::reuse_address(true));
    m_acceptor->bind(
        tcp::endpoint(tcp::v4(), static_cast<asio::ip::port_type>(port)));
    m_acceptor->listen();

    m_running = true;
    StartAccept();
    return true;
  } catch (const std::exception& e) {
    LOGE("Server start failed: {}", e.what());
    return false;
  }
}

void AsioServer::Stop() {
  m_running = false;

  if (m_acceptor && m_acceptor->is_open()) {
    asio::error_code ec;
    m_acceptor->close(ec);
  }

  m_ioContext.stop();
}

void AsioServer::Poll() {
  if (m_running) {
    m_ioContext.poll();
  }
}

void AsioServer::StartAccept() {
  auto newConnection = std::make_shared<AsioConnection>();

  m_acceptor->async_accept(
      *newConnection->m_socket,
      [this, newConnection](const asio::error_code& error) {
        HandleAccept(newConnection, error);
      });
}

void AsioServer::HandleAccept(
    std::shared_ptr<AsioConnection> newConnection,
    const asio::error_code& error) {
  if (!error && m_running) {
    newConnection->m_running = true;

    if (m_onClientConnected) {
      m_onClientConnected(newConnection);
    }

    StartAccept(); // Accept next connection
  }
}
