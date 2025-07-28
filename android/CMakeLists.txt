cmake_minimum_required(VERSION 3.20)
project(MRDesktopAndroid)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include shared source directories
set(SHARED_SRC_DIR "${CMAKE_SOURCE_DIR}/../src/shared")

# For Android builds, we need to point to vcpkg asio headers manually
# since the Android toolchain doesn't have access to vcpkg packages
set(VCPKG_ASIO_DIR "${CMAKE_SOURCE_DIR}/../extern/vcpkg/packages/asio_x64-windows/include")

if(EXISTS "${VCPKG_ASIO_DIR}/asio.hpp")
    message(STATUS "Using asio headers from: ${VCPKG_ASIO_DIR}")
    add_compile_definitions(ASIO_STANDALONE)
else()
    message(FATAL_ERROR "asio headers not found. Please ensure vcpkg install asio has been run.")
endif()

# Simple Android native client library for testing
add_library(MRDesktopSimpleClient SHARED
    app/src/main/cpp/simple_android_client.cpp
    ${SHARED_SRC_DIR}/AsioNetworking.cpp
)

target_include_directories(MRDesktopSimpleClient PRIVATE 
    app/src/main/cpp
    ${SHARED_SRC_DIR}
    ${VCPKG_ASIO_DIR}
)

# Full-featured Android native client library
add_library(MRDesktopClient SHARED
    app/src/main/cpp/android_client.cpp
    app/src/main/cpp/AndroidVideoDecoder.cpp
    app/src/main/cpp/AndroidVideoDecoderWrapper.cpp
    app/src/main/cpp/FrameRenderer.cpp
    app/src/main/cpp/CrashSafeFrameHandler.cpp
    ${SHARED_SRC_DIR}/AsioNetworking.cpp
    ${SHARED_SRC_DIR}/NetworkReceiver.cpp
)

target_include_directories(MRDesktopClient PRIVATE 
    app/src/main/cpp
    ${SHARED_SRC_DIR}
    ${VCPKG_ASIO_DIR}
)

# Find required Android libraries
find_library(log-lib log)
find_library(mediandk-lib mediandk)

target_link_libraries(MRDesktopSimpleClient 
    ${log-lib}
)

target_link_libraries(MRDesktopClient 
    ${log-lib}
    ${mediandk-lib}
)

# Note: asio is header-only with ASIO_STANDALONE, no linking required