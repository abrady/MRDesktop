@echo off
setlocal enabledelayedexpansion

REM Linux build script for Windows using Podman/Docker
REM Usage: build-linux.bat [command] [build_type]

REM Script configuration
set "CONTAINER_ENGINE=podman"
set "IMAGE_NAME=mrdesktop-linux"
set "BUILD_TYPE=%2"
if "%BUILD_TYPE%"=="" set "BUILD_TYPE=debug"
set "LINUX_DIR=%~dp0linux"

REM Show usage information FIRST
if "%1"=="help" goto :show_help
if "%1"=="-h" goto :show_help
if "%1"=="--help" goto :show_help
if "%1"=="" goto :show_help
goto :skip_help

:show_help
    echo Usage: %0 [command] [build_type]
    echo.
    echo Commands:
    echo   build [debug^|release]  - Build the project ^(default: debug^)
    echo   test  [debug^|release]  - Run tests
    echo   shell                  - Start interactive shell in container
    echo   image                  - Build container image
    echo   clean                  - Clean build artifacts
    echo   help                   - Show this help
    echo.
    echo Environment variables:
    echo   CONTAINER_ENGINE       - Container engine to use ^(podman^|docker, default: podman^)
    echo.
    echo Examples:
    echo   %0 build debug         - Build debug version
    echo   %0 build release       - Build release version
    echo   %0 test                - Run tests
    echo   %0 shell               - Interactive development shell
    echo.
    echo Note: Docker files are located in the linux/ subdirectory
    goto :eof

:skip_help

REM Check if linux directory exists
if not exist "%LINUX_DIR%" (
    echo [ERROR] Linux directory not found at: %LINUX_DIR%
    echo [INFO] Please ensure the linux/ subdirectory exists with Docker files
    exit /b 1
)

REM Check if container engine is available
where %CONTAINER_ENGINE% >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] %CONTAINER_ENGINE% is not installed or not in PATH
    echo [INFO] Please install Podman Desktop from: https://podman-desktop.io/downloads/windows
    echo [INFO] Or set CONTAINER_ENGINE=docker if you prefer Docker
    exit /b 1
)

REM Function to build the container image
if "%1"=="image" (
    echo [INFO] Building %CONTAINER_ENGINE% image from linux/ directory...
    pushd "%LINUX_DIR%"
    %CONTAINER_ENGINE% build -t %IMAGE_NAME% .
    set "BUILD_RESULT=!ERRORLEVEL!"
    popd
    if !BUILD_RESULT! equ 0 (
        echo [INFO] Image built successfully
    ) else (
        echo [ERROR] Failed to build image
        exit /b 1
    )
    goto :eof
)

REM Function to clean build artifacts
if "%1"=="clean" (
    echo [INFO] Cleaning build artifacts...
    if exist build\debug rmdir /s /q build\debug 2>nul
    if exist build\release rmdir /s /q build\release 2>nul
    if exist install\debug rmdir /s /q install\debug 2>nul
    if exist install\release rmdir /s /q install\release 2>nul
    echo [INFO] Clean completed
    goto :eof
)

REM For commands that need the container image, check if it exists
%CONTAINER_ENGINE% images %IMAGE_NAME% --format "{{.Repository}}" 2>nul | findstr /c:"%IMAGE_NAME%" >nul
if %ERRORLEVEL% neq 0 (
    echo [WARN] Container image not found, building it first...
    pushd "%LINUX_DIR%"
    %CONTAINER_ENGINE% build -t %IMAGE_NAME% .
    set "BUILD_RESULT=!ERRORLEVEL!"
    popd
    if !BUILD_RESULT! neq 0 (
        echo [ERROR] Failed to build image
        exit /b 1
    )
)

REM Function to run the build inside container
if "%1"=="build" (
    echo [INFO] Running Linux build ^(%BUILD_TYPE%^) in container...
    
    REM Create build directory if it doesn't exist
    if not exist build mkdir build

    %CONTAINER_ENGINE% run --rm -v "%CD%:/workspace" -w /workspace %IMAGE_NAME% bash -c "echo 'Configuring CMake for Linux %BUILD_TYPE%...' && cmake --preset linux-%BUILD_TYPE% && echo 'Building project...' && cmake --build build/%BUILD_TYPE% --parallel $(nproc) && echo 'Build completed successfully!' && echo 'Server binary: build/%BUILD_TYPE%/MRDesktopServer' && echo 'Console client binary: build/%BUILD_TYPE%/MRDesktopConsoleClient'"
    goto :eof
)

REM Function to run tests
if "%1"=="test" (
    echo [INFO] Running tests in container...
    %CONTAINER_ENGINE% run --rm -v "%CD%:/workspace" -w /workspace %IMAGE_NAME% bash -c "if [ -d 'build/%BUILD_TYPE%' ]; then cd build/%BUILD_TYPE% && ctest --output-on-failure; else echo 'No build directory found. Run build first.' && exit 1; fi"
    goto :eof
)

REM Function to run an interactive shell in the container
if "%1"=="shell" (
    echo [INFO] Starting interactive shell in container...
    %CONTAINER_ENGINE% run --rm -it -v "%CD%:/workspace" -w /workspace %IMAGE_NAME% bash
    goto :eof
)

echo [ERROR] Unknown command: %1
echo Run '%0 help' for usage information
exit /b 1