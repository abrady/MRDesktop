@echo off
echo Installing and launching MRDesktop Android app...

set ANDROID_HOME=C:\open\github\MRDesktop\android\sdk
set PATH=%ANDROID_HOME%\platform-tools;%PATH%

echo.
echo Checking if emulator/device is connected...
adb devices
if %errorlevel% neq 0 (
    echo ERROR: adb not found or no device connected
    echo Make sure emulator is running or device is connected
    pause
    exit /b 1
)

echo.
echo Installing APK...
adb install -r app\build\outputs\apk\debug\app-debug.apk
if %errorlevel% neq 0 (
    echo ERROR: Failed to install APK
    pause
    exit /b 1
)

echo.
echo Launching MRDesktop app...
adb shell am start -n com.mrdesktop/.MainActivity
if %errorlevel% neq 0 (
    echo ERROR: Failed to launch app
    pause
    exit /b 1
)

echo.
echo Success! MRDesktop app installed and launched.
echo The app should now be running on your emulator/device.
echo.
echo To connect to your desktop server:
echo 1. Make sure your desktop server is running: run.bat server
echo 2. In the app, use IP: 10.0.2.2 and Port: 8080
echo 3. Tap "Simple Connect" to test the connection
pause