@echo off
REM Launches Wild Omission directly in Standalone Game mode (no Editor UI required)
set UE_EDITOR="C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set UPROJECT="%~dp0..\WildOmission.uproject"

start "" %UE_EDITOR% %UPROJECT% -game -ResX=1920 -ResY=1080 -windowed -log
