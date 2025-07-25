@echo off
setlocal enabledelayedexpansion

echo MRDesktop Integration Test Suite
echo =================================
echo.

REM Get configuration (default to debug)
set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=debug

echo Configuration: %CONFIG%
echo.

REM Check if binaries exist
if "%CONFIG%"=="debug" (
    set SERVER_EXE=..\build\debug\Debug\MRDesktopServer.exe
    set CLIENT_EXE=..\build\debug\Debug\MRDesktopConsoleClient.exe
) else (
    set SERVER_EXE=..\build\release\Release\MRDesktopServer.exe
    set CLIENT_EXE=..\build\release\Release\MRDesktopConsoleClient.exe
)

if not exist "%SERVER_EXE%" (
    echo ERROR: Server executable not found at %SERVER_EXE%
    echo Please build the project first using: build.bat %CONFIG%
    exit /b 1
)

if not exist "%CLIENT_EXE%" (
    echo ERROR: Client executable not found at %CLIENT_EXE%
    echo Please build the project first using: build.bat %CONFIG%
    exit /b 1
)

echo Found server: %SERVER_EXE%
echo Found client: %CLIENT_EXE%
echo.

REM Kill any existing processes to ensure clean test
taskkill /f /im MRDesktopServer.exe >nul 2>&1
taskkill /f /im MRDesktopConsoleClient.exe >nul 2>&1

echo Starting test server in background...
start /b "" "%SERVER_EXE%" --test

REM Wait a moment for server to start
timeout /t 2 /nobreak >nul

echo Starting test client...
"%CLIENT_EXE%" --test --compression=none

set CLIENT_EXIT_CODE=%ERRORLEVEL%

REM Clean up server process
taskkill /f /im MRDesktopServer.exe >nul 2>&1

echo.
echo =================================
if %CLIENT_EXIT_CODE% equ 0 (
    echo TEST RESULT: PASSED
    echo All frames were transmitted and validated successfully!
) else (
    echo TEST RESULT: FAILED
    echo Frame transmission or validation failed ^(exit code: %CLIENT_EXIT_CODE%^)
)
echo =================================

exit /b %CLIENT_EXIT_CODE%