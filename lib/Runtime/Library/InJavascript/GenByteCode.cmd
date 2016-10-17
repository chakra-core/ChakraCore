::-------------------------------------------------------------------------------------------------------
:: Copyright (C) Microsoft. All rights reserved.
:: Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
::-------------------------------------------------------------------------------------------------------

@echo off
setlocal
set _HASERROR=0
set _binary=ch.exe
set _BuildDir=%~dp0..\..\..\..\Build

if "%_FILE%" == "" (
    set "_FILE=%~dp0*.js"
)

:ContinueArgParse
if not [%1]==[] (
    if [%1]==[-nojit] (
        set _suffix=.nojit
        goto :ContinueArgParseEnd
    )
    if [%1]==[-binary] (
        if [%2]==[] (
            echo Error: no argument supplied to -binary. Exiting ...
            exit /b 1
        )
        set _binary=%2
        shift
        goto :ContinueArgParseEnd
    )
    if [%1]==[-bindir] (
        if [%2]==[] (
            echo Error: no argument supplied to -bindir. Exiting ...
            exit /b 1
        )
        set _BinLocation=%2
        shift
        goto :ContinueArgParseEnd
    )

    :ContinueArgParseEnd
    shift
    goto :ContinueArgParse
)

:: This script will expect %_binary% to be built for x86_debug and x64_debug

if "%OutBaseDir%" NEQ "" (
    set _BinLocation=%OutBaseDir%\Chakra.Core%_suffix%\bin
)
if "%_BinLocation%"=="" (
    set _BinLocation=%_BuildDir%\VcBuild%_suffix%\bin
)

if not exist %_BinLocation%\x86_debug\%_binary% (
    echo Error: %_BinLocation%\x86_debug\%_binary% not found, please build sources. Exiting ...
    exit /b 1
)

if not exist %_BinLocation%\x64_debug\%_binary% (
    echo Error: %_BinLocation%\x64_debug\%_binary% not found, please build sources. Exiting ...
    exit /b 1
)

for %%i in (%_FILE%) do (
    call :GenerateLibraryByteCodeHeader %%i
)
exit /B %_HASERROR%

:GenerateLibraryBytecodeHeader

echo Generating %1%_suffix%.bc.32b.h
call :Generate %1 %_BinLocation%\x86_debug %1%_suffix%.bc.32b.h
echo Generating %1%_suffix%.bc.64b.h
call :Generate %1 %_BinLocation%\x64_debug %1%_suffix%.bc.64b.h
exit /B 0

:Generate
%2\%_binary% -GenerateLibraryByteCodeHeader:%3 -Intl %1
if "%errorlevel%" NEQ "0" (
    echo %1: Error generating bytecode file. Ensure %3 writable.
    set _HASERROR=1
) else (
    echo Bytecode generated. Please rebuild to incorporate the new bytecode.
)
