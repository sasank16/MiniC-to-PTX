# MiniC-to-PTX Test Script (PowerShell)
$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = Split-Path -Parent $scriptDir
$buildDir = Join-Path $rootDir "build"

# Ensure cmake and ninja from user python scripts are on path
$userScripts = "C:\Users\nvsai\AppData\Roaming\Python\Python314\Scripts"
if (Test-Path $userScripts) {
    $env:PATH = "$userScripts;C:\msys64\usr\bin;$env:PATH"
}

if (-not (Test-Path $buildDir)) {
    Write-Host "Build directory not found. Running build.ps1 first..." -ForegroundColor Yellow
    & (Join-Path $scriptDir "build.ps1")
}

Write-Host "==> Running CTest suite..." -ForegroundColor Cyan
ctest --test-dir $buildDir --output-on-failure

Write-Host "==> All tests executed successfully!" -ForegroundColor Green
