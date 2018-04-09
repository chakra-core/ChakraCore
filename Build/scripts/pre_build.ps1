#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

# Pre-Build script
#
# This script is fairly simple. It checks if it's running
# in a VSO. If it is, it uses the VSO environment variables
# to figure out the commit that triggered the build, and if
# such a commit exists, it saves it's description to the build
# output to make it easy to inspect builds.
#
# Require Environment:
#   $Env:TF_BUILD_SOURCEGETVERSION
#   $Env:TF_BUILD_DROPLOCATION
#
# Inferable Environment (if not specified, inferred by pre_post_util.ps1):
#   $Env:TF_BUILD_SOURCESDIRECTORY  (a.k.a. $srcpath)
#   $Env:TF_BUILD_BUILDDIRECTORY    (a.k.a. $objpath)
#   $Env:TF_BUILD_BINARIESDIRECTORY (a.k.a. $binpath)
#
# Optional information (metadata only):
#   $Env:TF_BUILD_BUILDDEFINITIONNAME
#   $Env:TF_BUILD_BUILDNUMBER
#   $Env:TF_BUILD_BUILDURI

param (
    [Parameter(Mandatory=$True)]
    [ValidateSet("x86", "x64", "arm", "arm64")]
    [string]$arch,
    [Parameter(Mandatory=$True)]
    [ValidateSet("debug", "release", "test", "codecoverage")]
    [string]$flavor,
    [ValidateSet("default", "codecoverage", "pogo")]
    [string]$subtype = "default",

    [string]$srcpath = "",
    [string]$binpath = "",
    [string]$objpath = "",
    [string]$logFile = "",

    [string]$corePath = "core",

    [string]$oauth
)

. $PSScriptRoot\pre_post_util.ps1

$srcpath, $buildRoot, $objpath, $binpath = `
    ComputePaths `
        -arch $arch -flavor $flavor -subtype $subtype -OuterScriptRoot $PSScriptRoot `
        -srcpath $srcpath -buildRoot $buildRoot -objpath $objpath -binpath $binpath

WriteCommonArguments

$buildName = ConstructBuildName $arch $flavor $subtype
if (($logFile -eq "") -and (Test-Path $buildRoot)) {
    if (-not(Test-Path -Path "${buildRoot}\logs")) {
        $dummy = New-Item -Path "${buildRoot}\logs" -ItemType Directory -Force
    }
    $logFile = "${buildRoot}\logs\pre_build.${buildName}.log"
    if (Test-Path -Path $logFile) {
        Remove-Item $logFile -Force
    }
}

#
# Create packages.config files
#

$packagesConfigFileText = @"
<?xml version="1.0" encoding="utf-8"?>
<packages>
  <package id="MicroBuild.Core" version="0.2.0" targetFramework="native" developmentDependency="true" />
</packages>
"@

$packagesFiles = Get-ChildItem -Path $Env:TF_BUILD_SOURCESDIRECTORY *.vcxproj -Recurse `
    | ForEach-Object { Join-Path $_.DirectoryName "packages.config" }

foreach ($file in $packagesFiles) {
    if (-not (Test-Path $file)) {
        Write-Output $packagesConfigFileText | Out-File $file -Encoding utf8
    }
}

#
# Create build metadata
#

if (Test-Path Env:\TF_BUILD_SOURCEGETVERSION)
{
    $commitHash = ($Env:TF_BUILD_SOURCEGETVERSION).split(':')[2]
    $gitExe = GetGitPath

    $CoreHash = ""
    if (Test-Path $corePath) {
        $CoreHash = Invoke-Expression "$gitExe rev-parse ${commitHash}:core"
        if (-not $?) {
            $CoreHash = ""
        }
    }

    $outputDir = $Env:TF_BUILD_DROPLOCATION
    if (-not(Test-Path -Path $outputDir)) {
        $dummy = New-Item -Path $outputDir -ItemType Directory -Force
    }

    Push-Location $srcpath

    $info = GetBuildInfo $oauth $commitHash

    $BuildDate = ([DateTime]$info.push.date).toString("yyMMdd-HHmm")

    $buildPushId, $buildPushIdPart1, $buildPushIdPart2, $buildPushIdString = GetBuildPushId $info

    # commit message
    $command = "$gitExe log -1 --name-status -m --first-parent -p $commitHash"
    $CommitMessageLines = Invoke-Expression $command
    $CommitMessage = $CommitMessageLines -join "`r`n"

    $changeTextFile = Join-Path -Path $outputDir -ChildPath "change.txt"

    $changeTextFileContent = @"
TF_BUILD_BUILDDEFINITIONNAME = $Env:TF_BUILD_BUILDDEFINITIONNAME
TF_BUILD_BUILDNUMBER = $Env:TF_BUILD_BUILDNUMBER
TF_BUILD_SOURCEGETVERSION = $Env:TF_BUILD_SOURCEGETVERSION
TF_BUILD_BUILDURI = $Env:TF_BUILD_BUILDURI

PushId = $buildPushId $buildPushIdString
PushDate = $buildDate

$CommitMessage
"@

    Write-Output "-----"
    Write-Output $changeTextFile
    Write-Output $changeTextFileContent
    Write-Output $changeTextFileContent | Out-File $changeTextFile -Encoding utf8 -Append

    Pop-Location

    # commit hash
    $buildCommit = ($Env:TF_BUILD_SOURCEGETVERSION).SubString(14)
    $CommitHash = $buildCommit.Split(":")[1]

    $outputJsonFile = Join-Path -Path $outputDir -ChildPath "change.json"
    $changeJson = New-Object System.Object

    $changeJson | Add-Member -type NoteProperty -name BuildDefinitionName -value $Env:TF_BUILD_BUILDDEFINITIONNAME
    $changeJson | Add-Member -type NoteProperty -name BuildNumber -value $Env:TF_BUILD_BUILDNUMBER
    $changeJson | Add-Member -type NoteProperty -name BuildUri -value $Env:TF_BUILD_BUILDURI
    $changeJson | Add-Member -type NoteProperty -name BuildDate -value $BuildDate
    $changeJson | Add-Member -type NoteProperty -name Branch -value $Env:BranchName
    $changeJson | Add-Member -type NoteProperty -name CommitHash -value $CommitHash
    $changeJson | Add-Member -type NoteProperty -name CoreHash -value $CoreHash
    $changeJson | Add-Member -type NoteProperty -name PushId -value $BuildPushId
    $changeJson | Add-Member -type NoteProperty -name PushIdPart1 -value $BuildPushIdPart1
    $changeJson | Add-Member -type NoteProperty -name PushIdPart2 -value $BuildPushIdPart2
    $changeJson | Add-Member -type NoteProperty -name PushIdString -value $BuildPushIdString
    $changeJson | Add-Member -type NoteProperty -name Username -value $Env:Username
    $changeJson | Add-Member -type NoteProperty -name CommitMessage -value $CommitMessageLines

    ## Look for a git note that requests a cirro run.

    ## We assume that, when this script is called in the context of a full build, the working directory
    ## is the root of the local full checkout.

    ## It's a bit of a layering violation to have this logic, which only applies to full builds, in the
    ## core pre_build script, but there's not an obviously better place to put it.  There is a pre_build
    ## script in the full repo, but it doesn't seem to be executed as part of a VSO build, and it's not
    ## clear if anyone else uses it.
    ##
    ## Since we have to handle the case where a full build doesn't have a ChakraHub note anyway, just
    ## add the logic to the core script.

    ## Without the OAuth token, the git fetch command hangs, causing the entire build to time out.  Per
    ## https://docs.microsoft.com/en-us/vsts/build-release/concepts/definitions/release/variables?view=vsts&tabs=shell#using-default-variables,
    ## we expect to find the auth token in System_AccessToken instead of System.AccessToken.
    if (${Env:System_AccessToken})
    {
        git -c http.extraheader="AUTHORIZATION: bearer ${Env:System_AccessToken}" fetch origin refs/notes/ChakraHub:refs/notes/ChakraHub
        Write-Output "looking for notes"
        ## One might be tempted to redirect the output of the "git notes" command on the next line to $null.  Don't do that;
        ## it causes the check to fail even when git notes are present.  However, we do have to redirect stderr, because
        ## git prints something to stderr if it doesn't find a note, and that would cause VSTS to think that the build failed.
        if (git notes --ref=ChakraHub list HEAD 2>&1)
        {
            Write-Output "found git notes; adding to change.json"
            $testConfig = (git notes --ref=ChakraHub show HEAD) -join "`n" | ConvertFrom-Json
            $changeJson | Add-Member -type NoteProperty -name TestConfig -value $testConfig
        }
        else
        {
            Write-Output "no git notes found"
        }
    }
    else
    {
        Write-Output "No OAuth token found; skipping check for git note that requests a cirro run"
    }

    Write-Output "-----"
    Write-Output $outputJsonFile
    $changeJson | ConvertTo-Json -depth 100 | Write-Output
    $changeJson | ConvertTo-Json -depth 100 | Out-File $outputJsonFile -Encoding utf8

    $buildInfoOutputDir = $objpath
    if (-not(Test-Path -Path $buildInfoOutputDir)) {
        $dummy = New-Item -Path $buildInfoOutputDir -ItemType Directory -Force
    }

    # generate build version prop file
    $propsFile = Join-Path -Path $buildInfoOutputDir -ChildPath "Chakra.Generated.BuildInfo.props"
    $propsFileTemplate = @"
<?xml version="1.0" encoding="utf-16"?>
<Project DefaultTargets="Build" ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup>
    <OutBaseDir>{0}</OutBaseDir>
    <IntBaseDir>{1}</IntBaseDir>
    <ChakraVersionBuildNumber>{2}</ChakraVersionBuildNumber>
    <ChakraVersionBuildQFENumber>{3}</ChakraVersionBuildQFENumber>
    <ChakraVersionBuildCommit>{4}</ChakraVersionBuildCommit>
    <ChakraVersionBuildDate>{5}</ChakraVersionBuildDate>
  </PropertyGroup>
</Project>
"@

    $propsFileContent = $propsFileTemplate -f $buildRoot, $objpath, $buildPushIdPart1, $buildPushIdPart2, $buildCommit, $buildDate

    Write-Output "-----"
    Write-Output $propsFile
    Write-Output $propsFileContent
    Write-Output $propsFileContent | Out-File $propsFile
}

#
# Clean up code analysis summary files in case they get left behind
#

if (($objpath -ne "") -and (Test-Path $objpath)) {
    Get-ChildItem $objpath -Include vc.nativecodeanalysis.all.xml -Recurse | Remove-Item -Verbose
}
