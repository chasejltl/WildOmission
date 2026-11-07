@echo off
setlocal
REM Opens the Wild Omission project in Unreal Engine 5.8.4 Editor
cd /d "%~dp0.."
set "UE_EDITOR=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set "UPROJECT=%CD%\WildOmission.uproject"

start "" /D "%CD%" "%UE_EDITOR%" "%UPROJECT%"
