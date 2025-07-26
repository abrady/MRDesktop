@echo off
:: fetch_android_toolchain.bat ────────────────────────────────────────────────
:: Self‑contained Android tool‑chain fetcher **for Windows**
:: ---------------------------------------------------------------------------
:: ✔ Designed to live in  <repo>\scripts\fetch_android_toolchain.bat
::   (ROOT is computed as the parent directory of this .bat)
:: ✔ Downloads & unpacks into  <repo>\android\ …
::      ndk\   → Android NDK r27 LTS (Clang‑17)
::      sdk\   → command‑line tools + build‑tools 35.0.0
::      java\  → Portable Temurin OpenJDK 17
:: ✔ Adds JAVA_HOME & sdkmanager to PATH during the run.
:: ✔ Idempotent: skip downloads/extracts if folders already exist.
:: ---------------------------------------------------------------------------
:: 2025‑07‑25 — adjusted: fix path so sdkmanager.bat ends up in
::                  android\sdk\cmdline-tools\latest\bin
::               clarify where to place this script.

setlocal EnableDelayedExpansion

:: ── Derive repo root ─────────────────────────────────────────────────────────
:: %~dp0 = full path of the script incl. trailing backslash
:: parent of that is the repo root
set "ROOT=%~dp0.."
set "TOOLS=%ROOT%\android"

:: ── Version pins ─────────────────────────────────────────────────────────────
set "NDK_VER=r27"                       :: NDK r27 LTS (July‑2024)
set "NDK_ZIP=android-ndk-%NDK_VER%-windows.zip"
set "NDK_URL=https://dl.google.com/android/repository/%NDK_ZIP%"

set "SDK_CLI_VER=11076708"              :: command‑line‑tools build id (matches Studio 2024.07)
set "SDK_ZIP=commandlinetools-win-%SDK_CLI_VER%_latest.zip"
set "SDK_URL=https://dl.google.com/android/repository/%SDK_ZIP%"

set "API_LATEST=33"                     :: compileSdkVersion (headers)
set "API_TARGET=32"                     :: targetSdkVersion (Meta Quest requirement)
set "API_MIN=29"                        :: minSdkVersion (Quest 1 support)

set "JDK_ZIP=openjdk17-win64.zip"
set "JDK_URL=https://github.com/adoptium/temurin17-binaries/releases/download/jdk-17.0.11+9/OpenJDK17U-jdk_x64_windows_hotspot_17.0.11_9.zip"
set "JDK_DIR=%TOOLS%\java\jdk-17.0.11+9"

:: ── Make folders & move to android/ ──────────────────────────────────────────
mkdir "%TOOLS%" 2>nul
pushd "%TOOLS%" >nul

:: ── 1. NDK ───────────────────────────────────────────────────────────────────
echo ========== Android NDK %NDK_VER%
if not exist "ndk" (
    if not exist "%NDK_ZIP%" (
        curl -# -L -o "%NDK_ZIP%" "%NDK_URL%"
    )
    echo Extracting NDK …
    powershell -NoLogo -NoProfile -Command "Expand-Archive -Path '%CD%\\%NDK_ZIP%' -DestinationPath '%CD%' -Force"
    move "android-ndk-%NDK_VER%" ndk >nul
)

:: ── 2. SDK command‑line tools ────────────────────────────────────────────────
echo ========== Android SDK command‑line tools
if not exist "sdk\cmdline-tools\latest\bin\sdkmanager.bat" (
    if not exist "%SDK_ZIP%" (
        curl -# -L -o "%SDK_ZIP%" "%SDK_URL%"
    )
    echo Extracting SDK tools …
    powershell -NoLogo -NoProfile -Command "Expand-Archive -Path '%CD%\\%SDK_ZIP%' -DestinationPath '%CD%\\sdk-tools-temp' -Force"
    rem Ensure destination hierarchy sdk\cmdline-tools\latest\*
    mkdir "sdk\cmdline-tools" 2>nul
    if exist "sdk\cmdline-tools\latest" rmdir /s /q "sdk\cmdline-tools\latest"
    move "sdk-tools-temp\cmdline-tools" "sdk\cmdline-tools\latest" >nul
    rmdir /s /q "sdk-tools-temp"
)

:: ── 3. Portable OpenJDK 17 ──────────────────────────────────────────────────
echo ========== OpenJDK 17 (portable)
if not exist "%JDK_DIR%\bin\java.exe" (
    if not exist "%JDK_ZIP%" (
        curl -# -L -o "%JDK_ZIP%" "%JDK_URL%"
    )
    echo Extracting JDK …
    powershell -NoLogo -NoProfile -Command "Expand-Archive -Path '%CD%\\%JDK_ZIP%' -DestinationPath '%CD%\\java' -Force"
)

:: ── 4. Install needed SDK packages ─────────────────────────────────────────--
set "JAVA_HOME=%JDK_DIR%"
set "PATH=%JAVA_HOME%\bin;%PATH%"

set "ANDROID_SDK_ROOT=%TOOLS%\sdk"
set "PATH=%ANDROID_SDK_ROOT%\cmdline-tools\latest\bin;%PATH%"

:: ── 4. Install essential SDK packages ──────────────────────────────────────
echo ========== Installing essential SDK packages
if not exist "%ANDROID_SDK_ROOT%\platforms\android-%API_LATEST%" (
    echo Installing platform-tools, build-tools, and platforms
    echo y | sdkmanager.bat "platform-tools" "platforms;android-%API_LATEST%" "build-tools;35.0.0"
)

:: ── 5. Install emulator and system images ─────────────────────────────────
echo ========== Installing emulator and system images
if not exist "%ANDROID_SDK_ROOT%\emulator\emulator.exe" (
    echo Installing emulator package
    REM echo y | 
    sdkmanager.bat "emulator"
)

if not exist "%ANDROID_SDK_ROOT%\system-images\android-33\google_apis\x86_64" (
    echo Installing Android 33 system image (this may take several minutes)
    echo y | sdkmanager.bat "system-images;android-33;google_apis;x86_64"
)

:: ── 6. Create questCI AVD ──────────────────────────────────────────────────
echo ========== Creating questCI AVD
set "AVD_DIR=%USERPROFILE%\.android\avd\questCI.avd"
if not exist "%AVD_DIR%" (
    echo Creating questCI AVD with Android 33, Google APIs, Pixel 5
    echo no | avdmanager.bat create avd -n questCI ^
        -k "system-images;android-33;google_apis;x86_64" ^
        --device "pixel_5"
    
    if errorlevel 1 (
        echo Warning: AVD creation failed, but you can create it manually later
        echo Command: avdmanager create avd -n questCI -k "system-images;android-33;google_apis;x86_64" --device "pixel_5"
    ) else (
        echo questCI AVD created successfully!
    )
) else (
    echo questCI AVD already exists
)

:: ── 7. Completion summary ─────────────────────────────────────────────────
popd >nul
echo.
echo ===============================================
echo Android Toolchain Setup Complete!
echo ===============================================
echo.
echo Installed components:
echo - Android NDK r27 LTS
echo - Android SDK command-line tools
echo - Platform-tools (adb, fastboot)
echo - OpenJDK 17 (portable)
echo - Emulator package
echo - Android 33 system image
echo - questCI AVD (Pixel 5, Android 13)
echo.
echo Ready to use:
echo 1. Build Android client: build_android_simple.bat
echo 2. Start emulator: start_questci_simple.bat
echo 3. Test client: test_questci_android.bat
echo.
echo All tools are in: %TOOLS%
echo.
