@echo off
title lplex_discrete.bat
if exist "%~1" goto run

echo.
echo    Usage  : drop an audio folder onto 'lplex_discrete.bat' to author
echo             a compilation dvd with gaps between audio tracks.
echo.
pause
goto done

:run
%~d0
cd "%~dp0"
cd ..
cls

lplex.exe --alignment discrete "%~1"

:done
