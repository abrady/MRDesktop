#pragma once

#define ASIO_STANDALONE
#include <asio.hpp>
#include "protocol.h"
#include <vector>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>

using asio::ip::tcp;

/**
 * Simple Asio-based networking classes to replace platform-specific socket code
 * Much cleaner than the previous raw socket implementations
 */

class AsioConnection {
public:
    using FrameCallback = std::function<void(const FrameMessage&, const std::vector<uint8_t>&)>;
    using CompressedFrameCallback = std::function<void(const CompressedFrameMessage&, const std::vector<uint8_t>&)>;
    using InputCallback = std::function<void(const MessageHeader&, const std::vector<uint8_t>&)>;
    using ErrorCallback = std::function<void(const std::string&)>;
    using DisconnectCallback = std::function<void()>;

private:
    asio::io_context m_ioContext;
    std::unique_ptr<tcp::socket> m_socket;
    std::thread m_ioThread;
    std::atomic<bool> m_running{false};
    
    // Callbacks
    FrameCallback m_onFrame;
    CompressedFrameCallback m_onCompressedFrame;
    InputCallback m_onInput;
    ErrorCallback m_onError;
    DisconnectCallback m_onDisconnect;
    
    // Receive state
    MessageHeader m_currentHeader;
    FrameMessage m_currentFrameMsg;
    CompressedFrameMessage m_currentCompressedFrameMsg;
    std::vector<uint8_t> m_receiveBuffer;
    bool m_readingHeader = true;
    
    // Connection settings
    CompressionType m_compression = COMPRESSION_NONE;

public:
    AsioConnection();
    ~AsioConnection();
    
    // Client operations
    void SetCompression(CompressionType compression) { m_compression = compression; }
    bool Connect(const std::string& host, int port);
    
    // Server operations  
    bool Listen(int port);
    
    // Send operations
    bool SendFrame(const FrameMessage& frameMsg, const std::vector<uint8_t>& frameData);
    bool SendCompressedFrame(const CompressedFrameMessage& frameMsg, const std::vector<uint8_t>& frameData);
    bool SendMouseMove(int32_t deltaX, int32_t deltaY, bool absolute = false, int32_t x = 0, int32_t y = 0);
    bool SendMouseClick(MouseClickMessage::MouseButton button, bool pressed);
    bool SendMouseScroll(int32_t deltaX, int32_t deltaY);
    bool SendCompressionRequest(CompressionType compression);
    
    // Raw data sending (for testing/debugging)
    bool SendData(const void* data, size_t size);
    
    // Connection management
    void Disconnect();
    bool IsConnected() const { return m_running && m_socket && m_socket->is_open(); }
    
    // Callback setters
    void SetFrameCallback(FrameCallback callback) { m_onFrame = callback; }
    void SetCompressedFrameCallback(CompressedFrameCallback callback) { m_onCompressedFrame = callback; }
    void SetInputCallback(InputCallback callback) { m_onInput = callback; }
    void SetErrorCallback(ErrorCallback callback) { m_onError = callback; }
    void SetDisconnectCallback(DisconnectCallback callback) { m_onDisconnect = callback; }

private:
    void StartReceive();
    void HandleReceive(const asio::error_code& error, size_t bytesTransferred);
    void ProcessMessage();
    void NotifyError(const std::string& message);
    void RunIoContext();
    
    template<typename T>
    bool SendMessage(const T& message);
    
    friend class AsioServer;
};

/**
 * Simple server that accepts connections and creates AsioConnection for each client
 */
class AsioServer {
public:
    using ClientConnectedCallback = std::function<void(std::shared_ptr<AsioConnection>)>;
    
private:
    asio::io_context m_ioContext;
    std::unique_ptr<tcp::acceptor> m_acceptor;
    std::thread m_ioThread;
    std::atomic<bool> m_running{false};
    ClientConnectedCallback m_onClientConnected;
    
public:
    AsioServer();
    ~AsioServer();
    
    bool Start(int port);
    void Stop();
    
    void SetClientConnectedCallback(ClientConnectedCallback callback) {
        m_onClientConnected = callback;
    }
    
private:
    void StartAccept();
    void HandleAccept(std::shared_ptr<AsioConnection> newConnection, const asio::error_code& error);
    void RunIoContext();
};