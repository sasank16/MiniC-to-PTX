# MiniC-to-PTX Build Script (PowerShell)
$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = Split-Path -Parent $scriptDir
$buildDir = Join-Path $rootDir "build"

# Ensure cmake and ninja from user python scripts are on path
$userScripts = "C:\Users\nvsai\AppData\Roaming\Python\Python314\Scripts"
if (Test-Path $userScripts) {
    $env:PATH = "$userScripts;C:\msys64\usr\bin;$env:PATH"
}

Write-Host "==> Configuring MiniC-to-PTX with CMake and Ninja..." -ForegroundColor Cyan
cmake -S $rootDir -B $buildDir -G Ninja -DCMAKE_CXX_COMPILER="C:/msys64/usr/bin/g++.exe" -DCMAKE_BUILD_TYPE=Debug

Write-Host "==> Building project targets..." -ForegroundColor Cyan
cmake --build $buildDir

Write-Host "==> Build completed successfully!" -ForegroundColor Green
