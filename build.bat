@echo off
echo Building MRDesktop project...

REM Check if we want Debug or Release build (default to Debug)
set BUILD_TYPE=Debug
if "%1"=="release" set BUILD_TYPE=Release
if "%1"=="Release" set BUILD_TYPE=Release

echo Build type: %BUILD_TYPE%

REM Check if build directory exists
if "%BUILD_TYPE%"=="Debug" (
    set BUILD_DIR=build\debug
) else (
    set BUILD_DIR=build\release
)

if not exist "%BUILD_DIR%" (
    echo Build directory doesn't exist. Run 'configure.bat' first.
    pause
    exit /b 1
)

REM Build the project
cmake --build %BUILD_DIR%

if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build successful!
echo.
echo To run the applications:
echo   run_server.bat           - Start the desktop server
echo   run_client.bat           - Connect to localhost
echo   run_client.bat [IP]      - Connect to specific IP
echo.
echo Example: run_client.bat 192.168.1.100
pause