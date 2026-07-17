# Configures and builds JSON Studio with the MSYS2 mingw64 toolchain.
#   .\build.ps1            -> build
#   .\build.ps1 -Run       -> build and launch
#   .\build.ps1 -Fresh     -> wipe build/ and reconfigure
param(
    [switch]$Run,
    [switch]$Fresh,
    [string]$Config = "Debug"
)

$ErrorActionPreference = "Stop"

$mingw = "C:\msys64\mingw64"
if (-not (Test-Path "$mingw\bin\g++.exe")) {
    throw "mingw64 toolchain niet gevonden in $mingw - draai de pacman-install eerst."
}

# mingw64\bin must lead PATH so cmake/ninja/g++ and the Qt DLLs all resolve here.
$env:PATH = "$mingw\bin;" + $env:PATH

$root = $PSScriptRoot
$build = Join-Path $root "build"

if ($Fresh -and (Test-Path $build)) {
    Remove-Item -Recurse -Force $build
}

& "$mingw\bin\cmake.exe" -S $root -B $build -G Ninja -DCMAKE_BUILD_TYPE=$Config
if ($LASTEXITCODE -ne 0) { throw "configure faalde" }

& "$mingw\bin\cmake.exe" --build $build
if ($LASTEXITCODE -ne 0) { throw "build faalde" }

Write-Host "OK -> $build\JsonStudio.exe" -ForegroundColor Green

if ($Run) {
    & "$build\JsonStudio.exe" (Join-Path $root "samples\sample.json")
}
