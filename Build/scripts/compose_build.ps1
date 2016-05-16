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

$outputJsonFile = Join-Path -Path $rootPath -ChildPath "build.json"
$buildInfo = New-Object System.Object

$changeJson = (Get-ChildItem -Path $rootPath "change.json" -Recurse)[0] | % { $_.FullName }
$changeInfo = (Get-Content $changeJson) -join "`n" | ConvertFrom-Json

# Determine the overall build status. Mark the build as "passed" until "failed" is encountered.
$overallBuildStatus = "passed"

$files = Get-ChildItem -Path $rootPath "*.json" -Recurse `
    | ? { ($_.Name -ne "change.json") -and ($_.Name -ne "build.json") } `
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
$buildInfo | ConvertTo-Json | Out-File $outputJsonFile -Encoding ascii
