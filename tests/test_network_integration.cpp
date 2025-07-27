#include <chrono>
#include <thread>
#include <gtest/gtest.h>
#include "NetworkReceiver.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

class NetworkIntegrationTest : public ::testing::Test {
 protected:
  void SetUp() override {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    receiver = std::make_unique<NetworkReceiver>();
  }

  void TearDown() override {
    receiver.reset();
#ifdef _WIN32
    WSACleanup();
#endif
  }

  std::unique_ptr<NetworkReceiver> receiver;
  const int TEST_PORT = 18080; // Different from default 8080 to avoid conflicts
};

TEST_F(NetworkIntegrationTest, ConnectToNonExistentServer) {
  // Test connection to a server that doesn't exist
  bool connected = receiver->Connect("127.0.0.1", TEST_PORT);
  EXPECT_FALSE(connected);
}

TEST_F(NetworkIntegrationTest, InvalidHostname) {
  // Test connection with invalid hostname
  bool connected = receiver->Connect("invalid.hostname.test", TEST_PORT);
  EXPECT_FALSE(connected);
}

TEST_F(NetworkIntegrationTest, InvalidPort) {
  // Test connection with invalid port
  bool connected = receiver->Connect("127.0.0.1", -1);
  EXPECT_FALSE(connected);

  connected = receiver->Connect("127.0.0.1", 65536);
  EXPECT_FALSE(connected);
}

// Mock server for testing
class MockServer {
 public:
  MockServer(int port) : port_(port), running_(false) {}

  ~MockServer() { stop(); }

  bool start() {
#ifdef _WIN32
    serverSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket_ == INVALID_SOCKET)
      return false;
#else
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0)
      return false;
#endif

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (bind(serverSocket_, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
      return false;
    }

    if (listen(serverSocket_, 1) < 0) {
      return false;
    }

    running_ = true;
    serverThread_ = std::thread(&MockServer::serverLoop, this);

    // Give server time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    return true;
  }

  void stop() {
    if (running_) {
      running_ = false;
#ifdef _WIN32
      closesocket(serverSocket_);
#else
      close(serverSocket_);
#endif
      if (serverThread_.joinable()) {
        serverThread_.join();
      }
    }
  }

 private:
  void serverLoop() {
    while (running_) {
      sockaddr_in clientAddr{};
      socklen_t clientAddrSize = sizeof(clientAddr);

#ifdef _WIN32
      SOCKET clientSocket =
          accept(serverSocket_, (sockaddr*)&clientAddr, &clientAddrSize);
      if (clientSocket != INVALID_SOCKET) {
        closesocket(clientSocket);
      }
#else
      int clientSocket =
          accept(serverSocket_, (sockaddr*)&clientAddr, &clientAddrSize);
      if (clientSocket >= 0) {
        close(clientSocket);
      }
#endif
    }
  }

  int port_;
  bool running_;
#ifdef _WIN32
  SOCKET serverSocket_;
#else
  int serverSocket_;
#endif
  std::thread serverThread_;
};

TEST_F(NetworkIntegrationTest, ConnectToRunningServer) {
  // Start a mock server
  MockServer server(TEST_PORT);
  ASSERT_TRUE(server.start());

  // Test connection to running server
  bool connected = receiver->Connect("127.0.0.1", TEST_PORT);
  EXPECT_TRUE(connected);

  server.stop();
}