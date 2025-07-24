@echo off
echo MRDesktop Android Tools Downloader
echo ===================================
echo This script downloads minimal Android development tools
echo.

set TOOLS_DIR=%~dp0android-tools
set NDK_VERSION=r25c
set SDK_VERSION=11076708

echo Tools will be installed to: %TOOLS_DIR%
echo.

REM Create tools directory
if not exist "%TOOLS_DIR%" mkdir "%TOOLS_DIR%"

REM Download Android NDK
echo [1/3] Downloading Android NDK %NDK_VERSION%...
set NDK_URL=https://dl.google.com/android/repository/android-ndk-%NDK_VERSION%-windows.zip
set NDK_ZIP=%TOOLS_DIR%\android-ndk-%NDK_VERSION%-windows.zip

if not exist "%TOOLS_DIR%\android-ndk-%NDK_VERSION%" (
    echo   Downloading NDK from: %NDK_URL%
    curl -L -o "%NDK_ZIP%" "%NDK_URL%"
    
    if %errorlevel% neq 0 (
        echo   ERROR: Failed to download NDK
        pause
        exit /b 1
    )
    
    echo   Extracting NDK...
    powershell -Command "Expand-Archive -Path '%NDK_ZIP%' -DestinationPath '%TOOLS_DIR%'"
    del "%NDK_ZIP%"
) else (
    echo   NDK already exists, skipping download
)

REM Download Android Command Line Tools
echo [2/3] Downloading Android SDK Command Line Tools...
set SDK_URL=https://dl.google.com/android/repository/commandlinetools-win-%SDK_VERSION%_latest.zip
set SDK_ZIP=%TOOLS_DIR%\commandlinetools-win-%SDK_VERSION%_latest.zip

if not exist "%TOOLS_DIR%\cmdline-tools" (
    echo   Downloading SDK tools from: %SDK_URL%
    powershell -Command "Invoke-WebRequest -Uri '%SDK_URL%' -OutFile '%SDK_ZIP%'"
    
    if %errorlevel% neq 0 (
        echo   ERROR: Failed to download SDK tools
        pause
        exit /b 1
    )
    
    echo   Extracting SDK tools...
    powershell -Command "Expand-Archive -Path '%SDK_ZIP%' -DestinationPath '%TOOLS_DIR%'"
    del "%SDK_ZIP%"
    
    REM Reorganize cmdline-tools structure
    if exist "%TOOLS_DIR%\cmdline-tools\cmdline-tools" (
        move "%TOOLS_DIR%\cmdline-tools\cmdline-tools" "%TOOLS_DIR%\cmdline-tools\latest"
    )
) else (
    echo   SDK tools already exist, skipping download
)

REM Set up environment
echo [3/3] Setting up environment...
set ANDROID_HOME=%TOOLS_DIR%
set ANDROID_NDK_HOME=%TOOLS_DIR%\android-ndk-%NDK_VERSION%
set PATH=%TOOLS_DIR%\cmdline-tools\latest\bin;%PATH%

echo.
echo Environment setup:
echo   ANDROID_HOME=%ANDROID_HOME%
echo   ANDROID_NDK_HOME=%ANDROID_NDK_HOME%

REM Create environment setup script
echo @echo off > "%TOOLS_DIR%\setup_env.bat"
echo set ANDROID_HOME=%TOOLS_DIR% >> "%TOOLS_DIR%\setup_env.bat"
echo set ANDROID_NDK_HOME=%TOOLS_DIR%\android-ndk-%NDK_VERSION% >> "%TOOLS_DIR%\setup_env.bat"
echo set PATH=%TOOLS_DIR%\cmdline-tools\latest\bin;%%PATH%% >> "%TOOLS_DIR%\setup_env.bat"

echo.
echo ✓ Android tools downloaded successfully!
echo.
echo To use these tools:
echo 1. Run: %TOOLS_DIR%\setup_env.bat
echo 2. Then: build_android.bat debug
echo.
echo Or set these environment variables permanently:
echo   ANDROID_HOME=%TOOLS_DIR%
echo   ANDROID_NDK_HOME=%TOOLS_DIR%\android-ndk-%NDK_VERSION%

pause