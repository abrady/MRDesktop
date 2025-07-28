@echo off
echo Starting Modern Android Emulator for MRDesktop...

REM Set up Android environment
set JAVA_HOME=C:\open\github\MRDesktop\android\java\jdk-17.0.11+9
set ANDROID_HOME=C:\open\github\MRDesktop\android\sdk
set ANDROID_NDK_HOME=C:\open\github\MRDesktop\android\ndk

echo Available AVDs:
"%ANDROID_HOME%\emulator\emulator.exe" -list-avds

echo.
echo Starting MRDesktop_Modern emulator with optimized settings...
echo (Close this window to stop the emulator)
echo.

REM Modern emulator with hardware acceleration and performance optimizations
"%ANDROID_HOME%\emulator\emulator.exe" -avd MRDesktop_Modern ^
    -gpu host ^
    -memory 4096 ^
    -cores 4

pause