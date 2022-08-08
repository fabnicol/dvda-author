@echo off
title mkisofs.bat
if exist "%~1" goto run

echo.
echo    Usage : drop a DVD folder ( i.e. containing a 'VIDEO_TS' subfolder )
echo            onto 'mkisofs.bat' to create an iso image for burning.
echo.
goto done

:run
%~d0
cd "%~dp0"
cd ..\bin
cls

set vlabel=%~nx1

:: ensuring volume label < 32 bytes to prevent mkisofs failure
mkisofs.exe -dvd-video -V "%vlabel%:~0,31" -udf -o "%~dp1%~nx1.iso" "%~1"

:done
pause
