@echo off
REM Launch Gridlock: Silicon Syndicate (UE5 client, standalone game window).
REM   run.bat [hegemony|ghost|hive] [seed]      e.g.  run.bat ghost nightfall
setlocal
set UE="D:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set ARCH=%1
if "%ARCH%"=="" set ARCH=hegemony
set SEED=%2
if "%SEED%"=="" set SEED=gridlock
if not exist "%~dp0ue5\Binaries\Win64\UnrealEditor-Gridlock.dll" (
  echo Game module not built. Run build.bat, then:
  echo   "D:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" GridlockEditor Win64 Development -Project="%~dp0ue5\Gridlock.uproject"
  exit /b 1
)
start "" %UE% "%~dp0ue5\Gridlock.uproject" -game -windowed -resx=1600 -resy=900 -ForceRes -arch=%ARCH% -seed=%SEED% %3 %4 %5
