# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

MRDesktop is a VR/MR Virtual Desktop project for Meta Quest 3 that enables viewing and controlling a PC or Mac screen inside a VR headset using passthrough mode. The system streams the desktop to the headset in real-time while allowing interactive control through VR controllers.

## Project Architecture

The system consists of four main components:

### 1. Desktop Host (Windows/macOS/Linux)

Located in `src/server/main.cpp`:

- **Screen Capture**: Uses DXGI Desktop Duplication API on Windows for efficient frame capture
- **Network Server**: TCP socket server listening on port 8080 for client connections
- **Input Processing**: Receives and processes mouse/keyboard commands from clients
- **Frame Streaming**: Sends captured desktop frames to connected clients

### 2. Console Client (Testing/Development)

Located in `src/clients/console/main.cpp`:

- **Network Client**: Connects to desktop host for receiving frames and sending input
- **Input Simulation**: Keyboard-based input (WASD/arrows for mouse movement)
- **Frame Reception**: Receives and saves desktop frames (creates `first_frame.bmp`)

### 3. Windows Client (GUI Development)

Located in `src/clients/windows/`:

- **Video Rendering**: Hardware-accelerated frame display
- **Input Handling**: Native Windows input processing
- **Window Management**: Full-screen and windowed display modes

### 4. RTP Stack Module

Located in `rtp-stack/`:

- **Standalone Module**: Independent RTP/RTCP implementation that can be built and tested in isolation
- **RTP Packet Handling**: Creates, parses, and validates RTP packets for real-time media streaming
- **RTCP Control**: Implements RTCP sender/receiver reports for quality monitoring and feedback
- **Jitter Buffer**: Manages packet reordering and timing for smooth media playback
- **Integration Ready**: Designed as a reusable library for frame streaming in the main MRDesktop project
- **Independent Build**: Has its own CMakeLists.txt for standalone development and testing

### 5. Android VR Client (Quest 3)

Located in `android/app/src/main/cpp/`:

- **Native Library**: C++ implementation for Quest integration
- **Frame Receiver**: Handles incoming video stream from desktop host
- **Input Sender**: Converts VR controller input to desktop commands
- **VR Integration**: Renders desktop in Quest passthrough environment

## Build System

The project uses CMake with cross-platform presets defined in `CMakePresets.json`:

### Desktop Builds (Windows/macOS/Linux)

- **Windows**: `windows-debug`, `windows-release` (Visual Studio 2022)
- **macOS**: `macos-debug`, `macos-release` (Ninja)
- **Linux**: `linux-debug`, `linux-release` (Ninja)

### Android Builds

- **ARM64**: `android-arm64-debug`, `android-arm64-release` (for devices)
- **OpenXR**: `android-openxr-arm64-debug` (for Quest builds with OpenXR)
- **x86_64**: `android-x86_64-debug`, `android-x86_64-release` (for emulator testing)

## Development Commands

### Cross-Platform Development (Windows/macOS/Linux)

```bash
# Configure project (Debug by default, or specify 'release')
python configure.py [debug|release]

# Build project (Debug by default, or specify 'release')  
python build.py [debug|release]

# Build with specific number of parallel jobs
python build.py debug -j 8

# Run server (listens on port 8080)
python run_server.py

# Run console client (connects to localhost:8080 or specific IP)
python run_console_client.py [IP_ADDRESS]

# Run integration tests
python scripts/run_test.py [debug|release]

# Format code using clang-format
python scripts/format.py
```

**Note**: The Python scripts (`configure.py` and `build.py`) work cross-platform and automatically detect your OS to use the appropriate CMake presets and build directories.

### Linux Development (using Docker/Podman)

```bash
# Build project using container
python linux/build-linux.py build [debug|release]

# Run tests in container
python linux/build-linux.py test [debug|release]

# Start interactive development shell (can use Python scripts inside)
python linux/build-linux.py shell

# Build container image
python linux/build-linux.py image

# Clean build artifacts
python linux/build-linux.py clean

# Alternative: Use Python scripts directly in container shell
# python linux/build-linux.py shell
# python3 configure.py debug
# python3 build.py debug

# Run compression tests
python run_compression_test.py
```

### Android Development

```batch
# Setup Android NDK environment (first time only)
setup_android.bat

# Alternative setup if Android Studio not installed:
tools\download_android_tools.bat
tools\build_apk_standalone.bat

# Build Android native library
build_android.bat [debug|release]

# Build APK (requires Android Studio)
cd android
# Open in Android Studio and build normally

# For OpenXR Quest builds (requires vcpkg and openxr-loader):
# Use android-openxr-arm64-debug preset
```

### RTP Stack Development

```batch
# Build and test RTP stack independently
cd rtp-stack

# Configure (uses parent project's presets)
cmake --preset windows-debug

# Build standalone RTP stack
cmake --build out/build/x64 --config Debug

# Run RTP stack tests
cd out/build/x64 && ctest --output-on-failure
```

## Testing

### Unit and Integration Tests

```batch
# Windows - Run all tests
python scripts/run_test.py [debug|release]

# Linux - Run tests in container
python linux/build-linux.py test [debug|release]

# Run specific test suite manually
cd build/debug && ctest --output-on-failure
```

### Desktop Streaming Tests

1. **Start Server**: `python run_server.py` (or `run.bat server`) - Shows "Server listening on port 8080..."
2. **Start Client**: `python run_console_client.py` (or `run.bat client`) - Creates `first_frame.bmp` with desktop screenshot
3. **Verify**: Both show FPS stats, client saves frame proving capture works

**Troubleshooting**:

- If server fails with "Desktop duplication is not available", close any remote desktop connections
- Make sure no other screen capture software is running
- Check Windows firewall isn't blocking port 8080

### Compression Testing

```bash
# Test H.265 compression (cross-platform)
python run_compression_test.py
```

The test suite includes:

- **Basic Tests**: Protocol serialization, video encoding/decoding
- **Integration Tests**: Network communication, frame transmission
- **Compression Tests**: H.265 encode/decode validation
- **RTP Stack Tests**: RTP packet parsing, RTCP functionality, jitter buffer management

## Communication Protocol

Defined in `src/shared/protocol.h`:

- **MSG_FRAME_DATA**: Desktop frame transmission (width, height, pixel data, format)
- **MSG_COMPRESSED_FRAME**: H.264/H.265 compressed frame data
- **MSG_COMPRESSION_REQUEST**: Client requests compression type (H.264/H.265/AV1)
- **MSG_PIXEL_FORMAT_REQUEST**: Client requests pixel format (RGBA/BGRA/ARGB/YUV420)
supported pixel format
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

### Pixel Format Conversion System

The system implements intelligent pixel format negotiation to handle cross-platform differences:

#### **Native Capture Formats**

- **Windows (DXGI)**: Captures in `DXGI_FORMAT_B8G8R8A8_UNORM` (BGRA) - this is the native desktop format
- **Linux/macOS (FFmpeg)**: Typically outputs BGRA from software decoders
- **Android (MediaCodec)**: Hardware decoders output in YUV420 format initially

#### **Client Format Requirements**

- **Windows clients**: Prefer BGRA (matches native capture, no conversion needed)
- **Android clients**: Require ARGB for `Bitmap.Config.ARGB_8888` compatibility
- **Console clients**: Accept BGRA (FFmpeg compatible)

#### **Negotiation Protocol**

1. Client sends `MSG_PIXEL_FORMAT_REQUEST` with preferred format (e.g., `PIXEL_FORMAT_ARGB`)
2. Server detects native capture format using `DXGIFormatToPixelFormat()`
3. Server converts frames only when `serverFormat != clientFormat`

#### **Performance Optimization**

- **Same format**: Zero-cost - conversion skipped entirely
- **BGRA→RGBA**: Fast in-place red/blue channel swap  
- **BGRA→ARGB**: Memory copy with channel reordering for Android compatibility
- **Automatic detection**: Server adapts to actual display format rather than assuming

This system ensures optimal performance while supporting the native formats each platform requires, avoiding unnecessary conversions when client and server formats match.

When compression is enabled the server currently encodes frames with FFmpeg's
H.26x encoders in the `YUV420` format. Pixel format negotiation is ignored for
these compressed frames. The client decodes to YUV and must convert the image to
BGRA or ARGB itself. To deliver compressed frames directly in the negotiated
format you could implement a custom encoder that preserves RGB channels (for
example using `libx264rgb` or a PNG codec) so the decoded frame already matches
the client's request.

### Android Integration

- Native C++ library compiled with Android NDK
- JNI bridge for Quest VR integration
- Hardware-accelerated video decoding preparation

## Dependencies

The project uses vcpkg for dependency management with `vcpkg.json`:

- **asio**: Asynchronous networking library
- **ffmpeg**: Video encoding/decoding with H.264, H.265, hardware acceleration (AMF, NVCODEC, QSV)
- **gtest**: Unit testing framework

### Desktop (Windows)

- **DXGI**: Desktop capture and DirectX integration
- **D3D11**: Graphics device management
- **WinSock2**: Network communication
- **Windows APIs**: Input injection (SendInput)

### Android

- **Android NDK**: Native C++ compilation (API level 24+)
- **CMake**: Build system integration
- **Ninja**: Fast build execution
- **OpenXR**: VR/AR runtime support (for Quest builds)

### Build Requirements

- **Windows**: Visual Studio 2022, vcpkg
- **Linux**: Docker/Podman for containerized builds
- **Android**: Android Studio, NDK, CMake
- **All Platforms**: CMake 3.20+, Git (for vcpkg submodule)

## Code Style and Development Practices

### Logging

- Use the structured logging system defined in `src/shared/Logging.h`
- Set `LOG_TAG` before including `Logging.h` in each source file
- Use appropriate log levels: `LOGD()` (debug), `LOGI()` (info), `LOGW()` (warn), `LOGE()` (error)
- **Never use `std::cout` or `std::cerr`** - always use the logging macros
- Log messages use fmt-style formatting: `LOGI("Processing {} bytes", size)`

### Formatting

- Uses **clang-format** with Meta's Snowplow style guidelines (`.clang-format`)
- Run `python scripts/format.py` to format all source files before committing
- 80-character line limit, 2-space indentation

### Build Configuration

- Always links against **release FFmpeg** libraries (via `VCPKG_BUILD_TYPE=release`)
- Uses CMake presets for consistent cross-platform configuration
- Separate debug/release build directories under `build/`

### Project Layout

```
src/
├── server/           # Desktop host application
├── clients/
│   ├── console/      # Command-line test client
│   ├── windows/      # Windows-specific client code
│   └── unreal/       # Unreal Engine integration
├── shared/           # Common networking, video, protocol code
rtp-stack/            # Standalone RTP/RTCP module
├── include/rtp/      # RTP stack headers (public API)
├── src/              # RTP stack implementation
├── tests/            # RTP stack unit tests
└── CMakeLists.txt    # Standalone build configuration
android/              # Android VR client (Quest)
tests/                # Main project unit and integration tests
scripts/              # Build and utility scripts
linux/                # Docker-based Linux build system
```

## Project Status

This is an active implementation with working desktop streaming between Windows host and client. The Android VR client is in development phase with native library structure in place. The RTP stack module is a standalone component that can be developed and tested independently, designed for future integration into the main streaming pipeline.

## important-instruction-reminders

Do what has been asked; nothing more, nothing less.
NEVER create files unless they're absolutely necessary for achieving your goal.
ALWAYS prefer editing an existing file to creating a new one.
NEVER proactively create documentation files (*.md) or README files. Only create documentation files if explicitly requested by the User.

## Claude Code Tool Preferences for Windows Development

When working on Windows, use these modern tools for better performance and readability:

### File Operations

- **fd** - Fast file finder (installed via winget)
  - Use `fd <pattern>` instead of `dir /s` or PowerShell's `Get-ChildItem -Recurse`
  - Example: `fd "*.cpp"` to find all C++ files
  - Example: `fd main` to find files/directories containing "main"

- **bat** - Enhanced file viewer with syntax highlighting (installed via winget)  
  - Use `bat <file>` instead of `type` for viewing files
  - Provides syntax highlighting, line numbers, and git integration
  - Example: `bat src/server/main.cpp`

- **eza** - Modern directory listing (installed via winget)
  - Use `eza` instead of `dir` for better formatted output
  - Use `eza -la` for detailed listing with file permissions and timestamps
  - Use `eza --tree` for tree view of directories

### Search Operations

- **ripgrep (rg)** - Fast text search (already available)
  - Use `rg <pattern>` instead of `findstr` for searching file contents
  - Example: `rg "LOG_TAG" --type cpp` to search in C++ files only
  - Example: `rg "MSG_FRAME_DATA" src/` to search in src directory

### Windows Command Preference Order

1. **fd** for finding files by name/pattern
2. **rg** for searching file contents  
3. **bat** for viewing files with syntax highlighting
4. **eza** for directory listings
5. Standard Windows commands (dir, type, etc.) as fallback

These tools provide better performance, colored output, and more intuitive syntax than traditional Windows commands.
