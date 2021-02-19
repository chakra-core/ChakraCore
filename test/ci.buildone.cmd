::-------------------------------------------------------------------------------------------------------
:: Copyright (C) Microsoft. All rights reserved.
:: Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
:: Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
::-------------------------------------------------------------------------------------------------------

@echo off
setlocal

if "%_ENTRY_SCRIPT_NAME%"=="" (
    set _ENTRY_SCRIPT_NAME=%0
)

REM check that we have enough parameters
if "%1"=="" (
    goto :usage
)
if "%2"=="" (
    goto :usage
)

pushd %~dp0

call ci.build.init.cmd %*

set _BuildArch=
set _BuildType=

call ci.build.cmd %JENKINS_BUILD_ARGS%

popd
goto :end

:: ============================================================================
:: Not enough params
:: ============================================================================
:usage

    echo Not enough parameters. Please specify architecture and type.
    echo Examples:
    echo.
    echo     %_ENTRY_SCRIPT_NAME% x86 debug
    echo     %_ENTRY_SCRIPT_NAME% x86 test
    echo     %_ENTRY_SCRIPT_NAME% x86 release
    echo.
    echo     %_ENTRY_SCRIPT_NAME% x64 debug
    echo     %_ENTRY_SCRIPT_NAME% x64 test
    echo     %_ENTRY_SCRIPT_NAME% x64 release

:end
endlocal
