$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$qtBin = "C:\Qt\6.11.1\mingw_64\bin"
$exePath = Join-Path $projectRoot "build\ChatServer.exe"

if (-not (Test-Path $exePath)) {
    Write-Host "ChatServer.exe was not found. Build the project first."
    exit 1
}

$env:PATH = "$qtBin;$env:PATH"
& $exePath
