cmake_minimum_required(VERSION 3.22)
project(rtp-stack LANGUAGES CXX)

option(RTP_BUILD_TESTS "Build RTP stack tests" OFF)
if(RTP_BUILD_TESTS)
  enable_testing()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(rtp_stack
  src/rtp_packet.cpp
  src/rtcp_packet.cpp
  src/rtp_sender.cpp
  src/rtp_receiver.cpp
  src/rtcp_sender.cpp
  src/jitter_buffer.cpp
)

target_include_directories(rtp_stack PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
find_path(ASIO_INCLUDE_DIR asio.hpp)
if(ASIO_INCLUDE_DIR)
  target_include_directories(rtp_stack PUBLIC ${ASIO_INCLUDE_DIR})
endif()


if(RTP_BUILD_TESTS)
  add_subdirectory(tests)
endif()
