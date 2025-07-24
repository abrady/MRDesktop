@echo off
echo Running MRDesktop...

REM Check if we want Debug or Release build (default to Debug)
set BUILD_TYPE=Debug
if "%1"=="release" set BUILD_TYPE=Release
if "%1"=="Release" set BUILD_TYPE=Release

REM Set the executable path
if "%BUILD_TYPE%"=="Debug" (
    set EXE_PATH=build\debug\Debug\mrdesktop.exe
) else (
    set EXE_PATH=build\release\Release\mrdesktop.exe
)

REM Check if executable exists
if not exist "%EXE_PATH%" (
    echo Executable not found at %EXE_PATH%
    echo Make sure you've run 'configure.bat' and 'build.bat' first
    pause
    exit /b 1
)

REM Run the executable
echo Running %EXE_PATH%...
"%EXE_PATH%"

echo.
echo Program finished. Press any key to close...
pause >nul