@echo off
setlocal
cd /d "%~dp0.."
set "UE_EDITOR=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set "UPROJECT=%CD%\WildOmission.uproject"

echo Launching Wild Omission (Fast Standalone Mode - 1280x720 Windowed)...
start "" /D "%CD%" "%UE_EDITOR%" "%UPROJECT%" -game -windowed -ResX=1280 -ResY=720 -NoVerifyGC -NoSplash -ExecCmds="r.DynamicGlobalIlluminationMethod 0, r.ReflectionMethod 2, r.Shadow.Virtual.Enable 0, sg.ShadowQuality 1, sg.GlobalIlluminationQuality 0, sg.ReflectionQuality 1, sg.PostProcessQuality 1, sg.EffectsQuality 1, sg.FoliageQuality 1, sg.ViewDistanceQuality 1, r.ScreenPercentage 85"
