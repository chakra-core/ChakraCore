#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

# Init Build Script
#
# Run this as the very first step in the build to configure the environment.
# This is distinct from the Pre-Build script as there may be more non-script steps that need to be
# taken before setting up and running the build.
# For example, this script creates a cmd script which should be run to initialize environment variables
# before running the Pre-Build script.

param (
    [string]$envConfigScript = "ComputedEnvironment.cmd",

    [string[]]$supportedPogoBuildTypes = @("x64_release", "x86_release"),

    [Parameter(Mandatory=$True)]
    [string]$oauth
)

# If $Env:BuildType is specified, extract BuildPlatform and BuildConfiguration
# Otherwise, if $Env:BuildPlatform and $Env:BuildConfiguration are specified, construct $BuildType
$BuildPlatform = $Env:BuildPlatform
$BuildConfiguration = $Env:BuildConfiguration
$BuildType = $Env:BuildType
$BuildSubtype = "default" # will remain as "default" or become e.g. "pogo", "codecoverage"

if (Test-Path Env:\BuildType) {
    $BuildType = $Env:BuildType
    $buildTypeSegments = $BuildType.split("_")
    $BuildPlatform = $buildTypeSegments[0]
    $BuildConfiguration = $buildTypeSegments[1]
    if ($buildTypeSegments[2]) {
        # overwrite with new value if it exists, otherwise keep as "default"
        $BuildSubtype = $buildTypeSegments[2]
    }

    if ($BuildConfiguration -eq "codecoverage") {
        $BuildConfiguration = "test" # codecoverage builds are actually "test" configuration
        $BuildSubtype = "codecoverage" # keep information about codecoverage in the subtype
    }

    if (-not ($BuildSubtype -in @("default","pogo","codecoverage"))) {
        Write-Error "Unsupported BuildSubtype: $BuildSubtype"
    }
} elseif ((Test-Path Env:\BuildPlatform) -and (Test-Path Env:\BuildConfiguration)) {
    $BuildPlatform = $Env:BuildPlatform
    $BuildConfiguration = $Env:BuildConfiguration
    $BuildType = "${BuildPlatform}_${BuildConfiguration}"
} else {
    Write-Error (@"

    Not enough information about BuildType:
        BuildType={0}
        BuildPlatform={1}
        BuildConfiguration={2}

"@ -f $Env:BuildType, $Env:BuildPlatform, $Env:BuildConfiguration)

    exit 1
}

# set up required variables and import pre_post_util.ps1
$arch = $BuildPlatform
$flavor = $BuildConfiguration
$OuterScriptRoot = $PSScriptRoot # Used in pre_post_util.ps1
. "$PSScriptRoot\pre_post_util.ps1"

$gitExe = GetGitPath

$BuildName = ConstructBuildName -arch $BuildPlatform -flavor $BuildConfiguration -subtype $BuildSubtype

$branch = $Env:BUILD_SOURCEBRANCH
if (-not $branch) {
    $branch = iex "$gitExe rev-parse --symbolic-full-name HEAD"
}

$BranchName = $branch.split('/',3)[2]
$BranchPath = $BranchName.replace('/','\')
$CommitHash = $Env:BUILD_SOURCEVERSION
if (-not $CommitHash) {
    $CommitHash = iex "$gitExe rev-parse HEAD"
}

$Username = (iex "$gitExe log $CommitHash -1 --pretty=%ae").split('@')[0]
$CommitDateTime = [DateTime]$(iex "$gitExe log $CommitHash -1 --pretty=%aD")
$CommitTime = Get-Date $CommitDateTime -Format yyMMdd.HHmm

#
# Get Build Info
#

$info = GetBuildInfo $oauth $CommitHash

$BuildPushDate = [datetime]$info.push.date
$PushDate = Get-Date $BuildPushDate -Format yyMMdd.HHmm

$buildPushId, $buildPushIdPart1, $buildPushIdPart2, $buildPushIdString = GetBuildPushId $info

$VersionMajor = UseValueOrDefault "$Env:VERSION_MAJOR" "1"
$VersionMinor = UseValueOrDefault "$Env:VERSION_MINOR" "2"
$VersionPatch = UseValueOrDefault "$Env:VERSION_PATCH" "0"
$VersionQFE   = UseValueOrDefault "$Env:VERSION_QFE"   "0"

$VersionString = "${VersionMajor}.${VersionMinor}.${VersionPatch}.${VersionQFE}"
$PreviewVersionString = "${VersionString}-preview"

# unless it is a build branch, subdivide the output directory by month
if ($BranchPath.StartsWith("build")) {
    $YearAndMonth = ""
} else {
    $YearAndMonth = (Get-Date $BuildPushDate -Format yyMM) + "\"
}

$BuildIdentifier = "${buildPushIdString}_${PushDate}_${Username}_${CommitHash}"
$ComputedDropPathSegment = "${BranchPath}\${YearAndMonth}${BuildIdentifier}"
$BinariesDirectory = "${Env:BUILD_SOURCESDIRECTORY}\Build\VcBuild"
$ObjectDirectory = "${BinariesDirectory}\obj\${BuildPlatform}_${BuildConfiguration}"

# Create a sentinel file for each build flavor to track whether the build is complete.
# * ${arch}_${flavor}.incomplete       # will be deleted when the build of this flavor completes

$buildIncompleteFileContentsString = @"
{0} is incomplete.
This could mean that the build is in progress, or that it was unable to run to completion.
The contents of this directory should not be relied on until the build completes.
"@

$DropPath = Join-Path $Env:DROP_ROOT $ComputedDropPathSegment
New-Item -ItemType Directory -Force -Path $DropPath
New-Item -ItemType Directory -Force -Path (Join-Path $Env:BUILD_SOURCESDIRECTORY "test\logs")
New-Item -ItemType Directory -Force -Path (Join-Path $BinariesDirectory "buildlogs")
New-Item -ItemType Directory -Force -Path (Join-Path $BinariesDirectory "logs")

$FlavorBuildIncompleteFile = Join-Path $DropPath "${BuildType}.incomplete"

if (-not (Test-Path $FlavorBuildIncompleteFile)) {
    ($buildIncompleteFileContentsString -f "Build of ${BuildType}") `
        | Out-File $FlavorBuildIncompleteFile -Encoding Ascii
}

$PogoConfig = $supportedPogoBuildTypes -contains "${Env:BuildPlatform}_${Env:BuildConfiguration}"

# Write the $envConfigScript

@"
set BranchName=${BranchName}
set BranchPath=${BranchPath}
set YearAndMonth=${YearAndMonth}
set BuildIdentifier=${BuildIdentifier}

set buildPushIdString=${buildPushIdString}
set VersionString=${VersionString}
set PreviewVersionString=${PreviewVersionString}
set PushDate=${PushDate}
set CommitTime=${CommitTime}
set Username=${Username}
set CommitHash=${CommitHash}

set ComputedDropPathSegment=${ComputedDropPathSegment}
set BinariesDirectory=${BinariesDirectory}
set DropPath=${DropPath}

set BuildType=${BuildType}
set BuildPlatform=${BuildPlatform}
set BuildConfiguration=${BuildConfiguration}
set BuildSubtype=${BuildSubtype}
set BuildName=${BuildName}

set FlavorBuildIncompleteFile=${FlavorBuildIncompleteFile}

set PogoConfig=${PogoConfig}

"@ `
    | Out-File $envConfigScript -Encoding Ascii

# Use the VSTS environment vars to construct a backwards-compatible VSO build environment
# for the sake of reusing the pre-build and post-build scripts as they are.

@"
set TF_BUILD_SOURCEGETVERSION=LG:${branch}:${CommitHash}
set TF_BUILD_DROPLOCATION=${BinariesDirectory}

set TF_BUILD_SOURCESDIRECTORY=${Env:BUILD_SOURCESDIRECTORY}
set TF_BUILD_BUILDDIRECTORY=${ObjectDirectory}
set TF_BUILD_BINARIESDIRECTORY=${BinariesDirectory}

set TF_BUILD_BUILDDEFINITIONNAME=${Env:BUILD_DEFINITIONNAME}
set TF_BUILD_BUILDNUMBER=${Env:BUILD_BUILDNUMBER}
set TF_BUILD_BUILDURI=${Env:BUILD_BUILDURI}
"@ `
    | Out-File $envConfigScript -Encoding Ascii -Append

# Export VSO variables that can be consumed by other VSO tasks where the task
# definition in VSO itself needs to know the value of the variable.
# If the task definition itself doesn't need to know the value of the variables,
# the variables that are added to the environment via the script generated above
# will be interpolated when the tasks run the associated command line with the
# given parameters.
#
# For example, for a Publish Artifacts task, VSO itself needs to know
# the value of DropPath in order to construct links to the artifacts correctly.
# Thus, we export a variable called VSO_DropPath (VSO_ prefix by convention)
# that the VSO build definition, not just the command environment, will know about.
#
# Uses command syntax documented here:
# https://github.com/Microsoft/vso-agent-tasks/blob/master/docs/authoring/commands.md
# Lines written to stdout that match this pattern are interpreted with this command syntax.

Write-Output "Setting VSO variable VSO_DropPath = ${DropPath}"
Write-Output "##vso[task.setvariable variable=VSO_DropPath;]${DropPath}"

Write-Output "Setting VSO variable VSO_VersionString = ${VersionString}"
Write-Output "##vso[task.setvariable variable=VSO_VersionString;]${VersionString}"

#
# Clean up files that might have been left behind from a previous build.
#

if ((Test-Path Env:\BUILD_BINARIESDIRECTORY) -and (Test-Path "$Env:BUILD_BINARIESDIRECTORY"))
{
    Remove-Item -Verbose "${Env:BUILD_BINARIESDIRECTORY}\*" -Recurse
}
