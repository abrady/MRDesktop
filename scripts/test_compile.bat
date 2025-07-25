@echo off
echo Testing if the code compiles without errors...
echo.

REM Just try to configure and build to check for compilation errors
if not exist "..\build\debug" (
    echo Running configure...
    call ..\configure.bat debug
    if %ERRORLEVEL% neq 0 (
        echo Configuration failed!
        exit /b %ERRORLEVEL%
    )
)

echo Building...
call ..\build.bat debug
if %ERRORLEVEL% neq 0 (
    echo Build failed!
    exit /b %ERRORLEVEL%
) else (
    echo Build successful! Test infrastructure is ready.
    echo.
    echo You can now run the test with:
    echo   run_test.bat [debug^|release]
    echo or
    echo   powershell .\run_test.ps1 [-Config debug^|release]
)