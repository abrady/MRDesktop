cmake_minimum_required(VERSION 3.20)
project(MRDesktopAndroidTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Simple Android native client library for testing
add_library(MRDesktopSimpleClient SHARED
    app/src/main/cpp/simple_android_client.cpp
)

target_include_directories(MRDesktopSimpleClient PRIVATE 
    app/src/main/cpp
)

# Find required Android libraries
find_library(log-lib log)

target_link_libraries(MRDesktopSimpleClient 
    ${log-lib}
)