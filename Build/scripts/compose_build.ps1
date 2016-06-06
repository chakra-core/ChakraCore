#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

# Compose Build script
#
# Aggregate metadata about a build and produce a file with useful information about the build
# for tools to consume and get a quick overview of the status of a completed build.

param (
    [Parameter(Mandatory=$True)]
    [string]$rootPath
)

#
# Aggregate build metadata and produce build.json
#

$outputJsonFile = Join-Path $rootPath "build.json"
$buildInfo = New-Object System.Object

$changeJson = (Get-ChildItem -Path $rootPath "change.json" -Recurse)[0].FullName
$changeText = (Get-ChildItem -Path $rootPath "change.txt"  -Recurse)[0].FullName
Copy-Item -Verbose -Force -Path $changeJson -Destination $rootPath
Copy-Item -Verbose -Force -Path $changeText -Destination $rootPath

$changeInfo = (Get-Content $changeJson) -join "`n" | ConvertFrom-Json

# Recursively locate ${arch}_${flavor}.json and move to $rootPath.
# This ensures that in the rebuild scenario, we don't have duplication of *.json files
# between the partially-composed root and the metadata directories.

Get-ChildItem -Path $rootPath "*.json" -Recurse `
    | ? { -not ($_.Name -in @("change.json", "build.json")) } `
    | % { Move-Item -Verbose -Force -Path $_.FullName -Destination $rootPath }

# Determine the overall build status. Mark the build as "passed" until "failed" is encountered.
$overallBuildStatus = "passed"

$files = Get-ChildItem -Path $rootPath "*.json" -Recurse `
    | ? { -not ($_.Name -in @("change.json", "build.json")) } `
    | % { $_.FullName }
$builds = New-Object System.Collections.ArrayList
foreach ($file in $files) {
    $json = (Get-Content $file) -join "`n" | ConvertFrom-Json
    $_ = $builds.Add($json)

    if ($json.status -eq "failed") {
        $overallBuildStatus = "failed"
    }
}

$buildInfo | Add-Member -type NoteProperty -name status -value $overallBuildStatus
$buildInfo | Add-Member -type NoteProperty -name change -value $changeInfo
$buildInfo | Add-Member -type NoteProperty -name builds -value $builds

$buildInfo | ConvertTo-Json | Write-Output
$buildInfo | ConvertTo-Json | Out-File $outputJsonFile -Encoding Ascii
