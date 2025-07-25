cmake_minimum_required(VERSION 3.22.1)

project("mrdesktop-android-client")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_library(log-lib log)
find_library(android-lib android)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/shared
)

# Copy shared protocol header to cpp directory
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/shared/protocol.h
    ${CMAKE_CURRENT_SOURCE_DIR}/protocol.h
    COPYONLY
)

# Source files
set(NATIVE_SRCS
    native_client.cpp
    ../../../src/shared/FrameReceiver.cpp
    input_sender.cpp
)

# Create shared library
add_library(${CMAKE_PROJECT_NAME} SHARED ${NATIVE_SRCS})

# Link libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    ${log-lib}
    ${android-lib}
)