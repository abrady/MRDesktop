cmake_minimum_required(VERSION 3.20)
project(MRDesktopAndroid)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include shared source directories
set(SHARED_SRC_DIR "${CMAKE_SOURCE_DIR}/../src/shared")

# Simple Android native client library for testing
add_library(MRDesktopSimpleClient SHARED
    app/src/main/cpp/simple_android_client.cpp
)

target_include_directories(MRDesktopSimpleClient PRIVATE 
    app/src/main/cpp
)

# Full-featured Android native client library
add_library(MRDesktopClient SHARED
    app/src/main/cpp/android_client.cpp
    ${SHARED_SRC_DIR}/NetworkReceiver.cpp
)

target_include_directories(MRDesktopClient PRIVATE 
    app/src/main/cpp
    ${SHARED_SRC_DIR}
)

# Find required Android libraries
find_library(log-lib log)

target_link_libraries(MRDesktopSimpleClient 
    ${log-lib}
)

target_link_libraries(MRDesktopClient 
    ${log-lib}
)