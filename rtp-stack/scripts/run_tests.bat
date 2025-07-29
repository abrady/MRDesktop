@echo off
echo Running RTP Stack Tests...

REM Save the current directory
set ORIGINAL_DIR=%CD%

REM Check if we want Debug or Release build (default to Debug)
set BUILD_TYPE=Debug
if "%1"=="release" set BUILD_TYPE=Release
if "%1"=="Release" set BUILD_TYPE=Release

echo Build type: %BUILD_TYPE%

REM Set build directory - use parent project's build where RTP tests are built
if "%BUILD_TYPE%"=="Debug" (
    set BUILD_DIR=..\build\debug\rtp-stack\tests\Debug
) else (
    set BUILD_DIR=..\build\release\rtp-stack\tests\Release
)

echo Build directory: %BUILD_DIR%

REM Check if build directory exists, if not suggest running parent configure/build
if not exist "%BUILD_DIR%" (
    echo Build directory %BUILD_DIR% does not exist!
    echo Please run configure.bat and build.bat from the parent MRDesktop directory first:
    echo   cd ..
    echo   configure.bat
    echo   build.bat
    cd /d "%ORIGINAL_DIR%"
    pause
    exit /b 1
)

REM Change to build directory to run tests
cd /d "%BUILD_DIR%"

REM Run RTP tests directly
echo.
echo Running RTP tests...
rtp_tests.exe

REM Save test result before changing directory
set TEST_RESULT=%errorlevel%

REM Return to original directory
cd /d "%ORIGINAL_DIR%"

if %TEST_RESULT% neq 0 (
    echo.
    echo Tests failed!
    pause
    exit /b 1
) else (
    echo.
    echo All tests passed!
)

pause