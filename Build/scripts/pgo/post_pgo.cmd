::-------------------------------------------------------------------------------------------------------
:: Copyright (C) Microsoft. All rights reserved.
:: Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
::-------------------------------------------------------------------------------------------------------

:: PGO Build Workflow:
:: - pre_pgi.cmd
:: - build (with PGI instrumentation enabled)
:: - post_pgi.cmd
:: - pogo_training.ps1
:: - pre_pgo.cmd
:: - build (using PGO profile)
:: * post_pgo.cmd

@echo off

if "%PogoConfig%"=="False" (
    echo ---- Not a Pogo Config. Skipping step.
    exit /b 0
)

set binpath_pgo=%1

if "%binpath_pgo%"=="" (
  goto:usage
)

set POGO_TYPE=

REM Clean binaries we no longer need
if exist %binpath_pgo%\*.lib ( del %binpath_pgo%\*.lib )
if exist %binpath_pgo%\*.pgc ( del %binpath_pgo%\*.pgc )
if exist %binpath_pgo%\*.pgd ( del %binpath_pgo%\*.pgd )
if exist %binpath_pgo%\pgort* ( del %binpath_pgo%\pgort* )

goto:eof

:usage
  echo Invalid/missing arguments
  echo.
  echo Usage: post_pgo.cmd ^<binary_path^>
  echo   - binary_path: output path of your binaries

exit /b 1
