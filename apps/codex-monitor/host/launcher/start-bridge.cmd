@echo off
setlocal
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoLogo -NoProfile -NonInteractive -WindowStyle Hidden -ExecutionPolicy Bypass -File "%~dp0run-bridge.ps1" > "%~dp0logs\launcher.log" 2>&1
exit /b %ERRORLEVEL%
