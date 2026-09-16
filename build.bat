@echo off
REM Gridlock: configure + build (MSVC 2022 x64, Ninja) + run the fail-fast test suite.
setlocal
set VCVARS="C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
set CMAKE="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set NINJA_DIR=C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja
call %VCVARS% x64 >nul 2>&1 || (echo vcvarsall failed & exit /b 1)
set "PATH=%NINJA_DIR%;%PATH%"
set CFG=%1
if "%CFG%"=="" set CFG=Release
%CMAKE% -S "%~dp0." -B "%~dp0build" -G Ninja -DCMAKE_BUILD_TYPE=%CFG% || exit /b 1
%CMAKE% --build "%~dp0build" || exit /b 1
"%~dp0build\gridlock_tests.exe" || exit /b 1
echo.
echo Build OK. Play:  build\gridlock.exe
