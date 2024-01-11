@echo off
setlocal enableextensions enabledelayedexpansion

rem Name of the previous arguments file name
set "prevArgsFile=Build\Binaries\%~1_prev_args.txt"

rem Extract the command line args
FOR /F "tokens=* USEBACKQ" %%F IN (`Source\Build\parse_cmd_args "Build\%~1.vcxproj.user" %~2`) DO (
    SET args=%%F
)
rem echo %args%

if "%args%" == "" (
    for /F "delims=" %%x in ('%prevArgsFile%') do set "args=%%x"
) else (
    echo %args% > %prevArgsFile%
)
rem echo %args%

rem Run the result of the build
call "Build\Binaries\%~2\%~1.exe" %args%