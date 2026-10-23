@echo off
systeminfo>~info.tmp
if exist sysinfo.txt del sysinfo.txt

for %%a in ( "OS Name" "OS Version" "Type" "Family") do (
	for /f "tokens=1* delims=:" %%x in (
		'type ~info.tmp ^| find "%%~a" ^| find /v "BIOS"'
	) do call :trim %%y
)
del ~info.tmp
::pause
start Notepad "sysinfo.txt"
exit /b

:trim
echo %* >> sysinfo.txt
exit /b
