::-------------------------------------------------------------------------------------------------------
:: Copyright (C) Microsoft. All rights reserved.
:: Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
::-------------------------------------------------------------------------------------------------------

:: PGO Build Workflow:
:: - pre_pgi.cmd
:: - build (with PGI instrumentation enabled)
:: - post_pgi.cmd
:: - pogo_training.ps1
:: * pre_pgo.cmd
:: - build (using PGO profile)
:: - post_pgo.cmd

@echo off

if "%PogoConfig%"=="False" (
    echo ---- Not a Pogo Config. Skipping step.
    exit /b 0
)

REM Optimize build with PGO data
set POGO_TYPE=PGO

goto:eof
