@echo off
rem GHOST LIGHT one-click entry point: builds and runs the engine. Usage: run.bat [release ^| test ^| clean] [engine options, e.g. --scene Scenes/CornellBox.scene.json]
setlocal
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\Run.ps1" %*
set "GHOST_EXIT=%ERRORLEVEL%"
if "%GHOST_EXIT%"=="0" goto :done
if /i "%~1"=="test" goto :done

rem Double-clicked windows close immediately; keep this one open so the error stays readable.
setlocal EnableDelayedExpansion
set "GHOST_CMD=!CMDCMDLINE!"
if /i not "!GHOST_CMD:/c =!"=="!GHOST_CMD!" pause
endlocal

:done
exit /b %GHOST_EXIT%
