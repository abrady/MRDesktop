#!/bin/bash

echo "MRDesktop Android Build Script"
echo "=============================="

# Check for required tools
if ! command -v cmake &> /dev/null; then
    echo "ERROR: CMake not found in PATH"
    echo "Please install CMake and add it to your PATH"
    exit 1
fi

if ! command -v ninja &> /dev/null; then
    echo "ERROR: Ninja not found in PATH"
    echo "Please install Ninja and add it to your PATH"
    echo "You can install it with: sudo apt-get install ninja-build (Ubuntu/Debian)"
    echo "Or download from: https://github.com/ninja-build/ninja/releases"
    exit 1
fi

# Check for Android NDK
if [ -z "$ANDROID_NDK_HOME" ]; then
    echo "ERROR: ANDROID_NDK_HOME environment variable not set"
    echo "Please set ANDROID_NDK_HOME to your Android NDK installation directory"
    echo "Example: export ANDROID_NDK_HOME=~/Android/Sdk/ndk/25.2.9519653"
    exit 1
fi

if [ ! -f "$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" ]; then
    echo "ERROR: Android NDK toolchain not found at $ANDROID_NDK_HOME"
    echo "Please verify your ANDROID_NDK_HOME path"
    exit 1
fi

echo "Found Android NDK at: $ANDROID_NDK_HOME"

# Set environment variables
export CMAKE_ANDROID_NDK="$ANDROID_NDK_HOME"

# Choose build configuration
BUILD_CONFIG="${1:-debug}"

case "$BUILD_CONFIG" in
    "debug")
        PRESET_NAME="android-arm64-debug"
        echo "Building Android ARM64 Debug configuration..."
        ;;
    "release")
        PRESET_NAME="android-arm64-release"
        echo "Building Android ARM64 Release configuration..."
        ;;
    *)
        echo "Invalid build configuration: $BUILD_CONFIG"
        echo "Usage: $0 [debug|release]"
        exit 1
        ;;
esac

echo
echo "Configuring with preset: $PRESET_NAME"
cmake --preset "$PRESET_NAME"

if [ $? -ne 0 ]; then
    echo "Configuration failed!"
    exit 1
fi

echo
echo "Building..."
cmake --build --preset "$PRESET_NAME"

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo
echo "Build successful!"
echo

echo "Native library location:"
if [ "$BUILD_CONFIG" = "debug" ]; then
    echo "  build/android-arm64-debug/libMRDesktopAndroidClient.so"
    BUILD_DIR="build/android-arm64-debug"
else
    echo "  build/android-arm64-release/libMRDesktopAndroidClient.so"
    BUILD_DIR="build/android-arm64-release"
fi

echo
echo "To use with Android Studio:"
echo "1. Copy the .so file to android/app/src/main/jniLibs/arm64-v8a/"
echo "2. Open android/ folder in Android Studio"
echo "3. Build APK normally"

echo
echo "Copying library automatically..."
JNI_DIR="android/app/src/main/jniLibs/arm64-v8a"

mkdir -p "$JNI_DIR"

if [ -f "$BUILD_DIR/libMRDesktopAndroidClient.so" ]; then
    cp "$BUILD_DIR/libMRDesktopAndroidClient.so" "$JNI_DIR/"
    echo "Library copied to JNI directory: $JNI_DIR/libMRDesktopAndroidClient.so"
else
    echo "Warning: Built library not found at expected location"
fi

echo "Done!"