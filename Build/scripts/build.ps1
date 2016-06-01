#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

# Build Script
#
# Use this script to run the appropriate flavor of MSBuild command for the $Env:BuildSubtype

param (
    [ValidateSet("x86", "x64", "arm")]
    [Parameter(Mandatory=$True)]
    [string]$arch,

    # We do not use ValidateSet here because this $flavor param is used to name the BuildConfuration
    # from the solution file. MsBuild will determine whether it is valid.
    [Parameter(Mandatory=$True)]
    [string]$flavor,

    [ValidateSet("default", "codecoverage", "pogo")]
    [string]$subtype = "default",

    [Parameter(Mandatory=$True)]
    [string]$solutionFile = "",

    [switch]$clean,

    [string]$binDir = "",
    [string]$buildlogsDir = "buildlogs",

    # assume NuGet is on the path, otherwise the caller must specify an explicit path
    [string]$nugetExe = "NuGet.exe",

    [string]$logFile = "",

    [string]$pogocmd = ""
)

$OuterScriptRoot = $PSScriptRoot
. $PSScriptRoot\pre_post_util.ps1

if (($logFile -eq "") -and (Test-Path Env:\TF_BUILD_BINARIESDIRECTORY)) {
    $logFile = "${Env:TF_BUILD_BINARIESDIRECTORY}\logs\build_${Env:BuildName}.log"
    if (Test-Path -Path $logFile) {
        Remove-Item $logFile -Force
    }
}

#
# NuGet restore
#

ExecuteCommand "& $nugetExe restore $solutionFile -NonInteractive"

#
# Setup and build
#

$msbuildExe = Locate-MSBuild
if (-not $msbuildExe) {
    Write-Error "Could not find msbuild.exe -- exiting..."
    exit 1
}

$binDir = UseValueOrDefault "$binDir" "${Env:BinariesDirectory}" "${Env:BUILD_SOURCESDIRECTORY}\Build\VcBuild"
$buildlogsPath = Join-Path $binDir $buildlogsDir

$defaultParams = "$solutionFile /nologo /m /nr:false /p:platform=`"${arch}`" /p:configuration=`"${flavor}`""
$loggingParams = @(
    "/fl1 `"/flp1:logfile=${buildlogsPath}\build.${Env:BuildName}.log;verbosity=normal`"",
    "/fl2 `"/flp2:logfile=${buildlogsPath}\build.${Env:BuildName}.err;errorsonly`"",
    "/fl3 `"/flp3:logfile=${buildlogsPath}\build.${Env:BuildName}.wrn;warningsonly`"",
    "/verbosity:normal"
    ) -join " "

$cleanTarget = "/t:`"Clean`""

$subtypeParams = ""
if (($subtype -eq "pogo") -and $pogocmd) {
    # TODO
} else {
    if ($subtype -eq "codecoverage") {
        $subtypeParams = "/p:ENABLE_CODECOVERAGE=true"
    }

    if ($clean) {
        $cleanCommand = ". `"$msbuildExe`" $cleanTarget $defaultParams $loggingParams $subtypeParams"
        ExecuteCommand $cleanCommand
    }

    $buildCommand = ". `"$msbuildExe`" $defaultParams $loggingParams $subtypeParams"
    ExecuteCommand $buildCommand
}
