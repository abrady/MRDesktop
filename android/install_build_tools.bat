@echo off
echo Installing Android SDK build tools and platforms...

set "JAVA_HOME=C:\open\github\MRDesktop\android\java\jdk-17.0.11+9"
set "ANDROID_SDK_ROOT=C:\open\github\MRDesktop\android\sdk"
set "PATH=%JAVA_HOME%\bin;%ANDROID_SDK_ROOT%\cmdline-tools\latest\bin;%PATH%"

echo Installing platforms and build-tools...
echo y | sdkmanager.bat "platforms;android-34" "build-tools;34.0.0"

echo.
echo Installation complete!
echo.
echo Installed components:
dir "%ANDROID_SDK_ROOT%\platforms" /b 2>nul
dir "%ANDROID_SDK_ROOT%\build-tools" /b 2>nul

pause