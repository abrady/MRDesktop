@echo off
echo Configuring MRDesktop project...

REM Check if we want Debug or Release build (default to Debug)
set BUILD_TYPE=Debug
if "%1"=="release" set BUILD_TYPE=Release
if "%1"=="Release" set BUILD_TYPE=Release

echo Build type: %BUILD_TYPE%

REM Configure with the appropriate preset
if "%BUILD_TYPE%"=="Debug" (
    cmake --preset windows-debug
) else (
    cmake --preset windows-release
)

if %errorlevel% neq 0 (
    echo Configuration failed!
    pause
    exit /b 1
)

echo Configuration successful!
echo Run 'build.bat' to build the project
pause