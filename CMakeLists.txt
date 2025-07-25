cmake_minimum_required(VERSION 3.20)
project(MRDesktop)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SVTAV1 REQUIRED IMPORTED_TARGET SvtAv1Enc)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavcodec libavformat libavutil libswresample libswscale)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option to build Android client
option(BUILD_ANDROID_CLIENT "Build Android native client library" OFF)
option(BUILD_OPENXR_CLIENT "Build OpenXR native client" OFF)

# Platform detection
if(ANDROID)
    set(PLATFORM_ANDROID TRUE)
    set(PLATFORM_DESKTOP FALSE)
elseif(WIN32 OR APPLE OR UNIX)
    set(PLATFORM_ANDROID FALSE)
    set(PLATFORM_DESKTOP TRUE)
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# Note: H.264 compression uses Windows Media Foundation (built into Windows)
# No external codec libraries needed for H.264 support

# Common include directories
set(COMMON_INCLUDES 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shared
)

# Build desktop applications
if(PLATFORM_DESKTOP)
    # Create server executable
    add_executable(MRDesktopServer
        src/server/main.cpp
        src/shared/H264Encoder.cpp
        src/shared/AV1Encoder.cpp
    )
    target_include_directories(MRDesktopServer PRIVATE ${COMMON_INCLUDES} ${SVTAV1_INCLUDE_DIRS} ${FFMPEG_INCLUDE_DIRS})

    # Create console client executable
    add_executable(MRDesktopConsoleClient
        src/clients/console/main.cpp
        src/shared/FrameLogger.cpp
        src/shared/FrameReceiver.cpp
    )
    target_include_directories(MRDesktopConsoleClient PRIVATE ${COMMON_INCLUDES} ${SVTAV1_INCLUDE_DIRS} ${FFMPEG_INCLUDE_DIRS})

    if(WIN32)
        # Server needs DXGI, Media Foundation and other Windows APIs
        target_compile_definitions(MRDesktopServer PRIVATE WIN32_LEAN_AND_MEAN)
        target_link_libraries(MRDesktopServer PRIVATE dxgi d3d11 ws2_32 mf mfplat mfuuid wmcodecdspuuid
            PkgConfig::libavcodec PkgConfig::libavformat PkgConfig::libavutil PkgConfig::libswresample PkgConfig::libswscale
            SvtAv1Enc)
        
        # Console client
        target_compile_definitions(MRDesktopConsoleClient PRIVATE WIN32_LEAN_AND_MEAN)
        target_link_libraries(MRDesktopConsoleClient PRIVATE ws2_32
            PkgConfig::libavcodec PkgConfig::libavformat PkgConfig::libavutil PkgConfig::libswresample PkgConfig::libswscale
            SvtAv1Enc)
    endif()

    if(NOT WIN32)
        target_link_libraries(MRDesktopServer PRIVATE
            PkgConfig::libavcodec PkgConfig::libavformat PkgConfig::libavutil PkgConfig::libswresample PkgConfig::libswscale
            SvtAv1Enc)
        target_link_libraries(MRDesktopConsoleClient PRIVATE
            PkgConfig::libavcodec PkgConfig::libavformat PkgConfig::libavutil PkgConfig::libswresample PkgConfig::libswscale
            SvtAv1Enc)
    endif()
    
    # Add Windows GUI client as a subdirectory
    if(WIN32)
        add_subdirectory(src/clients/windows)
    endif()
endif()

# Build Android client
if(PLATFORM_ANDROID OR BUILD_ANDROID_CLIENT)
    # Android native client library
    add_library(MRDesktopAndroidClient SHARED
        android/app/src/main/cpp/native_client.cpp
        src/shared/FrameReceiver.cpp
        android/app/src/main/cpp/input_sender.cpp
    )
    
    target_include_directories(MRDesktopAndroidClient PRIVATE 
        ${COMMON_INCLUDES}
        android/app/src/main/cpp
    )
    
    if(ANDROID)
        # Find required Android libraries
        find_library(log-lib log)
        find_library(android-lib android)
        
        target_link_libraries(MRDesktopAndroidClient 
            ${log-lib}
            ${android-lib}
        )
    endif()
endif()

# Build OpenXR client
if(BUILD_OPENXR_CLIENT)
    find_package(OpenXR REQUIRED)
    add_executable(MRDesktopOpenXR src/openxr/main.cpp)
    target_include_directories(MRDesktopOpenXR PRIVATE
        ${COMMON_INCLUDES}
    )
    target_link_libraries(MRDesktopOpenXR PRIVATE OpenXR::openxr_loader)
endif()