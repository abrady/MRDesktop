@echo off

echo ===============================================
echo Start QuestCI Emulator (Simple)
echo ===============================================

REM Set paths
set ANDROID_HOME=%~dp0android\sdk
set JAVA_HOME=%~dp0android\java\jdk-17.0.11+9
set PATH=%JAVA_HOME%\bin;%ANDROID_HOME%\emulator;%ANDROID_HOME%\platform-tools;%PATH%

echo Starting questCI emulator...
echo This will open the Android emulator window.
echo.

REM Start emulator directly with minimal options
start "QuestCI Emulator" "%ANDROID_HOME%\emulator\emulator.exe" -avd questCI -no-audio -gpu swiftshader_indirect

echo.
echo Emulator is starting in a separate window...
echo.
echo Wait for the Android home screen to appear (2-3 minutes)
echo Then run: test_questci_android.bat
echo.
echo You can close this command window.
pause