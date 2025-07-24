@echo off
echo MRDesktop Standalone APK Builder
echo ================================
echo Building APK without Android Studio using command-line tools
echo.

REM Check if Android tools are set up
if "%ANDROID_HOME%"=="" (
    echo Setting up Android tools environment...
    if exist "%~dp0android-tools\setup_env.bat" (
        call "%~dp0android-tools\setup_env.bat"
    ) else (
        echo ERROR: Android tools not found. Run download_android_tools.bat first.
        pause
        exit /b 1
    )
)

if "%ANDROID_NDK_HOME%"=="" (
    echo ERROR: ANDROID_NDK_HOME not set. Please run download_android_tools.bat
    pause
    exit /b 1
)

echo Using Android tools:
echo   ANDROID_HOME=%ANDROID_HOME%
echo   ANDROID_NDK_HOME=%ANDROID_NDK_HOME%
echo.

REM Step 1: Build native library
echo [1/4] Building native library...
cd /d "%~dp0.."
call build_android.bat debug

if %errorlevel% neq 0 (
    echo ERROR: Native library build failed
    pause
    exit /b 1
)

REM Step 2: Setup Android project structure
echo [2/4] Setting up Android project...
set ANDROID_PROJECT=%~dp0android-standalone
if not exist "%ANDROID_PROJECT%" mkdir "%ANDROID_PROJECT%"

REM Copy Android project files
robocopy "android" "%ANDROID_PROJECT%" /E /XD build .gradle

REM Step 3: Create Gradle wrapper (if not exists)
echo [3/4] Setting up Gradle...
cd /d "%ANDROID_PROJECT%"

if not exist "gradlew.bat" (
    echo   Creating Gradle wrapper...
    
    REM Create basic gradle wrapper
    if not exist "gradle\wrapper" mkdir "gradle\wrapper"
    
    echo distributionBase=GRADLE_USER_HOME > gradle\wrapper\gradle-wrapper.properties
    echo distributionPath=wrapper/dists >> gradle\wrapper\gradle-wrapper.properties
    echo distributionUrl=https\://services.gradle.org/distributions/gradle-8.0-bin.zip >> gradle\wrapper\gradle-wrapper.properties
    echo zipStoreBase=GRADLE_USER_HOME >> gradle\wrapper\gradle-wrapper.properties
    echo zipStorePath=wrapper/dists >> gradle\wrapper\gradle-wrapper.properties
    
    REM Download gradle wrapper jar
    powershell -Command "Invoke-WebRequest -Uri 'https://github.com/gradle/gradle/raw/v8.0.0/gradle/wrapper/gradle-wrapper.jar' -OutFile 'gradle\wrapper\gradle-wrapper.jar'"
    
    REM Create gradlew.bat
    echo @rem > gradlew.bat
    echo @rem Licensed under the Apache License, Version 2.0 >> gradlew.bat
    echo @rem >> gradlew.bat
    echo @if "%%DEBUG%%" == "" @echo off >> gradlew.bat
    echo @rem ## >> gradlew.bat
    echo set DIRNAME=%%~dp0 >> gradlew.bat
    echo if "%%DIRNAME%%" == "" set DIRNAME=. >> gradlew.bat
    echo set APP_BASE_NAME=%%~n0 >> gradlew.bat
    echo set APP_HOME=%%DIRNAME%% >> gradlew.bat
    echo java -jar "%%APP_HOME%%gradle\wrapper\gradle-wrapper.jar" %%* >> gradlew.bat
)

REM Step 4: Build APK
echo [4/4] Building APK...
call gradlew.bat assembleDebug

if %errorlevel% neq 0 (
    echo ERROR: APK build failed
    pause
    exit /b 1
)

echo.
echo ✓ APK built successfully!
echo.
echo APK location: %ANDROID_PROJECT%\app\build\outputs\apk\debug\app-debug.apk
echo.
echo To install on device:
echo   adb install "%ANDROID_PROJECT%\app\build\outputs\apk\debug\app-debug.apk"

pause