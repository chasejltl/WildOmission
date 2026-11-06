@echo off
REM Runs all WildOmission Phase 1-4 automation tests headlessly in Unreal Engine 5.8
set UE_CMD="C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
set UPROJECT="%~dp0..\WildOmission.uproject"

%UE_CMD% %UPROJECT% -NullRHI -NoSound -Unattended -NoSplash -NOPAUSE -stdout -FullStdOutLogOutput -ExecCmds="Automation RunTests WildOmission.Phase1+WildOmission.Phase2+WildOmission.Phase3+WildOmission.Phase4; Quit" -TestExit="Automation Test Queue Empty"
exit /b %ERRORLEVEL%
