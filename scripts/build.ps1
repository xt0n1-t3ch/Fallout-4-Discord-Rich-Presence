param(
    [Parameter(Mandatory=$true)] [ValidateSet('configure','build','test','all')] [string]$Action,
    [string]$Preset = 'flat-ng-release'
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot

function Import-VsDevCmd {
    $candidates = @(
        'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat',
        'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat'
    )
    $vsDevCmd = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
    if (-not $vsDevCmd) { throw 'No VsDevCmd.bat found in expected locations' }
    $cmd = "`"$vsDevCmd`" -arch=x64 -host_arch=x64 -no_logo && set"
    & cmd.exe /c $cmd | ForEach-Object {
        if ($_ -match '^(?<name>[^=]+)=(?<value>.*)$') {
            Set-Item -Path "Env:$($matches.name)" -Value $matches.value -ErrorAction SilentlyContinue
        }
    }
    Write-Output "VsDevCmd env loaded from $vsDevCmd"
}

function Ensure-VcpkgRoot {
    $preferred = 'D:\X\vcpkg'
    if (Test-Path "$preferred\vcpkg.exe") {
        $env:VCPKG_ROOT = $preferred
    } elseif (-not (Test-Path "$env:VCPKG_ROOT\vcpkg.exe")) {
        throw "no usable vcpkg.exe at $preferred or $env:VCPKG_ROOT"
    }
    Write-Output "VCPKG_ROOT = $env:VCPKG_ROOT"
}

function Add-VcpkgTools {
    $root = 'D:\X\vcpkg'
    if (-not (Test-Path "$root\downloads\tools")) {
        throw "vcpkg downloads/tools not found at $root"
    }
    $ninjaExe = Get-ChildItem "$root\downloads\tools" -Filter 'ninja.exe' -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1
    $cmakeExe = Get-ChildItem "$root\downloads\tools" -Filter 'cmake.exe' -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if (-not $ninjaExe) { throw "ninja.exe not found under $root\downloads\tools" }
    if (-not $cmakeExe) { throw "cmake.exe not found under $root\downloads\tools" }
    $env:Path = "$($cmakeExe.DirectoryName);$($ninjaExe.DirectoryName);" + $env:Path
    Write-Output "Added to PATH: $($ninjaExe.DirectoryName), $($cmakeExe.DirectoryName)"
}

Import-VsDevCmd
Ensure-VcpkgRoot
Add-VcpkgTools

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
