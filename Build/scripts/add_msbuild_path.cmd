::-------------------------------------------------------------------------------------------------------
:: Copyright (C) Microsoft. All rights reserved.
:: Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
::-------------------------------------------------------------------------------------------------------

:: add_msbuild_path.cmd
::
:: Locate msbuild.exe and add it to the PATH

where /q msbuild.exe
if "%ERRORLEVEL%" == "0" (
    exit /b 0
)

REM Try Dev14 first
set MSBUILD_VERSION=14.0
set MSBUILD_PATH="%ProgramFiles%\msbuild\%MSBUILD_VERSION%\Bin\x86"

if not exist %MSBUILD_PATH%\msbuild.exe (
    set MSBUILD_PATH="%ProgramFiles(x86)%\msbuild\%MSBUILD_VERSION%\Bin"
)

if not exist %MSBUILD_PATH%\msbuild.exe (
    set MSBUILD_PATH="%ProgramFiles(x86)%\msbuild\%MSBUILD_VERSION%\Bin\amd64"
)

if exist %MSBUILD_PATH%\msbuild.exe (
    goto :MSBuildFound
)

set MSBUILD_VERSION=12.0
set MSBUILD_PATH="%ProgramFiles%\msbuild\%MSBUILD_VERSION%\Bin\x86"
echo Dev14 not found, trying Dev %MSBUILD_VERSION%

if not exist %MSBUILD_PATH%\msbuild.exe (
    set MSBUILD_PATH="%ProgramFiles(x86)%\msbuild\%MSBUILD_VERSION%\Bin"
)

if not exist %MSBUILD_PATH%\msbuild.exe (
    set MSBUILD_PATH="%ProgramFiles(x86)%\msbuild\%MSBUILD_VERSION%\Bin\amd64"
)

if not exist %MSBUILD_PATH%\msbuild.exe (
    echo Can't find msbuild.exe in %MSBUILD_PATH%
    exit /b 1
)

:MSBuildFound
echo MSBuild located at %MSBUILD_PATH%

set PATH=%MSBUILD_PATH%;%PATH%
set MSBUILD_PATH=
exit /b 0
