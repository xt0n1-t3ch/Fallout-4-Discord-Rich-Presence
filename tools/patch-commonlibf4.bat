@echo off
rem CommonLibF4 fork bug: GetNormalized returns reference to local. VS 2026 /permissive- catches it.
rem One-line fix: change return type from hkVector4f& to hkVector4f (RVO friendly).
set HKVEC=build\flat-ng-release\_deps\commonlibf4-src\CommonLibF4\include\RE\Havok\hkVector4.h
if not exist "%HKVEC%" ( echo HKVECTOR4_NOT_FETCHED & exit /b 1 )
powershell -NoProfile -Command "(Get-Content '%HKVEC%' -Raw) -replace 'hkVector4f& GetNormalized', 'hkVector4f GetNormalized' | Set-Content '%HKVEC%' -NoNewline"
echo PATCHED %HKVEC%
exit /b 0
