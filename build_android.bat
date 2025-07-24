@echo off
echo MRDesktop Android Build Script
echo ==============================

REM Check for required tools
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: CMake not found in PATH
    echo Please install CMake and add it to your PATH
    pause
    exit /b 1
)

where ninja >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Ninja not found in PATH
    echo Please install Ninja and add it to your PATH
    echo You can download it from: https://github.com/ninja-build/ninja/releases
    pause
    exit /b 1
)

REM Check for Android NDK
if "%ANDROID_NDK_HOME%"=="" (
    echo ERROR: ANDROID_NDK_HOME environment variable not set
    echo Please set ANDROID_NDK_HOME to your Android NDK installation directory
    echo Example: set ANDROID_NDK_HOME=C:\Microsoft\AndroidNDK\android-ndk-r25c
    pause
    exit /b 1
)

if not exist "%ANDROID_NDK_HOME%\build\cmake\android.toolchain.cmake" (
    echo ERROR: Android NDK toolchain not found at %ANDROID_NDK_HOME%
    echo Please verify your ANDROID_NDK_HOME path
    pause
    exit /b 1
)

echo Found Android NDK at: %ANDROID_NDK_HOME%

REM Set environment variables
set CMAKE_ANDROID_NDK=%ANDROID_NDK_HOME%

REM Choose build configuration
set BUILD_CONFIG=%1
if "%BUILD_CONFIG%"=="" set BUILD_CONFIG=debug

if "%BUILD_CONFIG%"=="debug" (
    set PRESET_NAME=android-arm64-debug
    echo Building Android ARM64 Debug configuration...
) else if "%BUILD_CONFIG%"=="release" (
    set PRESET_NAME=android-arm64-release
    echo Building Android ARM64 Release configuration...
) else (
    echo Invalid build configuration: %BUILD_CONFIG%
    echo Usage: build_android.bat [debug^|release]
    pause
    exit /b 1
)

echo.
echo Configuring with preset: %PRESET_NAME%
cmake --preset %PRESET_NAME%

if %errorlevel% neq 0 (
    echo Configuration failed!
    pause
    exit /b 1
)

echo.
echo Building...
cmake --build --preset %PRESET_NAME%

if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build successful!
echo.
echo Native library location:
if "%BUILD_CONFIG%"=="debug" (
    echo   build\android-arm64-debug\libMRDesktopAndroidClient.so
) else (
    echo   build\android-arm64-release\libMRDesktopAndroidClient.so
)

echo.
echo To use with Android Studio:
echo 1. Copy the .so file to android\app\src\main\jniLibs\arm64-v8a\
echo 2. Open android\ folder in Android Studio
echo 3. Build APK normally

echo.
echo To copy library automatically:
set BUILD_DIR=build\android-arm64-%BUILD_CONFIG%
set JNI_DIR=android\app\src\main\jniLibs\arm64-v8a

if not exist "%JNI_DIR%" mkdir "%JNI_DIR%"

if exist "%BUILD_DIR%\libMRDesktopAndroidClient.so" (
    copy "%BUILD_DIR%\libMRDesktopAndroidClient.so" "%JNI_DIR%\"
    echo Library copied to JNI directory: %JNI_DIR%\libMRDesktopAndroidClient.so
) else (
    echo Warning: Built library not found at expected location
)

pause