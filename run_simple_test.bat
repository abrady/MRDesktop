@echo off
echo MRDesktop Simple Test
echo ====================
echo.

set SERVER_EXE=build\debug\Debug\MRDesktopServer.exe
set CLIENT_EXE=build\debug\Debug\MRDesktopConsoleClient.exe

if not exist "%SERVER_EXE%" (
    echo ERROR: Server not found at %SERVER_EXE%
    echo Please build the project first
    exit /b 1
)

if not exist "%CLIENT_EXE%" (
    echo ERROR: Client not found at %CLIENT_EXE%
    echo Please build the project first
    exit /b 1
)

echo Starting server in test mode...
start "MRDesktop Test Server" "%SERVER_EXE%" --test

echo Waiting 3 seconds for server to start...
timeout /t 3 /nobreak >nul

echo Starting client in test mode...
"%CLIENT_EXE%" --test --compression=none

set CLIENT_EXIT_CODE=%ERRORLEVEL%

echo.
echo Cleaning up server process...
taskkill /f /im MRDesktopServer.exe >nul 2>&1

echo.
if %CLIENT_EXIT_CODE% equ 0 (
    echo TEST PASSED!
) else (
    echo TEST FAILED with exit code %CLIENT_EXIT_CODE%
)

exit /b %CLIENT_EXIT_CODE%