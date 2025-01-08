@echo off
setlocal

:: Check if the program path argument is provided
if "%~1"=="" exit /b

:: Get the program path from the argument
set PROGRAM_PATH=%~1

:: Remove the shortcut from the Start Menu
powershell -NoProfile -Command "Remove-Item -Path ([System.IO.Path]::Combine($env:APPDATA, 'Microsoft\Windows\Start Menu\Programs\%~n1.lnk')) -Force"

exit /b
