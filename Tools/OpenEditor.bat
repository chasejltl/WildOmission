@echo off
REM Opens the Wild Omission project in Unreal Engine 5.8.4 Editor
set UE_EDITOR="C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set UPROJECT="%~dp0..\WildOmission.uproject"

start "" %UE_EDITOR% %UPROJECT%
