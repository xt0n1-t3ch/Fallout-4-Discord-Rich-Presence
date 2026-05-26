@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" -vcvars_ver=14.50 >nul 2>nul
if errorlevel 1 ( echo VCVARS_FAIL & exit /b 1 )
set "VCPKG_ROOT=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\vcpkg"
set "PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"
cmake --preset flat-ng-release -DF4DRP_BUILD_PLUGIN=ON 2>&1
if errorlevel 1 exit /b %errorlevel%
call tools\patch-commonlibf4.bat
if errorlevel 1 exit /b %errorlevel%
cmake --build --preset flat-ng-release --target f4discordrp_ng --parallel 2>&1
exit /b %errorlevel%
