#include "AsioNetworking.h"
#include <iostream>

// AsioConnection Implementation
AsioConnection::AsioConnection() {
  m_socket = std::make_unique<tcp::socket>(m_ioContext);
}

AsioConnection::~AsioConnection() {
  Disconnect();
}

bool AsioConnection::Connect(const std::string& host, int port) {
  try {
    tcp::resolver resolver(m_ioContext);
    auto endpoints = resolver.resolve(host, std::to_string(port));

    asio::connect(*m_socket, endpoints);

    m_running = true;
    StartReceive();
    return true;
  } catch (const std::exception& e) {
    NotifyError("Connect failed: " + std::string(e.what()));
    return false;
  }
}

bool AsioConnection::Listen(int port) {
  // This is for single-connection listening (simplified)
  try {
    tcp::acceptor acceptor(m_ioContext, tcp::endpoint(tcp::v4(), static_cast<asio::ip::port_type>(port)));
    acceptor.accept(*m_socket);

    m_running = true;
    StartReceive();
    return true;
  } catch (const std::exception& e) {
    NotifyError("Listen failed: " + std::string(e.what()));
    return false;
  }
}

void AsioConnection::Disconnect() {
  m_running = false;

  if (m_socket && m_socket->is_open()) {
    asio::error_code ec;
    m_socket->close(ec);
  }

  m_ioContext.stop();


  if (m_onDisconnect) {
    m_onDisconnect();
  }
}

void AsioConnection::Poll() {
  if (m_running) {
    m_ioContext.poll();
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

  std::cout << "AsioConnection::SendCompressionRequest: sending type="
            << msg.header.type << " size=" << msg.header.size
            << " compression=" << compression << std::endl;

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

void AsioConnection::StartReceive() {
  if (!IsConnected()) {
    return;
  }

  if (m_readingHeader) {
    // Read message header first
    asio::async_read(
        *m_socket,
        asio::buffer(&m_currentHeader, sizeof(MessageHeader)),
        [this](const asio::error_code& error, size_t bytesTransferred) {
          HandleReceive(error, bytesTransferred);
        });
  } else {
    // Read message data
    asio::async_read(
        *m_socket,
        asio::buffer(m_receiveBuffer),
        [this](const asio::error_code& error, size_t bytesTransferred) {
          HandleReceive(error, bytesTransferred);
        });
  }
}

void AsioConnection::HandleReceive(
    const asio::error_code& error, size_t bytesTransferred) {
  (void)bytesTransferred;
  if (error) {
    if (error == asio::error::eof) {
      NotifyError("Connection closed by peer");
    } else {
      NotifyError("Receive error: " + error.message());
    }
    Disconnect();
    return;
  }

  if (m_readingHeader) {
    // Header received, now read the complete message structure
    switch (m_currentHeader.type) {
      case MSG_FRAME_DATA: {
        // Read the remaining FrameMessage fields
        m_receiveBuffer.resize(sizeof(FrameMessage) - sizeof(MessageHeader));
        asio::async_read(
            *m_socket,
            asio::buffer(m_receiveBuffer),
            [this](const asio::error_code& error, size_t bytesTransferred) {
              (void)bytesTransferred;
              if (error) {
                NotifyError("Failed to read FrameMessage: " + error.message());
                return;
              }

              // Reconstruct the complete FrameMessage
              FrameMessage frameMsg;
              frameMsg.header = m_currentHeader;
              memcpy(
                  reinterpret_cast<char*>(&frameMsg) + sizeof(MessageHeader),
                  m_receiveBuffer.data(),
                  sizeof(FrameMessage) - sizeof(MessageHeader));

              // Now read the frame data
              m_receiveBuffer.resize(frameMsg.dataSize);
              m_readingHeader = false;
              m_currentFrameMsg = frameMsg; // Store for later processing
              StartReceive();
            });
        return;
      }
      case MSG_COMPRESSED_FRAME: {
        // Read the remaining CompressedFrameMessage fields
        m_receiveBuffer.resize(
            sizeof(CompressedFrameMessage) - sizeof(MessageHeader));
        asio::async_read(
            *m_socket,
            asio::buffer(m_receiveBuffer),
            [this](const asio::error_code& error, size_t bytesTransferred) {
              (void)bytesTransferred;
              if (error) {
                NotifyError(
                    "Failed to read CompressedFrameMessage: " +
                    error.message());
                return;
              }

              // Reconstruct the complete CompressedFrameMessage
              CompressedFrameMessage compMsg;
              compMsg.header = m_currentHeader;
              memcpy(
                  reinterpret_cast<char*>(&compMsg) + sizeof(MessageHeader),
                  m_receiveBuffer.data(),
                  sizeof(CompressedFrameMessage) - sizeof(MessageHeader));

              // Now read the compressed data
              m_receiveBuffer.resize(compMsg.compressedSize);
              m_readingHeader = false;
              m_currentCompressedFrameMsg =
                  compMsg; // Store for later processing
              StartReceive();
            });
        return;
      }
      case MSG_MOUSE_MOVE:
      case MSG_MOUSE_CLICK:
      case MSG_MOUSE_SCROLL:
      case MSG_COMPRESSION_REQUEST: {
        // These messages have fixed size, read remaining data directly
        size_t remainingSize = m_currentHeader.size - sizeof(MessageHeader);
        if (remainingSize > 0) {
          m_receiveBuffer.resize(remainingSize);
          asio::async_read(
              *m_socket,
              asio::buffer(m_receiveBuffer),
              [this](const asio::error_code& error, size_t bytesTransferred) {
                (void)bytesTransferred;
                if (!error) {
                  ProcessMessage();
                  m_readingHeader = true;
                  StartReceive();
                } else {
                  NotifyError(
                      "Failed to read input message: " + error.message());
                }
              });
        } else {
          ProcessMessage();
          m_readingHeader = true;
          StartReceive();
        }
        return;
      }
    }
  } else {
    // Data received, process the complete message
    ProcessMessage();
    m_readingHeader = true;
    StartReceive();
  }
}

void AsioConnection::ProcessMessage() {
  std::cout << "AsioConnection::ProcessMessage: type=" << m_currentHeader.type
            << std::endl;
  switch (m_currentHeader.type) {
    case MSG_FRAME_DATA: {
      std::cout << "Processing frame data" << std::endl;
      if (m_onFrame) {
        m_onFrame(m_currentFrameMsg, m_receiveBuffer);
      }
      break;
    }
    case MSG_COMPRESSED_FRAME: {
      std::cout << "Processing compressed frame" << std::endl;
      if (m_onCompressedFrame) {
        m_onCompressedFrame(m_currentCompressedFrameMsg, m_receiveBuffer);
      }
      break;
    }
    case MSG_MOUSE_MOVE:
    case MSG_MOUSE_CLICK:
    case MSG_MOUSE_SCROLL:
    case MSG_COMPRESSION_REQUEST: {
      std::cout << "Processing input message: type=" << m_currentHeader.type
                << " callback=" << (m_onInput ? "set" : "null") << std::endl;
      if (m_onInput) {
        m_onInput(m_currentHeader, m_receiveBuffer);
      } else {
        std::cout << "No input callback set!" << std::endl;
      }
      break;
    }
    default:
      std::cout << "Unknown message type: " << m_currentHeader.type
                << std::endl;
      break;
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
    m_acceptor->bind(tcp::endpoint(tcp::v4(), port));
    m_acceptor->listen();

    m_running = true;
    StartAccept();
    return true;
  } catch (const std::exception& e) {
    std::cerr << "Server start failed: " << e.what() << std::endl;
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
    newConnection->StartReceive();

    if (m_onClientConnected) {
      m_onClientConnected(newConnection);
    }

    StartAccept(); // Accept next connection
  }
}

