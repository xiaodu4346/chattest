$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$qtBin = "C:\Qt\6.11.1\mingw_64\bin"
$exePath = Join-Path $projectRoot "build\ChatTest.exe"

if (-not (Test-Path $exePath)) {
    Write-Host "ChatTest.exe was not found. Build the project first."
    exit 1
}

$env:PATH = "$qtBin;$env:PATH"
Start-Process -FilePath $exePath -WorkingDirectory (Split-Path -Parent $exePath)
