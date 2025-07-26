# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

MRDesktop is a VR/MR Virtual Desktop project for Meta Quest 3 that enables viewing and controlling a PC or Mac screen inside a VR headset using passthrough mode. The system streams the desktop to the headset in real-time while allowing interactive control through VR controllers.

## Project Architecture

The system consists of three main components:

### 1. Desktop Host (Windows/macOS)
Located in `src/server/main.cpp`:
- **Screen Capture**: Uses DXGI Desktop Duplication API on Windows for efficient frame capture
- **Network Server**: TCP socket server listening on port 8080 for client connections
- **Input Processing**: Receives and processes mouse/keyboard commands from clients
- **Frame Streaming**: Sends captured desktop frames to connected clients

### 2. Desktop Client (Testing/Development)
Located in `src/client/main.cpp`:
- **Network Client**: Connects to desktop host for receiving frames and sending input
- **Input Simulation**: Sends mouse movement, clicks, and scroll commands for testing
- **Frame Reception**: Receives and saves desktop frames (creates `first_frame.bmp`)

### 3. Android VR Client (Quest 3)
Located in `android/app/src/main/cpp/`:
- **Native Library**: C++ implementation for Quest integration
- **Frame Receiver**: Handles incoming video stream from desktop host
- **Input Sender**: Converts VR controller input to desktop commands
- **VR Integration**: Renders desktop in Quest passthrough environment

## Build System

The project uses CMake with cross-platform presets defined in `CMakePresets.json`:

### Desktop Builds (Windows/macOS)
- **Windows**: `windows-debug`, `windows-release` (Visual Studio 2022)
- **macOS**: `macos-debug`, `macos-release` (Ninja)

### Android Builds
- **ARM64**: `android-arm64-debug`, `android-arm64-release` (for devices)
- **x86_64**: `android-x86_64-debug` (for emulator testing)

## Development Commands

### Windows Desktop Development
```batch
# Configure project (Debug by default, or specify 'release')
configure.bat [release]

# Build project (Debug by default, or specify 'release')
build.bat [release]

# Run server (listens on port 8080)
run.bat server [release]

# Run client (connects to localhost:8080)
run.bat client [release]
```

### Android Development
```batch
# Fetch Android toolchain (first time only)
scripts\fetch_android_toolchain.bat

# Setup Android NDK environment (first time only)
setup_android.bat

# Build Android native library
build_android.bat [debug|release]

# Build APK (requires Android Studio)
cd android
# Open in Android Studio and build normally
```

### Testing Desktop Streaming
1. **Start Server**: `run.bat server` - Shows "Server listening on port 8080..."
2. **Start Client**: `run.bat client` - Creates `first_frame.bmp` with desktop screenshot
3. **Verify**: Both show FPS stats, client saves frame proving capture works

## Communication Protocol

Defined in `src/shared/protocol.h`:
- **MSG_FRAME_DATA**: Desktop frame transmission (width, height, pixel data)
- **MSG_MOUSE_MOVE**: Mouse movement (absolute/relative coordinates)
- **MSG_MOUSE_CLICK**: Mouse button press/release (left/right/middle)
- **MSG_MOUSE_SCROLL**: Mouse wheel scrolling (horizontal/vertical)

## Key Technical Details

### Desktop Capture (Windows)
- Uses DXGI Desktop Duplication API for efficient screen capture
- Hardware-accelerated capture directly from GPU framebuffer
- Handles display resolution and multi-monitor scenarios

### Network Architecture
- TCP-based communication for reliability
- Custom binary protocol for frame data and input messages
- Designed for low-latency streaming (~20-50ms target)

### Android Integration
- Native C++ library compiled with Android NDK
- JNI bridge for Quest VR integration
- Hardware-accelerated video decoding preparation

## Dependencies

### Desktop (Windows)
- **DXGI**: Desktop capture and DirectX integration
- **D3D11**: Graphics device management
- **WinSock2**: Network communication
- **Windows APIs**: Input injection (SendInput)

### Android
- **Android NDK**: Native C++ compilation
- **CMake**: Build system integration
- **Ninja**: Fast build execution

## Project Status

This is an active implementation with working desktop streaming between Windows host and client. The Android VR client is in development phase with native library structure in place.