::-------------------------------------------------------------------------------------------------------
:: Copyright (C) Microsoft. All rights reserved.
:: Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
::-------------------------------------------------------------------------------------------------------

:: add_msbuild_path.cmd
::
:: Locate msbuild.exe and add it to the PATH.
:: This script wraps the logic in locate_msbuild.ps1.
::
:: Note: we use the ugly pattern below because it seems to be the best (or only) way
:: to take the output of a command and assign it to an environment variable.
::     for /f "delims=" %%a in ('command') do @set VARIABLE=%%a

@echo off
set USE_MSBUILD_12=%1

if "%USE_MSBUILD_12%" == "True" (
    echo Trying Dev12 directly...
    for /f "delims=" %%a in ('powershell -Command ". %~dp0\locate_msbuild.ps1; Split-Path -Parent (Locate-MSBuild-Version -version 12.0)"') do @set MSBUILD_PATH=%%a
) else (
    for /f "delims=" %%a in ('powershell -Command "[bool](Get-Command msbuild.exe -ErrorAction SilentlyContinue)"') do @set MSBUILD_FOUND=%%a
    if "%MSBUILD_FOUND%" == "True" (
        REM msbuild.exe is already on the PATH
        goto :CleanUp
    )
    for /f "delims=" %%a in ('powershell -Command ". %~dp0\locate_msbuild.ps1; Split-Path -Parent (Locate-MSBuild)"') do @set MSBUILD_PATH=%%a
)

REM Test whether msbuild.exe was found.
if "%MSBUILD_PATH%" == "" (
    REM msbuild.exe was not found
    echo Can't locate msbuild.exe
    goto :CleanUp
) else (
    goto :MSBuildFound
)

:MSBuildFound
echo MSBuild located at %MSBUILD_PATH%
set PATH=%MSBUILD_PATH%;%PATH%

:CleanUp
set USE_MSBUILD_12=
set MSBUILD_PATH=
set MSBUILD_FOUND=
