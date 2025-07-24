@echo off
echo MRDesktop Application Launcher
echo =============================

REM Check if we want Debug or Release build (default to Debug)
set BUILD_TYPE=Debug
set APP_TYPE=%1

REM Handle build type parameter
if "%2"=="release" set BUILD_TYPE=Release
if "%2"=="Release" set BUILD_TYPE=Release

REM Check what application to run
if "%APP_TYPE%"=="server" goto run_server
if "%APP_TYPE%"=="client" goto run_client

REM Show usage if no valid app type specified
echo Usage: run.bat [server^|client] [debug^|release]
echo.
echo Examples:
echo   run.bat server        - Run MRDesktopServer (Debug)
echo   run.bat client        - Run MRDesktopClient (Debug)
echo   run.bat server release - Run MRDesktopServer (Release)
echo   run.bat client release - Run MRDesktopClient (Release)
pause
exit /b 1

:run_server
echo Running MRDesktop Server (%BUILD_TYPE%)...
if "%BUILD_TYPE%"=="Debug" (
    set EXE_PATH=build\debug\Debug\MRDesktopServer.exe
) else (
    set EXE_PATH=build\release\Release\MRDesktopServer.exe
)
goto check_and_run

:run_client
echo Running MRDesktop Client (%BUILD_TYPE%)...
if "%BUILD_TYPE%"=="Debug" (
    set EXE_PATH=build\debug\Debug\MRDesktopClient.exe
) else (
    set EXE_PATH=build\release\Release\MRDesktopClient.exe
)
goto check_and_run

:check_and_run
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