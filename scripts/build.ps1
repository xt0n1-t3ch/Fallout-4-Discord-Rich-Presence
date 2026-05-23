param(
    [Parameter(Mandatory=$true)] [ValidateSet('configure','build','test','all')] [string]$Action,
    [string]$Preset = 'flat-ng-release'
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot

function Import-VsDevCmd {
    $vsw = 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path $vsw)) { throw 'vswhere missing' }
    $vsRoot = & $vsw -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vsRoot) { throw 'No VS installation with C++ workload found' }
    $vsDevCmd = Join-Path $vsRoot 'Common7\Tools\VsDevCmd.bat'
    if (-not (Test-Path $vsDevCmd)) { throw "VsDevCmd missing at $vsDevCmd" }
    $cmd = "`"$vsDevCmd`" -arch=x64 -host_arch=x64 -no_logo && set"
    & cmd.exe /c $cmd | ForEach-Object {
        if ($_ -match '^(?<name>[^=]+)=(?<value>.*)$') {
            Set-Item -Path "Env:$($matches.name)" -Value $matches.value -ErrorAction SilentlyContinue
        }
    }
    Write-Output "VsDevCmd env loaded from $vsRoot"
}

function Ensure-VcpkgRoot {
    if (-not $env:VCPKG_ROOT) {
        $env:VCPKG_ROOT = 'D:\X\vcpkg'
    }
    if (-not (Test-Path "$env:VCPKG_ROOT\vcpkg.exe")) {
        throw "VCPKG_ROOT invalid: $env:VCPKG_ROOT (no vcpkg.exe)"
    }
    Write-Output "VCPKG_ROOT = $env:VCPKG_ROOT"
}

Import-VsDevCmd
Ensure-VcpkgRoot

Set-Location $repo
Write-Output "PWD = $((Get-Location).Path)"

switch ($Action) {
    'configure' { cmake --preset $Preset }
    'build'     { cmake --build --preset $Preset --parallel }
    'test'      {
        cmake --preset flat-ng-debug
        cmake --build --preset flat-ng-debug --parallel
        ctest --preset flat-ng-debug --output-on-failure --no-tests=error
    }
    'all' {
        cmake --preset $Preset
        cmake --build --preset $Preset --parallel
    }
}
