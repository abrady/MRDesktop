$ErrorActionPreference = 'Stop'

$OutputDir = Join-Path $PSScriptRoot '..\out'
if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir | Out-Null
}

$serverExe = Join-Path $PSScriptRoot '..\build\MRDesktopServer.exe'
if (-not (Test-Path $serverExe)) {
    Write-Error "MRDesktopServer.exe not found at $serverExe"
    exit 1
}

# Start server in headless record mode
$process = Start-Process -FilePath $serverExe `
    -ArgumentList '--test','--record',`"$OutputDir\clip.h265`",`"$OutputDir\clip.mp4`" `
    -WindowStyle Hidden -PassThru

Start-Sleep -Seconds 3

try {
    if (-not $process.HasExited) { $process | Stop-Process -Force }
}
catch {}

if ($process.ExitCode -ne 0) {
    Write-Error "MRDesktopServer exited with code $($process.ExitCode)"
    exit $process.ExitCode
}

# Extract first frame as separate elementary stream
$ffmpeg = 'ffmpeg'
if (Get-Command $ffmpeg -ErrorAction SilentlyContinue) {
    & $ffmpeg -y -i "$OutputDir/clip.mp4" -vframes 1 -c:v copy "$OutputDir/frame.h265" | Out-Null
}

Write-Host "Generated sample at $OutputDir" -ForegroundColor Green
