cmake_minimum_required(VERSION 3.20)
project(MRDesktopWindowsClient)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig QUIET)
find_package(FFMPEG REQUIRED)

# Source files
set(CLIENT_SOURCES
    src/main.cpp
    src/VideoRenderer.cpp
    src/SimpleVideoRenderer.cpp
    src/InputHandler.cpp
    src/WindowManager.cpp
    # Shared library files
    ../../shared/NetworkReceiver.cpp
    ../../shared/VideoDecoder.cpp
)

# Header files  
set(CLIENT_HEADERS
    include/VideoRenderer.h
    include/SimpleVideoRenderer.h
    include/InputHandler.h
    include/WindowManager.h
    # Shared library headers
    ../../shared/NetworkReceiver.h
    ../../shared/FrameUtils.h
    ../../shared/protocol.h
)

# Create the Windows client executable
add_executable(MRDesktopWindowsClient WIN32 ${CLIENT_SOURCES} ${CLIENT_HEADERS})

# Include directories
target_include_directories(MRDesktopWindowsClient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared
    ${FFMPEG_INCLUDE_DIRS}
)

# Windows-specific libraries
if(WIN32)
    target_compile_definitions(MRDesktopWindowsClient PRIVATE 
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
    
    target_link_libraries(MRDesktopWindowsClient PRIVATE
        # FFmpeg libraries
        ${FFMPEG_LIBRARIES}
        # Network
        ws2_32
        # Graphics  
        gdi32
        user32
        d2d1
        dwrite
        # Windows API
        kernel32
        shell32
        ole32
        oleaut32
        uuid
        comdlg32
        advapi32
        # Media Foundation for H.264 decoding (when implemented)
        mf
        mfplat
        mfuuid
    )
endif()

# Compiler-specific options
if(MSVC)
    target_compile_options(MRDesktopWindowsClient PRIVATE
        /W4
        /permissive-
        /Zc:__cplusplus
    )
else()
    target_compile_options(MRDesktopWindowsClient PRIVATE
        -Wall
        -Wextra
        -pedantic
    )
endif()

# Set output directory
set_target_properties(MRDesktopWindowsClient PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    OUTPUT_NAME "MRDesktopWindowsClient"
)