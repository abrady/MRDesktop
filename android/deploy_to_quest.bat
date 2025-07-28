@echo off
echo Deploying MRDesktop to Quest (2G0YC6LF600029)...

REM Set up Android environment
set JAVA_HOME=C:\open\github\MRDesktop\android\java\jdk-17.0.11+9
set ANDROID_HOME=C:\open\github\MRDesktop\android\sdk
set ANDROID_NDK_HOME=C:\open\github\MRDesktop\android\ndk
set PATH=%ANDROID_HOME%\platform-tools;%PATH%

echo.
echo Step 1: Check device connection...
adb devices

echo.
echo Step 2: Build APK...
call build_android.bat

echo.
echo Step 3: Stop any running MRDesktop instances...
adb -s 2G0YC6LF600029 shell am force-stop com.mrdesktop
echo Previous instances stopped.

echo.
echo Step 4: Install APK to Quest...
adb -s 2G0YC6LF600029 install -r app\build\outputs\apk\debug\app-debug.apk

echo.
echo Step 5: Launch MRDesktop on Quest...
adb -s 2G0YC6LF600029 shell am start -n com.mrdesktop/.MainActivity

echo.
echo Step 6: Show logs (press Ctrl+C to stop)...
echo Watching MRDesktop logs on Quest...
adb -s 2G0YC6LF600029 logcat | findstr "MRDesk"

pause