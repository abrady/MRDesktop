param(
    [string]$Config = "debug"
)

Write-Host "MRDesktop Integration Test Suite" -ForegroundColor Cyan
Write-Host "=================================" -ForegroundColor Cyan
Write-Host ""

Write-Host "Configuration: $Config" -ForegroundColor Yellow
Write-Host ""

# Define paths
if ($Config -eq "debug") {
    $ServerExe = "..\build\debug\Debug\MRDesktopServer.exe"
    $ClientExe = "..\build\debug\Debug\MRDesktopConsoleClient.exe"
} else {
    $ServerExe = "..\build\release\Release\MRDesktopServer.exe"
    $ClientExe = "..\build\release\Release\MRDesktopConsoleClient.exe"
}

# Check if binaries exist
if (-not (Test-Path $ServerExe)) {
    Write-Host "ERROR: Server executable not found at $ServerExe" -ForegroundColor Red
    Write-Host "Please build the project first using: build.bat $Config" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path $ClientExe)) {
    Write-Host "ERROR: Client executable not found at $ClientExe" -ForegroundColor Red
    Write-Host "Please build the project first using: build.bat $Config" -ForegroundColor Red
    exit 1
}

Write-Host "Found server: $ServerExe" -ForegroundColor Green
Write-Host "Found client: $ClientExe" -ForegroundColor Green
Write-Host ""

# Kill any existing processes to ensure clean test
Get-Process -Name "MRDesktopServer" -ErrorAction SilentlyContinue | Stop-Process -Force
Get-Process -Name "MRDesktopConsoleClient" -ErrorAction SilentlyContinue | Stop-Process -Force

Write-Host "Starting test server in background..." -ForegroundColor Yellow
$ServerProcess = Start-Process -FilePath $ServerExe -ArgumentList "--test" -PassThru -WindowStyle Hidden

# Wait a moment for server to start
Start-Sleep -Seconds 2

Write-Host "Starting test client..." -ForegroundColor Yellow
$ClientProcess = Start-Process -FilePath $ClientExe -ArgumentList "--test", "--compression=none" -Wait -PassThru -NoNewWindow

$ClientExitCode = $ClientProcess.ExitCode

# Clean up server process
if ($ServerProcess -and -not $ServerProcess.HasExited) {
    $ServerProcess | Stop-Process -Force
}

Write-Host ""
Write-Host "=================================" -ForegroundColor Cyan
if ($ClientExitCode -eq 0) {
    Write-Host "TEST RESULT: PASSED" -ForegroundColor Green
    Write-Host "All frames were transmitted and validated successfully!" -ForegroundColor Green
} else {
    Write-Host "TEST RESULT: FAILED" -ForegroundColor Red
    Write-Host "Frame transmission or validation failed (exit code: $ClientExitCode)" -ForegroundColor Red
}
Write-Host "=================================" -ForegroundColor Cyan

exit $ClientExitCode