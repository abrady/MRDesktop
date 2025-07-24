@echo off
set SERVER_IP=%1
set SERVER_PORT=%2

if "%SERVER_IP%"=="" (
    echo Starting MRDesktop Client - localhost:8080
    build\debug\bin\Debug\MRDesktopClient.exe
    goto done
)

if "%SERVER_PORT%"=="" (
    echo Starting MRDesktop Client - %SERVER_IP%:8080
    build\debug\bin\Debug\MRDesktopClient.exe --ip=%SERVER_IP%
    goto done
)

echo Starting MRDesktop Client - %SERVER_IP%:%SERVER_PORT%
build\debug\bin\Debug\MRDesktopClient.exe --ip=%SERVER_IP% --port=%SERVER_PORT%

:done
pause