# -------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
# -------------------------------------------------------------------------------------------------------

# This file is called as a CustomBuild step from Chakra.ICU.Data.vcxproj.
#
# To generate the data file, we need to build GenCCode.exe for the host platform. We can then run
# GenCCode.exe --object to produce a platform-agnostic object file, which the linking step of
# Chakra.ICU.Data should be able to link into a DLL for the target platform.

param(
    [parameter(Mandatory=$true)]
    [alias("d")]
    [string]$DataFile,

    [parameter(Mandatory=$true)]
    [alias("p")]
    [string]$TargetPlatform,

    [parameter(Mandatory=$true)]
    [alias("c")]
    [string]$TargetConfiguration,

    [parameter(Mandatory=$true)]
    [alias("m")]
    [string]$MSBuildPath,

    [parameter(Mandatory=$true)]
    [alias("i")]
    [string]$IntDir,

    [parameter(Mandatory=$true)]
    [alias("v")]
    [string]$IcuVersionMajor
)

$scriptRoot=Split-Path -Path $MyInvocation.MyCommand.Path

# This gets the actual platform of the host, as opposed to the %PROCESSOR_ARCHITECTURE% environment variable which
# changes depending on if 32 and 64 bit binaries are calling each other
$hostPlatform=(Get-ItemProperty "HKLM:\System\CurrentControlSet\Control\Session Manager\Environment").PROCESSOR_ARCHITECTURE
if ($hostPlatform -eq "AMD64") {
    $hostPlatform="x64"
} elseif ($hostPlatform -eq "X86") {
    $hostPlatform="x86"
}

Write-Host DataFile: $DataFile
Write-Host TargetPlatform: $TargetPlatform
Write-Host MSBuildPath: $MSBuildPath
Write-Host HostPlatform: $hostPlatform

$sep="_"
$genccode="$scriptRoot\..\..\Build\VcBuild\bin\$hostPlatform" + "_release\Chakra.ICU.GenCCode.exe"

if (-not (Test-Path $genccode)) {
    Write-Host
    Write-Host Could not find $genccode, building from scratch
    cmd /c "$MSBuildPath" /nologo "$scriptRoot\..\..\deps\Chakra.ICU\Chakra.ICU.GenCCode.vcxproj" "/p:Platform=$hostPlatform;Configuration=Release;SolutionDir=$scriptRoot\..\..\Build\"
}

Write-Host
Write-Host Building object file
cmd /c "$genccode" --object --destdir $IntDir --entrypoint icudt$IcuVersionMajor $DataFile

Write-Host "Object file created in $IntDir"
