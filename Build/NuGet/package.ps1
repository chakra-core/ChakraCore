#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

using namespace System.Collections
using namespace System.IO
using namespace System.Text

$packageRoot = Split-Path -Parent $MyInvocation.MyCommand.Definition
$packageVersionFile = Join-Path $packageRoot '.pack-version'
$packageСommonPropertiesFile = Join-Path $packageRoot 'package-common-properties.xml'
$packageArtifactsDir = Join-Path $packageRoot 'Artifacts'
$localNugetExe = Join-Path $packageRoot 'nuget.exe'

# helper for getting properties that are common to all packages
function GetPackageCommonProperties() {
    $version = (Get-Content $packageVersionFile)
    $commonPropertiesXml = [xml](Get-Content $packageСommonPropertiesFile -Encoding utf8)

    $commonProperties = @{ version = $version }

    foreach ($propertyElem in $commonPropertiesXml.DocumentElement.SelectNodes('child::*')) {
        $commonProperties.Add($propertyElem.Name, $propertyElem.'#text')
    }

    return $commonProperties;
}

# helper to create NuGet package
function CreateNugetPackage ([string]$nuspecFile, [Hashtable]$commonProperties) {
    $id = [Path]::GetFileNameWithoutExtension($packageNuspecFile)
    $properties = @{ id = $id }
    $properties += $commonProperties

    $sb = New-Object StringBuilder

    foreach ($propertyName in $properties.Keys){
        $propertyValue = $properties[$propertyName]

        if ($sb.Length -gt 0) {
            [void]$sb.Append(';');
        }
        [void]$sb.AppendFormat('{0}={1}', $propertyName, $propertyValue.Replace('"', '""'));
    }

    $propertiesStr = $sb.toString()
    [void]$sb.Clear()

    & $localNugetExe pack $nuspecFile -OutputDirectory $packageArtifactsDir -Properties $propertiesStr
}

if (Test-Path $packageArtifactsDir) {
    # Delete any existing output.
    Remove-Item "$packageArtifactsDir\*.nupkg"
}

if (!(Test-Path $localNugetExe)) {
    $nugetDistUrl = 'https://dist.nuget.org/win-x86-commandline/latest/nuget.exe'

    Write-Host "NuGet.exe not found - downloading latest from $nugetDistUrl"

    Invoke-WebRequest $nugetDistUrl -OutFile $localNugetExe
}

$packageCommonProperties = GetPackageCommonProperties

# Create new packages for any nuspec files that exist in this directory.
foreach ($packageNuspecFile in $(Get-Item "$packageRoot\*.nuspec")) {
    CreateNugetPackage $packageNuspecFile $packageCommonProperties
}
