@echo off
REM Setup Android development environment variables
REM Usage: Call this script to set environment variables in current shell
REM Example: setup_android_env.bat

echo Setting up Android development environment...

REM Set Android environment variables
set JAVA_HOME=C:\open\github\MRDesktop\android\java\jdk-17.0.11+9
set ANDROID_HOME=C:\open\github\MRDesktop\android\sdk
set ANDROID_NDK_HOME=C:\open\github\MRDesktop\android\ndk

REM Add tools to PATH
set PATH=%JAVA_HOME%\bin;%ANDROID_HOME%\platform-tools;%ANDROID_HOME%\tools;%ANDROID_HOME%\cmdline-tools\latest\bin;%PATH%

echo.
echo Android environment configured:
echo   JAVA_HOME=%JAVA_HOME%
echo   ANDROID_HOME=%ANDROID_HOME%
echo   ANDROID_NDK_HOME=%ANDROID_NDK_HOME%
echo.
echo Available commands:
echo   adb              - Android Debug Bridge
echo   gradlew.bat      - Gradle wrapper
echo   java             - Java runtime
echo   javac            - Java compiler
echo.
echo Environment is ready for Android development!