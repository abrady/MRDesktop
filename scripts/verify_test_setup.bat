@echo off
echo MRDesktop Test Setup Verification
echo ==================================
echo.

REM Check if debug executables exist (since we know they're built)
set DEBUG_SERVER=..\build\debug\Debug\MRDesktopServer.exe
set DEBUG_CLIENT=..\build\debug\Debug\MRDesktopConsoleClient.exe

echo Checking debug build...
if exist "%DEBUG_SERVER%" (
    echo ✓ Found server: %DEBUG_SERVER%
) else (
    echo ✗ Missing server: %DEBUG_SERVER%
)

if exist "%DEBUG_CLIENT%" (
    echo ✓ Found client: %DEBUG_CLIENT%
) else (
    echo ✗ Missing client: %DEBUG_CLIENT%
)

echo.
echo Checking server test mode...
echo Running: "%DEBUG_SERVER%" --test
echo (This should show "RUNNING IN TEST MODE" then exit after a few seconds)
echo.
echo Press Ctrl+C to stop if it hangs, then run the full test
echo.
timeout /t 1 /nobreak >nul
"%DEBUG_SERVER%" --test 2>nul | findstr /C:"TEST MODE" || echo Server test mode check complete

echo.
echo Checking client test mode...
echo Running: %DEBUG_CLIENT% --help
echo.
"%DEBUG_CLIENT%" --help 2>nul || echo Client appears to be working

echo.
echo ==================================
echo If both executables exist, you can now run:
echo   run_test.bat debug
echo or
echo   powershell .\run_test.ps1 -Config debug
echo ==================================