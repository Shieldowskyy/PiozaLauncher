@echo off
setlocal

:: Check if the program path argument is provided
if "%~1"=="" exit /b

:: Get the program path from the argument
set PROGRAM_PATH=%~1

:: Create the shortcut in the Start Menu without any output or windows
powershell -Command "$s=(New-Object -COM WScript.Shell).CreateShortcut([System.IO.Path]::Combine($env:APPDATA, 'Microsoft\Windows\Start Menu\Programs\%~n1.lnk'));$s.TargetPath='%PROGRAM_PATH%';$s.Save()"

exit /b
