@echo off
echo MRDesktop H.265 Compression Test
echo ==================================
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
start "MRDesktop Test Server H265" "%SERVER_EXE%" --test

echo Waiting 3 seconds for server to start...
timeout /t 3 /nobreak >nul

echo Starting client in test mode with H.265 compression...
"%CLIENT_EXE%" --test --compression=h265

set CLIENT_EXIT_CODE=%ERRORLEVEL%

echo.
echo Cleaning up server process...
taskkill /f /im MRDesktopServer.exe >nul 2>&1

echo.
if %CLIENT_EXIT_CODE% equ 0 (
    echo H.265 COMPRESSION TEST PASSED!
    echo Compressed frames were successfully encoded and decoded.
) else (
    echo H.265 COMPRESSION TEST FAILED with exit code %CLIENT_EXIT_CODE%
)

echo.
echo Press any key to close...
pause >nul
exit /b %CLIENT_EXIT_CODE%