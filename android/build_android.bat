@echo off

REM Check for clean argument
if "%1"=="clean" (
    echo Cleaning MRDesktop Android project
) else (
    echo Building MRDesktop Android APK
)

set JAVA_HOME=C:\open\github\MRDesktop\android\java\jdk-17.0.11+9
set ANDROID_HOME=C:\open\github\MRDesktop\android\sdk
set ANDROID_NDK_HOME=C:\open\github\MRDesktop\android\ndk

echo Setting up environment
echo JAVA_HOME=%JAVA_HOME%
echo ANDROID_HOME=%ANDROID_HOME%
echo ANDROID_NDK_HOME=%ANDROID_NDK_HOME%

echo Checking SDK components
if not exist "%ANDROID_HOME%\build-tools\34.0.0" (
    echo Installing build-tools
    "%ANDROID_HOME%\cmdline-tools\latest\bin\sdkmanager.bat" "build-tools;34.0.0"
)
if not exist "%ANDROID_HOME%\platforms\android-34" (
    echo Installing platform
    "%ANDROID_HOME%\cmdline-tools\latest\bin\sdkmanager.bat" "platforms;android-34"
)

set PATH=%JAVA_HOME%\bin;%PATH%

if "%1"=="clean" (
    echo Cleaning with Gradle
    "%~dp0gradlew.bat" clean
    echo Clean complete! Run without 'clean' argument to build.
)

echo Building with Gradle
"%~dp0gradlew.bat" assembleDebug
echo.
echo Build complete!
