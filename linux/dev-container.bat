@echo off
REM MRDesktop Linux Development Container Script (Windows Batch)
REM This script creates a Linux development environment with the project mounted as a volume

setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%.."
set "CONTAINER_NAME=mrdesktop-dev"
set "IMAGE_NAME=mrdesktop:linux-dev"

echo MRDesktop Linux Development Container
echo =====================================
echo Project root: %PROJECT_ROOT%
echo.

REM Check if Podman is available
podman info >nul 2>&1
if errorlevel 1 (
    echo Error: Podman is not running or not accessible
    echo Please install/start Podman and try again
    pause
    exit /b 1
)

REM Parse command line arguments
set "COMMAND=%~1"
if "%COMMAND%"=="" set "COMMAND=shell"

if "%COMMAND%"=="build" goto :build
if "%COMMAND%"=="clean" goto :clean
if "%COMMAND%"=="shell" goto :shell
if "%COMMAND%"=="help" goto :help
if "%COMMAND%"=="-h" goto :help
if "%COMMAND%"=="--help" goto :help

echo Unknown command: %COMMAND%
echo Use '%~nx0 help' for usage information
pause
exit /b 1

:build
echo Building Docker image...
cd /d "%SCRIPT_DIR%"
podman build -t "%IMAGE_NAME%" .
if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)
echo Docker image built successfully!
echo.
goto :end

:clean
echo Stopping and removing existing container...
podman stop "%CONTAINER_NAME%" >nul 2>&1
podman rm "%CONTAINER_NAME%" >nul 2>&1

echo Checking if image exists...
podman image inspect "%IMAGE_NAME%" >nul 2>&1
if not errorlevel 1 (
    echo Removing Podman image...
    podman rmi "%IMAGE_NAME%"
    echo Image removed!
) else (
    echo Image not found, nothing to clean
)
goto :end

:shell
REM Check if image exists
podman image inspect "%IMAGE_NAME%" >nul 2>&1
if errorlevel 1 (
    echo Podman image not found. Building...
    call :build
)

REM Stop and remove existing container
podman stop "%CONTAINER_NAME%" >nul 2>&1
podman rm "%CONTAINER_NAME%" >nul 2>&1

echo Starting development container...
echo Project directory will be mounted at /workspace
echo Any changes made in the container will sync with your host filesystem.
echo.
echo To exit the container, type 'exit'
echo.

podman run -it ^
    --name "%CONTAINER_NAME%" ^
    --rm ^
    -v "%PROJECT_ROOT%:/workspace" ^
    -w /workspace ^
    -p 8080:8080 ^
    -p 8081:8081 ^
    --cap-add=SYS_PTRACE ^
    "%IMAGE_NAME%" ^
    /bin/bash

goto :end

:help
echo Usage: %~nx0 [command]
echo.
echo Commands:
echo   shell     Start development container (default)
echo   build     Build the Docker image
echo   clean     Remove container and image
echo   help      Show this help message
echo.
echo The container mounts your project directory at /workspace
echo Changes made in the container sync with your host filesystem.
echo.
echo Port forwarding:
echo   8080 - MRDesktop server port
echo   8081 - Additional development port
echo.
goto :end

:end
if "%COMMAND%"=="shell" (
    echo.
    echo Container exited. Press any key to continue...
    pause >nul
)