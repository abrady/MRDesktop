cmake_minimum_required(VERSION 3.20)
project(MRDesktopAndroid)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include shared source directories
set(SHARED_SRC_DIR "${CMAKE_SOURCE_DIR}/../src/shared")

# For Android builds, use vcpkg headers manually (more reliable than vcpkg integration)
set(VCPKG_ASIO_DIR "${CMAKE_SOURCE_DIR}/../extern/vcpkg/packages/asio_x64-windows/include")
set(VCPKG_SPDLOG_DIR "${CMAKE_SOURCE_DIR}/../extern/vcpkg/packages/spdlog_x64-windows/include")
set(VCPKG_FMT_DIR "${CMAKE_SOURCE_DIR}/../extern/vcpkg/packages/fmt_x64-windows/include")

if(EXISTS "${VCPKG_ASIO_DIR}/asio.hpp")
    message(STATUS "Using asio headers from: ${VCPKG_ASIO_DIR}")
    add_compile_definitions(ASIO_STANDALONE)
else()
    message(FATAL_ERROR "asio headers not found. Please run 'configure.bat' first.")
endif()

if(EXISTS "${VCPKG_SPDLOG_DIR}/spdlog/spdlog.h")
    message(STATUS "Using spdlog headers from: ${VCPKG_SPDLOG_DIR}")
    # Note: SPDLOG_HEADER_ONLY is automatically defined by spdlog when not using compiled lib
else()
    message(FATAL_ERROR "spdlog headers not found. Please run 'configure.bat' first.")
endif()

if(EXISTS "${VCPKG_FMT_DIR}/fmt/base.h")
    message(STATUS "Using fmt headers from: ${VCPKG_FMT_DIR}")
    add_compile_definitions(FMT_HEADER_ONLY)
else()
    message(FATAL_ERROR "fmt headers not found. Please run 'configure.bat' first.")
endif()

# Simple Android native client library for testing
add_library(MRDesktopSimpleClient SHARED
    app/src/main/cpp/simple_android_client.cpp
    ${SHARED_SRC_DIR}/AsioNetworking.cpp
    ${SHARED_SRC_DIR}/Logging.cpp
)

target_include_directories(MRDesktopSimpleClient PRIVATE 
    app/src/main/cpp
    ${SHARED_SRC_DIR}
    ${VCPKG_ASIO_DIR}
    ${VCPKG_SPDLOG_DIR}
    ${VCPKG_FMT_DIR}
)

# Full-featured Android native client library
add_library(MRDesktopClient SHARED
    app/src/main/cpp/android_client.cpp
    app/src/main/cpp/AndroidVideoDecoder.cpp
    app/src/main/cpp/AndroidVideoDecoderWrapper.cpp
    app/src/main/cpp/AndroidSurfaceVideoDecoder.cpp
    app/src/main/cpp/FrameRenderer.cpp
    app/src/main/cpp/CrashSafeFrameHandler.cpp
    ${SHARED_SRC_DIR}/AsioNetworking.cpp
    ${SHARED_SRC_DIR}/NetworkReceiver.cpp
    ${SHARED_SRC_DIR}/Logging.cpp
)

target_include_directories(MRDesktopClient PRIVATE 
    app/src/main/cpp
    ${SHARED_SRC_DIR}
    ${VCPKG_ASIO_DIR}
    ${VCPKG_SPDLOG_DIR}
    ${VCPKG_FMT_DIR}
)

# Find required Android libraries
find_library(log-lib log)
find_library(mediandk-lib mediandk)
find_library(android-lib android)
find_library(gles-lib GLESv2)

target_link_libraries(MRDesktopSimpleClient 
    ${log-lib}
)

target_link_libraries(MRDesktopClient 
    ${log-lib}
    ${mediandk-lib}
    ${android-lib}
    ${gles-lib}
)

# Note: asio, spdlog, and fmt are header-only with compile definitions