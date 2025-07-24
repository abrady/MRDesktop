@echo off
set SERVER_IP=%1

if "%SERVER_IP%"=="" (
    echo Starting MRDesktop Console Client - localhost:8080
    echo This client provides keyboard-only control (WASD/arrows for mouse)
    build\debug\Debug\MRDesktopConsoleClient.exe
) else (
    echo Starting MRDesktop Console Client - %SERVER_IP%:8080
    echo This client provides keyboard-only control (WASD/arrows for mouse)
    build\debug\Debug\MRDesktopConsoleClient.exe --ip=%SERVER_IP%
)

pause