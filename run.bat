@echo off
echo MRDesktop Application Launcher
echo =============================

REM Default values
set BUILD_TYPE=Debug
set APP_TYPE=%1
set SERVER_IP=

REM Check first parameter (app type)
if "%APP_TYPE%"=="" goto show_usage
if "%APP_TYPE%"=="server" goto param_ok
if "%APP_TYPE%"=="client" goto param_ok
goto show_usage

:param_ok

REM Handle second parameter
if "%2"=="release" set BUILD_TYPE=Release
if "%2"=="Release" set BUILD_TYPE=Release

REM For client, check if second param is an IP address (not release)
if "%APP_TYPE%"=="client" (
    if not "%2"=="" (
        if not "%2"=="release" (
            if not "%2"=="Release" (
                set SERVER_IP=%2
            )
        )
    )
)

REM Route to appropriate handler
if "%APP_TYPE%"=="server" goto run_server
if "%APP_TYPE%"=="client" goto run_client

:show_usage
echo Usage: run.bat [server^|client] [ip-address^|release]
echo.
echo Examples:
echo   run.bat server              - Run MRDesktopServer (Debug)
echo   run.bat client              - Run MRDesktopClient (Debug, localhost)
echo   run.bat client 192.168.1.100 - Run MRDesktopClient connecting to IP
echo   run.bat client release       - Run MRDesktopClient (Release, localhost)
echo   run.bat server release       - Run MRDesktopServer (Release)
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
if "%SERVER_IP%"=="" (
    echo Running MRDesktop Client (%BUILD_TYPE%) - localhost:8080...
) else (
    echo Running MRDesktop Client (%BUILD_TYPE%) - connecting to %SERVER_IP%:8080...
)
if "%BUILD_TYPE%"=="Debug" (
    set EXE_PATH=build\debug\bin\Debug\MRDesktopClient.exe
) else (
    set EXE_PATH=build\release\bin\Release\MRDesktopClient.exe
)
goto check_and_run

:check_and_run
REM Check if executable exists
if not exist "%EXE_PATH%" (
    echo Executable not found at %EXE_PATH%
    echo Make sure you've built the project first
    pause
    exit /b 1
)

REM Run the executable
echo Running %EXE_PATH%...
if "%APP_TYPE%"=="client" if not "%SERVER_IP%"=="" (
    "%EXE_PATH%" --ip=%SERVER_IP%
) else (
    "%EXE_PATH%"
)

echo.
echo Program finished. Press any key to close...
pause >nul