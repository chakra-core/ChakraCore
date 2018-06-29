::-------------------------------------------------------------------------------------------------------
:: Copyright (C) Microsoft. All rights reserved.
:: Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
::-------------------------------------------------------------------------------------------------------

@echo off

if not "%JENKINS_BUILD%" == "True" (
    echo This script should be run under a Jenkins Build environment
    exit /b 2
)

set REPO_ROOT=%~dp0\..

set JENKINS_BUILD_ARGS=
set JENKINS_FORCE_MSBUILD_VERSION=
:ContinueArgParse
if not [%1]==[] (
    if [%1]==[msbuild14] (
        set JENKINS_FORCE_MSBUILD_VERSION=%1
        goto :ContinueArgParseEnd
    )

    :: DEFAULT - add any other params to %JENKINS_BUILD_ARGS%
    if [%1] NEQ [] (
        set JENKINS_BUILD_ARGS=%JENKINS_BUILD_ARGS% %1
        goto :ContinueArgParseEnd
    )

    :ContinueArgParseEnd
    shift
    goto :ContinueArgParse
)

:: ========================================
:: Set up msbuild.exe
:: ========================================

call %REPO_ROOT%\Build\scripts\add_msbuild_path.cmd %JENKINS_FORCE_MSBUILD_VERSION%
