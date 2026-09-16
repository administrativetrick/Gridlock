@echo off
REM Package Gridlock: Silicon Syndicate as a standalone Windows build (no editor needed to play).
REM   package.bat            -> dist\Windows\Gridlock.exe  (Development: logs + dev flags available)
REM   package.bat shipping   -> dist\Windows\Gridlock.exe  (Shipping)
REM Requires: build.bat run once (build\gridlock_sim.lib), UE 5.8 installed, generated assets in ue5\Content.
setlocal
set UE=D:\Program Files\Epic Games\UE_5.8
set CFG=Development
if /I "%1"=="shipping" set CFG=Shipping
set ROOT=%~dp0
if not exist "%ROOT%build\gridlock_sim.lib" (
  echo gridlock_sim.lib missing - run build.bat first.
  exit /b 1
)
if not exist "%ROOT%ue5\Content\Materials\M_Neon.uasset" (
  echo Generated assets missing - run:
  echo   "%UE%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%ROOT%ue5\Gridlock.uproject" -run=GLAssets
  exit /b 1
)
call "%UE%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%ROOT%ue5\Gridlock.uproject" -noP4 -utf8output ^
  -platform=Win64 -clientconfig=%CFG% -build -cook -stage -pak -prereqs -nodebuginfo ^
  -archive -archivedirectory="%ROOT%dist"
if errorlevel 1 (
  echo PACKAGING FAILED
  exit /b 1
)
echo.
echo Packaged: %ROOT%dist\Windows\Gridlock.exe
