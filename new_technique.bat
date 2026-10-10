@echo off
rem Creates a new technique folder from Techniques\_Template. Usage: new_technique.bat ^<Category^> ^<Name^>  (e.g. Shadows RayTracedShadows)
setlocal
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\NewTechnique.ps1" %*
exit /b %ERRORLEVEL%
