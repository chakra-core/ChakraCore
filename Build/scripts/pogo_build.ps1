#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

# Use this script to run a POGO build for the given BuildType (arch, flavor, subtype)

param (
    [ValidateSet("x86", "x64", "arm")]
    [Parameter(Mandatory=$True)]
    [string]$arch,

    # We do not use ValidateSet here because this $flavor param is used to name the BuildConfuration
    # from the solution file. MsBuild will determine whether it is valid.
    [Parameter(Mandatory=$True)]
    [string]$flavor,

    [ValidateSet("default", "codecoverage", "pogo")]
    [string]$subtype = "pogo",

    [Parameter(Mandatory=$True)]
    [string]$solutionFile,

    [switch]$clean,

    # $binDir will be inferred if not provided.
    [string]$binDir = "",
    [string]$buildlogsSubdir = "buildlogs",

    # Assume NuGet is on the path, otherwise the caller must specify an explicit path.
    [string]$nugetExe = "NuGet.exe",

    [string]$logFile = "",

    #
    # POGO training parameters
    #

    [string[]]$scenarios = @(),

    [Parameter(Mandatory=$True)]
    [string]$binpath,

    [string]$binaryName = "ch.exe"
)

#
# Configure logging
#

$OuterScriptRoot = $PSScriptRoot
. $PSScriptRoot\pre_post_util.ps1

$buildName = ConstructBuildName -arch $arch -flavor $flavor -subtype $subtype

if (($logFile -eq "") -and (Test-Path Env:\TF_BUILD_BINARIESDIRECTORY)) {
    $logFile = "${Env:TF_BUILD_BINARIESDIRECTORY}\logs\pogo_build.${buildName}.log"
}

if (($logFile -ne "") -and (Test-Path $logFile)) {
    Remove-Item $logFile -Force
}

#
# Only continue with this build if it is a valid pogo build configuration
#

if ($subtype -ne "pogo") {
    WriteMessage "This build's subtype is not pogo (subtype: $subtype). Skipping build."
    exit 0
}

if ($scenarios.Length -eq 0) {
    WriteMessage "No training scenarios selected. Please specify training scenarios using the -scenarios parameter."
    exit 0
}

#
# NuGet restore
#

ExecuteCommand "& $nugetExe restore $solutionFile -NonInteractive"

#
# Setup
#

$msbuildExe = Locate-MSBuild
if (-not $msbuildExe) {
    WriteErrorMessage "Error: Could not find msbuild.exe -- exiting (1)..."
    exit 1
}

$binDir = UseValueOrDefault "$binDir" "${Env:BinariesDirectory}" "${Env:BUILD_SOURCESDIRECTORY}\Build\VcBuild"
$buildlogsPath = Join-Path $binDir $buildlogsSubdir

$defaultParams = "$solutionFile /nologo /m /nr:false /p:platform=`"${arch}`" /p:configuration=`"${flavor}`""
$loggingParams = @(
    "/fl1 `"/flp1:logfile=${buildlogsPath}\build.${buildName}.log;verbosity=normal`"",
    "/fl2 `"/flp2:logfile=${buildlogsPath}\build.${buildName}.err;errorsonly`"",
    "/fl3 `"/flp3:logfile=${buildlogsPath}\build.${buildName}.wrn;warningsonly`"",
    "/verbosity:normal"
    ) -join " "

$targets = ""
if ($clean) {
    $targets += "`"/t:Clean,Rebuild`""
}

$binary = Join-Path $binpath $binaryName

#
# Build
#

function Build($targets="", $pogoParams) {
    $buildCommand = "& `"$msbuildExe`" $targets $defaultParams $loggingParams $pogoParams"
    ExecuteCommand "$buildCommand"
    if ($global:LastExitCode -ne 0) {
        WriteErrorMessage "Failed msbuild command:`n$buildCommand`n"
        WriteErrorMessage "Build failed. Exiting..."
        exit 1
    }
}

Build -pogoParams "`"/p:POGO_TYPE=PGI`"" -targets "$targets"

$scenariosParamValue = $scenarios -join ','
$pogoTrainingCommand = "& `"${PSScriptRoot}\pgo\pogo_training.ps1`" -arch $arch -flavor $flavor -subtype $subtype -binary $binary -scenarios $scenariosParamValue"
ExecuteCommand "$pogoTrainingCommand"

Build -pogoParams "`"/p:POGO_TYPE=PGO`""

#
# Clean up
#

if (("$binpath" -ne "") -and (Test-Path $binpath)) {
    # remove *.pgc, *.pgd, and pgort*
    Get-ChildItem -Recurse -Path $binpath "*" `
        | ? { $_.Name -match "(.*\.pg[cd]|pgort.*)" } `
        | % { Remove-Item -Force $_.FullName }
}
