@echo off
setlocal
echo Installing Microsoft Visual C++ Redistributable 14.50.35719.0 for Unreal Engine 5.8...
set "REDIST_EXE=C:\Program Files\Epic Games\UE_5.8\Engine\Extras\Redist\en-us\vc_redist.x64.exe"
if exist "%REDIST_EXE%" (
    "%REDIST_EXE%" /install /passive /norestart
    echo Installation completed (or already up to date).
) else (
    echo Could not find "%REDIST_EXE%".
)
pause
