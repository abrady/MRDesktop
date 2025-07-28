@echo off
echo Starting Android Emulator for MRDesktop...

REM Set up Android environment
set JAVA_HOME=C:\open\github\MRDesktop\android\java\jdk-17.0.11+9
set ANDROID_HOME=C:\open\github\MRDesktop\android\sdk
set ANDROID_NDK_HOME=C:\open\github\MRDesktop\android\ndk

echo Available AVDs:
"%ANDROID_HOME%\emulator\emulator.exe" -list-avds

echo.
echo Starting MRDesktop_Test emulator...
echo (Close this window to stop the emulator)
echo.
echo Using compatibility settings for older emulator...
"%ANDROID_HOME%\emulator\emulator.exe" -avd MRDesktop_Test -no-skin -gpu swiftshader_indirect

pause