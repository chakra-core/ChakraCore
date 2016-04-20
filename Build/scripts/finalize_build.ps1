#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

#
# Clean up the sentinel which previously marked the build as incomplete.
#

Remove-Item -Path ${Env:FlavorBuildIncompleteFile} -Force

#
# Create build status JSON file for this flavor.
#

$FullOutputPath = Join-Path $Env:DROP_ROOT $Env:OutputPath
$BuildLogsPath = Join-Path $FullOutputPath "buildlogs"
$buildFlavorErrFile = Join-Path $BuildLogsPath "build_${Env:BuildPlatform}${Env:BuildConfiguration}.err"

# If build_{}{}.err contains any text then there were build errors and we record that the build failed.
$BuildFailed = $false
if (Test-Path $buildFlavorErrFile) {
    $BuildFailed = (Get-Item $buildFlavorErrFile).length -gt 0
}

$status = "passed"
if ($BuildFailed) {
    $status = "failed"
}

$buildFlavorJsonFile = Join-Path $FullOutputPath "${Env:FlavorName}.json"
$buildFlavorJson = New-Object System.Object

$buildFlavorJson | Add-Member -type NoteProperty -name status -value $status
$buildFlavorJson | Add-Member -type NoteProperty -name arch -value $Env:BuildPlatform
$buildFlavorJson | Add-Member -type NoteProperty -name flavor -value $Env:BuildConfiguration

$buildFlavorJson | ConvertTo-Json | Write-Output
$buildFlavorJson | ConvertTo-Json | Out-File $buildFlavorJsonFile -Encoding ascii
