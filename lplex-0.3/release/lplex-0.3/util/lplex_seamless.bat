@echo off
title lplex_seamless.bat
if exist "%~1" goto run

echo.
echo    Usage : drop an audio folder onto 'lplex_seamless.bat' to author
echo            a concert dvd with continuous audio tracks.
echo.
pause
goto done

:run
%~d0
cd "%~dp0"
cd ..
cls

lplex.exe --alignment seamless --shift backward "%~1"

:done
