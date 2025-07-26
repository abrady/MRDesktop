@echo off
setlocal enabledelayedexpansion

echo ===============================================
echo MRDesktop Simple Android Build Script
echo ===============================================

REM Set build type (default to debug)
set BUILD_TYPE=debug
if not "%1"=="" set BUILD_TYPE=%1

echo Build Type: %BUILD_TYPE%
echo.

REM Set paths to Android toolchain
set ANDROID_NDK_HOME=%~dp0android\ndk
set ANDROID_TOOLCHAIN=%ANDROID_NDK_HOME%\build\cmake\android.toolchain.cmake

echo Android NDK: %ANDROID_NDK_HOME%
echo Toolchain: %ANDROID_TOOLCHAIN%
echo.

REM Check if NDK exists
if not exist "%ANDROID_NDK_HOME%" (
    echo Error: Android NDK not found at %ANDROID_NDK_HOME%
    echo Please run scripts\fetch_android_toolchain.bat first
    pause
    exit /b 1
)

if not exist "%ANDROID_TOOLCHAIN%" (
    echo Error: Android toolchain file not found
    pause
    exit /b 1
)

REM Create build directory
set BUILD_DIR=build\android-simple-%BUILD_TYPE%
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

echo Building in: %BUILD_DIR%
echo.

REM Configure CMake for Android (using the simple android/CMakeLists.txt)
echo Configuring simple Android build...
pushd android

REM Set CMake variables for Android
set CMAKE_BUILD_TYPE=Debug
if /i "%BUILD_TYPE%"=="release" set CMAKE_BUILD_TYPE=Release

REM Try Ninja first, fall back to NMake if Ninja not available
ninja --version >nul 2>&1
if errorlevel 1 (
    echo Ninja not found, using NMake Makefiles...
    set GENERATOR=NMake Makefiles
) else (
    echo Using Ninja generator...
    set GENERATOR=Ninja
)

cmake -B ..\%BUILD_DIR% ^
    -DCMAKE_TOOLCHAIN_FILE="%ANDROID_TOOLCHAIN%" ^
    -DANDROID_ABI=x86_64 ^
    -DANDROID_NATIVE_API_LEVEL=24 ^
    -DANDROID_STL=c++_shared ^
    -DCMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE% ^
    -G "%GENERATOR%" .

if errorlevel 1 (
    echo Error: Failed to configure Android build
    popd
    pause
    exit /b 1
)

REM Build the project
echo Building Android native library...
cmake --build ..\%BUILD_DIR%

if errorlevel 1 (
    echo Error: Failed to build Android native library
    popd
    pause
    exit /b 1
)

popd

echo.
echo ===============================================
echo Android build completed successfully!
echo ===============================================
echo.

REM Show the built library
echo Built library location: %BUILD_DIR%\libMRDesktopSimpleClient.so
if exist "%BUILD_DIR%\libMRDesktopSimpleClient.so" (
    echo Library size: 
    dir "%BUILD_DIR%\libMRDesktopSimpleClient.so" | findstr "libMRDesktopSimpleClient.so"
    echo.
    echo Simple Android client library is ready for use!
) else (
    echo Warning: Library file not found at expected location
    echo Checking build directory contents:
    dir "%BUILD_DIR%" 2>nul
)

echo.
echo Next steps:
echo   1. Test with emulator: test_android.bat [server_ip] [port]
echo   2. Or manually copy library to Android project
echo.
pause