@echo off
setlocal enabledelayedexpansion

REM Run tests for MRDesktop Linux builds on Windows
REM Usage: run-tests-linux.bat [build_type]

REM Script configuration
set "CONTAINER_ENGINE=podman"
set "IMAGE_NAME=mrdesktop-linux"
set "BUILD_TYPE=%1"
if "%BUILD_TYPE%"=="" set "BUILD_TYPE=debug"

REM Check if container engine is available
where %CONTAINER_ENGINE% >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] %CONTAINER_ENGINE% is not installed or not in PATH
    echo [INFO] Please install Podman Desktop from: https://podman-desktop.io/downloads/windows
    exit /b 1
)

REM Check if image exists
%CONTAINER_ENGINE% images %IMAGE_NAME% >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Container image not found. Run 'python linux/build-linux.py image' first.
    exit /b 1
)

REM Check if build exists (Linux builds use linux-prefixed directories)
if not exist "..\build\linux-%BUILD_TYPE%" (
    echo [WARN] Build directory not found. Building first...
    call python ..\linux\build-linux.py build %BUILD_TYPE%
    if %ERRORLEVEL% neq 0 (
        echo [ERROR] Build failed
        exit /b 1
    )
)

echo [INFO] Running Linux tests ^(%BUILD_TYPE%^) in container...

%CONTAINER_ENGINE% run --rm -v "%CD%\..:/workspace" -w /workspace %IMAGE_NAME% bash -c "cd build/linux-%BUILD_TYPE% && echo 'Running CTest...' && ctest --output-on-failure --verbose && echo '' && echo 'Running individual test executables for detailed output...' && if [ -f 'tests/basic_tests' ]; then echo 'Running basic_tests...' && ./tests/basic_tests --gtest_output=xml:basic_tests_results.xml; fi && if [ -f 'tests/integration_tests' ]; then echo 'Running integration_tests...' && ./tests/integration_tests --gtest_output=xml:integration_tests_results.xml; fi && echo 'Tests completed!'"