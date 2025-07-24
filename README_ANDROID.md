# MRDesktop Android Self-Contained Build

This document explains how to build the Android client using standalone CMake (without Android Studio/Gradle).

## 🎯 Self-Contained Build Benefits

- **No Android Studio required** - just CMake and NDK
- **Integration with main build system** - uses same CMakePresets.json
- **vcpkg support** - for future Android dependencies
- **Command-line friendly** - scriptable and CI-ready
- **Cross-platform** - works on Windows, macOS, and Linux

## 📋 Prerequisites

### Windows
1. **CMake** (3.20+)
2. **Ninja** build system
3. **Android NDK** (r23c or newer)
4. **Git** (for vcpkg if needed)

### Quick Setup (Windows)
```batch
# Run automated setup
setup_android.bat

# Or manually set NDK path
set ANDROID_NDK_HOME=C:\Microsoft\AndroidNDK\android-ndk-r25c
```

### Manual Setup (Linux/macOS)
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install cmake ninja-build

# Set NDK path
export ANDROID_NDK_HOME=~/Android/Sdk/ndk/25.2.9519653
```

## 🛠 Building the Android Client

### Method 1: Using Build Scripts

**Windows:**
```batch
# Debug build
build_android.bat debug

# Release build  
build_android.bat release
```

**Linux/macOS:**
```bash
# Debug build
./build_android.sh debug

# Release build
./build_android.sh release
```

### Method 2: Using CMake Presets Directly

```bash
# Configure for Android ARM64 Debug
cmake --preset android-arm64-debug

# Build
cmake --build --preset android-arm64-debug

# For emulator (x86_64)
cmake --preset android-x86_64-debug
cmake --build --preset android-x86_64-debug
```

## 📁 Output Structure

After building, you'll get:
```
build/android-arm64-debug/
├── libMRDesktopAndroidClient.so    # Main native library
├── CMakeFiles/                     # Build artifacts
└── ...

android/app/src/main/jniLibs/arm64-v8a/
└── libMRDesktopAndroidClient.so    # Auto-copied library
```

## 🔧 Integration with Android Studio

The build scripts automatically copy the compiled `.so` library to the correct JNI location:

1. **Build native library**: `build_android.bat debug`
2. **Open Android Studio**: Open the `android/` folder
3. **Build APK**: Use normal Android Studio build process
4. **Install & Test**: The APK will include your native library

## 📐 CMake Configuration Details

### Available Presets
- `android-arm64-debug` - ARM64 debug build (most devices)
- `android-arm64-release` - ARM64 release build  
- `android-x86_64-debug` - x86_64 debug build (emulator)

### Android Build Settings
```cmake
# Automatically configured:
ANDROID_ABI=arm64-v8a           # Target architecture
ANDROID_NATIVE_API_LEVEL=24     # Android 7.0+ (API 24)
ANDROID_STL=c++_shared          # Shared C++ runtime
BUILD_ANDROID_CLIENT=ON         # Enable Android client build
```

## 🔄 vcpkg Integration (Future)

The setup is ready for vcpkg Android dependencies:

```json
// vcpkg.json - add Android-compatible libraries
{
  "dependencies": [
    "openssl",      // Example: for TLS support
    "protobuf"      // Example: for message serialization
  ]
}
```

## 🐛 Troubleshooting

### Common Issues

1. **"Android NDK not found"**
   ```bash
   # Check environment variable
   echo $ANDROID_NDK_HOME
   
   # Should point to NDK root containing build/cmake/android.toolchain.cmake
   ```

2. **"Ninja not found"**
   ```bash
   # Install ninja
   # Windows: Install via Visual Studio or download binary
   # Ubuntu: sudo apt-get install ninja-build
   # macOS: brew install ninja
   ```

3. **"CMake version too old"**
   ```bash
   # Need CMake 3.20+
   cmake --version
   ```

4. **Library not found in APK**
   ```bash
   # Verify library was copied to JNI directory
   ls android/app/src/main/jniLibs/arm64-v8a/
   ```

### Build Verification

```bash
# Check if library was built correctly
file build/android-arm64-debug/libMRDesktopAndroidClient.so

# Should output:
# libMRDesktopAndroidClient.so: ELF 64-bit LSB shared object, ARM aarch64...
```

## 🚀 Advanced Usage

### Custom NDK Path
```bash
export CMAKE_ANDROID_NDK=/custom/path/to/ndk
cmake --preset android-arm64-debug
```

### Different API Levels
```bash
cmake --preset android-arm64-debug -DANDROID_NATIVE_API_LEVEL=28
```

### Multiple ABIs
```bash
# Build for different architectures
cmake --preset android-arm64-debug   # ARM64 devices
cmake --preset android-x86_64-debug  # x86_64 emulator

# Future: Add armeabi-v7a and x86 presets if needed
```

## 📊 Comparison: Standalone vs Android Studio

| Feature | Standalone CMake | Android Studio |
|---------|------------------|----------------|
| Setup Complexity | Minimal | Full IDE |
| Build Speed | Fast | Slower |
| CI/CD Friendly | Yes | Limited |
| Debugging | Command-line | Full IDE |
| APK Generation | Manual | Automatic |
| Dependencies | vcpkg | Gradle |

## 🎯 Recommended Workflow

1. **Development**: Use standalone CMake for fast iteration
2. **Testing**: Copy .so to Android Studio for APK generation
3. **CI/CD**: Use standalone CMake for automated builds
4. **Debugging**: Use Android Studio when needed

This approach gives you the best of both worlds - fast, self-contained builds with the option to use full Android Studio when needed!