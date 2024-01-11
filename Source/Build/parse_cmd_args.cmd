@echo off

setlocal enableextensions enabledelayedexpansion
set "xmlFile=%~1"
set "configToken=%~2"

rem Look for the config
for /f "tokens=1,2 delims=:" %%n in ('findstr /n /i /c:"%configToken% Windows64|x64" "%xmlFile%"') do (
    rem Look for the command line args
    for /f "tokens=*" %%l in ('type "%xmlFile%" ^| more +%%n') do (
        set "line=%%l"

        rem Check if the line matches the command line arg files
        if not "!line!"=="!line:LocalDebuggerCommandArguments=!" (
            set "args=!line:<LocalDebuggerCommandArguments>=!"
            set "args=!args:</LocalDebuggerCommandArguments>=!"
            echo %args%
            goto endLoop
        )
    )
)
:endLoop
    echo %args%