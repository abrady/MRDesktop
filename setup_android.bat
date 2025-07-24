@echo off
echo MRDesktop Android Setup Script
echo ==============================
echo This script will help you set up Android development environment for MRDesktop
echo.

REM Check if Android NDK is already set up
if not "%ANDROID_NDK_HOME%"=="" (
    if exist "%ANDROID_NDK_HOME%\build\cmake\android.toolchain.cmake" (
        echo Android NDK already configured at: %ANDROID_NDK_HOME%
        goto :check_tools
    )
)

echo Setting up Android NDK...
echo.

REM Try to find Android NDK in common locations
set NDK_FOUND=0

REM Check Visual Studio Android NDK
if exist "C:\Microsoft\AndroidNDK64" (
    for /d %%i in ("C:\Microsoft\AndroidNDK64\android-ndk-*") do (
        if exist "%%i\build\cmake\android.toolchain.cmake" (
            echo Found Android NDK: %%i
            set ANDROID_NDK_HOME=%%i
            set NDK_FOUND=1
            goto :ndk_found
        )
    )
)

REM Check Android Studio SDK
if exist "%LOCALAPPDATA%\Android\Sdk\ndk" (
    for /d %%i in ("%LOCALAPPDATA%\Android\Sdk\ndk\*") do (
        if exist "%%i\build\cmake\android.toolchain.cmake" (
            echo Found Android NDK: %%i
            set ANDROID_NDK_HOME=%%i
            set NDK_FOUND=1
            goto :ndk_found
        )
    )
)

:ndk_found
if %NDK_FOUND%==0 (
    echo ERROR: Android NDK not found automatically
    echo.
    echo Please install Android NDK manually:
    echo 1. Install Android Studio
    echo 2. Open SDK Manager (Tools ^> SDK Manager^)
    echo 3. Go to SDK Tools tab
    echo 4. Check "NDK (Side by side^)" and install
    echo.
    echo Or download standalone NDK from:
    echo https://developer.android.com/ndk/downloads
    echo.
    pause
    exit /b 1
)

REM Set environment variable permanently
echo Setting ANDROID_NDK_HOME environment variable...
setx ANDROID_NDK_HOME "%ANDROID_NDK_HOME%" >nul
echo ANDROID_NDK_HOME set to: %ANDROID_NDK_HOME%

:check_tools
echo.
echo Checking required tools...

REM Check CMake
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo WARNING: CMake not found in PATH
    echo Please install CMake from: https://cmake.org/download/
    echo Or install via Visual Studio Installer
) else (
    echo ✓ CMake found
)

REM Check Ninja
where ninja >nul 2>&1
if %errorlevel% neq 0 (
    echo WARNING: Ninja not found in PATH
    echo Please install Ninja from: https://github.com/ninja-build/ninja/releases
    echo Or install via Visual Studio (C++ CMake tools^)
) else (
    echo ✓ Ninja found
)

REM Check Git
where git >nul 2>&1
if %errorlevel% neq 0 (
    echo WARNING: Git not found in PATH
    echo Please install Git from: https://git-scm.com/downloads
) else (
    echo ✓ Git found
)

echo.
echo Setup complete! 
echo.
echo You can now build the Android client with:
echo   build_android.bat debug   - for debug build
echo   build_android.bat release - for release build
echo.
echo Note: You may need to restart your command prompt for environment variables to take effect.
pause