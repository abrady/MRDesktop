@echo off
echo Configuring MRDesktop project...

REM Check if we want Debug or Release build (default to Debug)
set BUILD_TYPE=Debug
if "%1"=="release" set BUILD_TYPE=Release
if "%1"=="Release" set BUILD_TYPE=Release

echo Build type: %BUILD_TYPE%

REM Always use release libraries from vcpkg (e.g. FFmpeg)
set VCPKG_BUILD_TYPE=release

REM Initialize vcpkg submodule if not already done
if not exist "extern\vcpkg\.git" (
    echo Initializing vcpkg submodule...
    git submodule update --init --recursive extern/vcpkg
    if %errorlevel% neq 0 (
        echo Failed to initialize vcpkg submodule!
        pause
        exit /b 1
    )
)

REM Bootstrap vcpkg if not already done
if not exist "extern\vcpkg\vcpkg.exe" (
    echo Bootstrapping vcpkg...
    call extern\vcpkg\bootstrap-vcpkg.bat
    if %errorlevel% neq 0 (
        echo Failed to bootstrap vcpkg!
        pause
        exit /b 1
    )
)

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