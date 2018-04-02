@echo off
title lplex_padded.bat
if exist "%~1" goto run

echo.
echo    Usage : drop an audio folder onto 'lplex_padded.bat' to author
echo            a compilation dvd with padding between audio tracks.
echo.
pause
goto done

:run
%~d0
cd "%~dp0"
cd ..
cls

Lplex.exe --alignment padded "%~1"
