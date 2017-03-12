#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

# Finalize Build Script
#
# This script is run as the final step in a build definition
# to clean up and produce metadata about the build.

param (
    [Parameter(Mandatory=$True)]
    [ValidateSet("x86", "x64", "arm")]
    [string]$arch,

    [Parameter(Mandatory=$True)]
    [ValidateSet("debug", "release", "test", "codecoverage")]
    [string]$flavor,

    [ValidateSet("default", "codecoverage", "pogo")]
    [string]$subtype = "default",

    $corePathSegment = "" # e.g. "core"
)

. $PSScriptRoot\pre_post_util.ps1

$sourcesDir, $_, $_, $_ = `
    ComputePaths `
        -arch $arch -flavor $flavor -subtype $subtype -OuterScriptRoot $PSScriptRoot

$coreSourcesDir = Join-Path $sourcesDir $corePathSegment

$buildName = ConstructBuildName -arch $arch -flavor $flavor -subtype $subtype

#
# Clean up the sentinel which previously marked this build flavor as incomplete.
#

Remove-Item -Path $Env:FlavorBuildIncompleteFile -Force

#
# Copy all logs to DropPath
#

$buildlogsDropPath = Join-Path $Env:DropPath "buildlogs"
$logsDropPath = Join-Path $Env:DropPath "logs"
$testlogsDropPath = Join-Path $Env:DropPath "testlogs"

New-Item -Force -ItemType Directory -Path $buildlogsDropPath
New-Item -Force -ItemType Directory -Path $logsDropPath
New-Item -Force -ItemType Directory -Path $testlogsDropPath

$buildlogsSourcePath = Join-Path $Env:BinariesDirectory "buildlogs"
$logsSourcePath = Join-Path $Env:BinariesDirectory "logs"
$testlogsSourcePath = Join-Path $Env:BinariesDirectory "testlogs"

if (Test-Path $buildlogsSourcePath) {
    Copy-Item -Verbose -Recurse -Force (Join-Path $buildlogsSourcePath "*") $buildlogsDropPath
}
if (Test-Path $logsSourcePath) {
    Copy-Item -Verbose -Recurse -Force (Join-Path $logsSourcePath "*") $logsDropPath
}
if (Test-Path $testlogsSourcePath) {
    Copy-Item -Verbose -Recurse -Force (Join-Path $testlogsSourcePath "*") $testlogsDropPath
}

#
# Create build status JSON file for this flavor.
#

$buildErrFile = Join-Path $buildLogsDropPath "build.${buildName}.err"
$testSummaryFile = Join-Path $testLogsDropPath "summary.${arch}${flavor}.log"

# if build.*.err contains any text then there were build errors
$BuildSucceeded = $true
if (Test-Path $buildErrFile) {
    # build errors file has non-zero length iff the build failed
    $BuildSucceeded = (Get-Item $buildErrFile).length -eq 0
}

# if summary.*.err contains any text then there were test failures
$TestsPassed = $true
if (Test-Path $testSummaryFile) {
    # summary file has non-zero length iff the tests failed
    $TestsPassed = (Get-Item $testSummaryFile).length -eq 0
}

$Status = "passed"
if ((-not $BuildSucceeded) -or (-not $TestsPassed)) {
    $Status = "failed"
}

$buildFlavorJsonFile = Join-Path $Env:DropPath "${Env:BuildName}.json"
$buildFlavorJson = New-Object System.Object

$buildFlavorJson | Add-Member -type NoteProperty -name buildName -value $Env:BuildName
$buildFlavorJson | Add-Member -type NoteProperty -name status -value $Status
$buildFlavorJson | Add-Member -type NoteProperty -name BuildSucceeded -value $BuildSucceeded
$buildFlavorJson | Add-Member -type NoteProperty -name testsPassed -value $TestsPassed
$buildFlavorJson | Add-Member -type NoteProperty -name arch -value $Env:BuildPlatform
$buildFlavorJson | Add-Member -type NoteProperty -name flavor -value $Env:BuildConfiguration
$buildFlavorJson | Add-Member -type NoteProperty -name subtype -value $Env:BuildSubtype

$buildFlavorJson | ConvertTo-Json | Write-Output
$buildFlavorJson | ConvertTo-Json | Out-File $buildFlavorJsonFile -Encoding utf8

#
# Copy outputs to metadata directory
#

$metadataDir = Join-Path $coreSourcesDir "metadata"
New-Item -Force -ItemType Directory -Path $metadataDir

Copy-Item -Verbose -Force (Join-Path $sourcesDir "ComputedEnvironment.cmd") $metadataDir
Copy-Item -Verbose -Force (Join-Path $coreSourcesDir "Build\scripts\compose_build.ps1") $metadataDir
Copy-Item -Verbose -Force (Join-Path $Env:BinariesDirectory "change.json") $metadataDir
Copy-Item -Verbose -Force (Join-Path $Env:BinariesDirectory "change.txt") $metadataDir
Copy-Item -Verbose -Force $buildFlavorJsonFile $metadataDir

# Search for *.nuspec files and copy them to $metadataDir
Get-ChildItem -Path (Join-Path $sourcesDir "Build") "*.nuspec" `
    | ForEach-Object { Copy-Item -Verbose -Force $_.FullName $metadataDir }

#
# Copy POGO directory if present for this build
#

$PogoFolder = Join-Path $Env:BinariesDirectory "bin\${Env:BuildType}_pogo"
if (Test-Path $PogoFolder) {
    $BinDropPath = Join-Path $Env:DropPath "bin"
    Write-Output "Copying `"$PogoFolder`" to `"$BinDropPath`"..."
    Copy-Item -Verbose $PogoFolder $BinDropPath -Recurse -Force
}
