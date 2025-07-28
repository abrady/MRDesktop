@echo off
echo Creating modern Android AVD for MRDesktop...

REM Set up Android environment
set JAVA_HOME=C:\open\github\MRDesktop\android\java\jdk-17.0.11+9
set ANDROID_HOME=C:\open\github\MRDesktop\android\sdk
set ANDROID_NDK_HOME=C:\open\github\MRDesktop\android\ndk
set PATH=%ANDROID_HOME%\cmdline-tools\latest\bin;%ANDROID_HOME%\platform-tools;%PATH%

echo.
echo Checking available system images...
echo.

REM Use Android 33 which should be available
echo Installing Android 33 system image...
"%ANDROID_HOME%\cmdline-tools\latest\bin\sdkmanager.bat" "system-images;android-33;google_apis;x86_64"

echo.
echo Creating new AVD: MRDesktop_Modern
echo no | "%ANDROID_HOME%\cmdline-tools\latest\bin\avdmanager.bat" create avd -n MRDesktop_Modern -k "system-images;android-33;google_apis;x86_64" -d pixel_6

echo.
echo AVD created successfully!
echo You can now use run_emulator_modern.bat to start it.
pause