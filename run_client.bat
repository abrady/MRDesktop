@echo off
set SERVER_IP=%1
set SERVER_PORT=%2

if "%SERVER_IP%"=="" (
    echo Starting MRDesktop Windows Client - localhost:8080
    build\debug\bin\Debug\MRDesktopWindowsClient.exe
    goto done
)

if "%SERVER_PORT%"=="" (
    echo Starting MRDesktop Windows Client - %SERVER_IP%:8080
    build\debug\bin\Debug\MRDesktopWindowsClient.exe --ip=%SERVER_IP%
    goto done
)

echo Starting MRDesktop Windows Client - %SERVER_IP%:%SERVER_PORT%
build\debug\bin\Debug\MRDesktopWindowsClient.exe --ip=%SERVER_IP% --port=%SERVER_PORT%

:done
pause