::-------------------------------------------------------------------------------------------------------
:: Copyright (C) Microsoft. All rights reserved.
:: Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
::-------------------------------------------------------------------------------------------------------

:: PGO Build Workflow:
:: * pre_pgi.cmd
:: - build (with PGI instrumentation enabled)
:: - post_pgi.cmd
:: - pogo_training.ps1
:: - pre_pgo.cmd
:: - build (using PGO profile)
:: - post_pgo.cmd

@echo off

if "%PogoConfig%"=="False" (
    echo ---- Not a Pogo Config. Skipping step.
    exit /b 0
)

set arch_pgi=%1
set flavor_pgi=%2
set binpath_pgi=%3
set builderror=

if "%arch_pgi%"=="" (
  goto:usage
)
if "%flavor_pgi%"=="" (
  goto:usage
)
if "%binpath_pgi%"=="" (
  goto:usage
)

if not exist %binpath_pgi% (
  md %binpath_pgi%
) else (
  if exist %binpath_pgi%\*.pgc ( del %binpath_pgi%\*.pgc )
)

REM Build with pgo instrumentation
set POGO_TYPE=PGI

REM Temporary fix around pgo bug, todo:: check if still necessary once toolset is updated
set _LINK_=/cgthreads:1

goto:checkpass

:usage
  echo Invalid/missing arguments
  echo.
  echo Usage: pre_pgi.cmd ^<arch^> ^<flavor^> ^<binary_path^>
  echo   - arch  : Architecture you want to build pogo
  echo   - flavor: flavor you want to build pogo
  echo   - binary_path: output path of your binaries

exit /b 1

:checkpass
exit /b 0
