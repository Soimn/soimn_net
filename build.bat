@echo off

setlocal

cd %~dp0

if not exist build mkdir build
cd build

cl /nologo /W4 /wd4201 ..\src\main.c /link /out:genweb.exe /pdb:genweb.pdb

if %errorlevel%==0 (
	echo Generating Website
	cd ..
	.\build\genweb.exe
)

endlocal
