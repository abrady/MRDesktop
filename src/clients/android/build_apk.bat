@echo off
echo Building MRDesktop Android APK...
echo ================================

REM Check if gradlew exists
if not exist "gradlew.bat" (
    echo ERROR: gradlew.bat not found. 
    echo Make sure you're running this from the android directory.
    echo You may need to create the gradle wrapper first.
    pause
    exit /b 1
)

REM Build debug APK
echo Building debug APK...
call gradlew.bat assembleDebug

if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build successful!
echo APK location: app\build\outputs\apk\debug\app-debug.apk
echo.
echo To install on device: adb install app\build\outputs\apk\debug\app-debug.apk
pause