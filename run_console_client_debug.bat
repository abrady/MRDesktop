@echo off
echo MRDesktop Console Client - Debug Mode
echo ====================================
echo.
echo This will run the console client in debug mode, capturing the first 5 frames
echo and saving them to the debug_frames/ directory for analysis.
echo.
echo Usage examples:
echo   Default (127.0.0.1:8080, 5 frames): %~n0
echo   Custom server: %~n0 --ip=192.168.1.100 --port=8080
echo   More frames: %~n0 --debug-frames=10
echo.

if not exist "build\debug\Debug\MRDesktopConsoleClient.exe" (
    echo Error: Console client not found. Please build the project first.
    echo Run: cmake --build build/debug --target MRDesktopConsoleClient
    pause
    exit /b 1
)

echo Running console client in debug mode...
echo Press Ctrl+C to stop
echo.

"build\debug\Debug\MRDesktopConsoleClient.exe" --debug-frames %*

echo.
echo Console client finished.
if exist "debug_frames\" (
    echo.
    echo Debug frames have been saved to debug_frames/ directory:
    dir debug_frames /b
    echo.
    echo Check frame_metadata.txt for detailed information about captured frames.
)
pause