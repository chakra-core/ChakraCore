#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

# Use this script to run a build for the given BuildType (arch, flavor, subtype)

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
    [string]$solutionFile,

    [switch]$clean,

    [string]$binDir = "", # will be inferred if not provided.
    [string]$buildlogsSubdir = "buildlogs",

    # assume NuGet is on the path, otherwise the caller must specify an explicit path
    [string]$nugetExe = "NuGet.exe",

    [string]$logFile = "",

    #
    # Skip flags
    #

    [switch]$skipPogo, # or set $Env:SKIP_POGO before invoking build

    #
    # POGO training parameters
    #

    [string[]]$scenarios = @(),
    [string]$binpath, # will be inferred if not provided
    [string]$binaryName = "ch.exe"
)

#
# Configure logging
#

$OuterScriptRoot = $PSScriptRoot
. $PSScriptRoot\pre_post_util.ps1
. $PSScriptRoot\locate_msbuild.ps1

$buildName = ConstructBuildName -arch $arch -flavor $flavor -subtype $subtype

if (($logFile -eq "") -and (Test-Path Env:\TF_BUILD_BINARIESDIRECTORY)) {
    $logFile = "${Env:TF_BUILD_BINARIESDIRECTORY}\logs\run_build.${Env:BuildName}.log"
}

if (($logFile -ne "") -and (Test-Path $logFile)) {
    Remove-Item $logFile -Force
}

#
# NuGet restore
#

if (-not (Get-Command $nugetExe -ErrorAction SilentlyContinue)) {
    ExecuteCommand "& $nugetExe restore $solutionFile -NonInteractive"
}

#
# Setup
#

$msbuildExe = Locate-MSBuild
if (-not $msbuildExe) {
    WriteErrorMessage "Could not find msbuild.exe -- exiting..."
    exit 1
}

$binDir = UseValueOrDefault "$binDir" "${Env:BinariesDirectory}" "${Env:BUILD_SOURCESDIRECTORY}\Build\VcBuild"
$buildlogsPath = Join-Path $binDir $buildlogsSubdir

$skipPogo = $skipPogo -or (Test-Path Env:\SKIP_POGO)

if (("$binpath" -ne "") -or (-not (Test-Path $binpath))) {
    $binpath = Join-Path $binDir "bin\${buildName}"
}

$defaultParams = "$solutionFile /nologo /m /nr:false /p:platform=`"${arch}`" /p:configuration=`"${flavor}`""
$loggingParams = @(
    "/fl1 `"/flp1:logfile=${buildlogsPath}\build.${Env:BuildName}.log;verbosity=normal`"",
    "/fl2 `"/flp2:logfile=${buildlogsPath}\build.${Env:BuildName}.err;errorsonly`"",
    "/fl3 `"/flp3:logfile=${buildlogsPath}\build.${Env:BuildName}.wrn;warningsonly`"",
    "/verbosity:normal"
    ) -join " "

$targets = ""
if ($clean) {
    $targets += "`"/t:Clean,Rebuild`""
}

#
# Build
#

function Build($targets="", $extraParams) {
    $buildCommand = "& `"$msbuildExe`" $targets $defaultParams $loggingParams $extraParams"
    ExecuteCommand "$buildCommand"
    if ($global:LastExitCode -ne 0) {
        WriteErrorMessage "Failed msbuild command:`n$buildCommand`n"
        WriteErrorMessage "Build failed. Exiting..."
        exit 1
    }
}

if ($subtype -eq "pogo") {
    if ($scenarios.Length -eq 0) {
        WriteMessage "No training scenarios selected for pogo build. Please specify training scenarios using the -scenarios parameter."
        exit 1
    }

    Build -extraParams "`"/p:POGO_TYPE=PGI`"" -targets "$targets"

    if (-not $skipPogo) {
        $scenariosParamValue = $scenarios -join ','
        $binary = Join-Path $binpath $binaryName
        if (($binary -ne "") -and (Test-Path $binary)) {
            $pogoTrainingCommand = "& `"${PSScriptRoot}\pgo\pogo_training.ps1`" -arch $arch -flavor $flavor -subtype $subtype -binary $binary -scenarios $scenariosParamValue"
            ExecuteCommand "$pogoTrainingCommand"
        } else {
            WriteMessage "Binary not found at `"$binary`". Exiting..."
            exit 1
        }

        Build -extraParams "`"/p:POGO_TYPE=PGO`""
    }
} else {
    $subtypeParams = ""
    if ($subtype -eq "codecoverage") {
        $subtypeParams = "/p:ENABLE_CODECOVERAGE=true"
    }

    Build -extraParams $subTypeParams -targets $targets
}

#
# Clean up
#

if (("$binpath" -ne "") -and (Test-Path $binpath)) {
    # remove *.pgc, *.pgd, and pgort*
    Get-ChildItem -Recurse -Path $binpath "*" `
        | ? { $_.Name -match "(.*\.pg[cd]|pgort.*)" } `
        | % { Remove-Item -Force $_.FullName }
}

exit $global:lastexitcode
