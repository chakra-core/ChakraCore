#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Copyright (c) ChakraCore Project Contributors. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

using namespace System.Text

using module '.\package-classes.psm1'

Set-StrictMode -Version 5.1

$packageRoot = Split-Path -Parent $MyInvocation.MyCommand.Definition
$packageVersionFile = Join-Path $packageRoot '.pack-version'
$packageDataFile = Join-Path $packageRoot 'package-data.xml'
$packageArtifactsDir = Join-Path $packageRoot 'Artifacts'
$localNugetExe = Join-Path $packageRoot 'nuget.exe'

# helper to download file with retry
function DownloadFileWithRetry([string]$sourceUrl, [string]$destFile, [int]$retries) {
    $delayTimeInSeconds = 5

    while ($true) {
        try {
            Invoke-WebRequest $sourceUrl -OutFile $destFile
            break
        }
        catch {
            Write-Host "Failed to download $sourceUrl"

            if ($retries -gt 0) {
                $retries--

                Write-Host "Waiting $delayTimeInSeconds seconds before retrying. Retries left: $retries"
                Start-Sleep -Seconds $delayTimeInSeconds
            }
            else {
                $exception = $_.Exception
                throw $exception
            }
        }
    }
}

# helper to create NuGet package
function CreateNugetPackage ([Package]$package, [string]$version, [string]$outputDir) {
    $properties = $package.Properties.Clone()
    $properties['id'] = $package.Id
    $properties['version'] = $version

    $sb = New-Object StringBuilder

    foreach ($propertyName in $properties.Keys) {
        $propertyValue = $properties[$propertyName]

        if ($sb.Length -gt 0) {
            [void]$sb.Append(';')
        }
        [void]$sb.AppendFormat('{0}={1}', $propertyName, $propertyValue.Replace('"', '""'))
    }

    $propertiesStr = $sb.toString()
    [void]$sb.Clear()

    $package.PreprocessFiles()
    & $localNugetExe pack $package.NuspecFile -OutputDirectory $outputDir -Properties $propertiesStr
    $package.RemovePreprocessedFiles()
}

if (Test-Path $packageArtifactsDir) {
    # Delete any existing output.
    Remove-Item "$packageArtifactsDir\*.nupkg"
}

if (!(Test-Path $localNugetExe)) {
    $nugetDistUrl = 'https://dist.nuget.org/win-x86-commandline/latest/nuget.exe'

    Write-Host "NuGet.exe not found - downloading latest from $nugetDistUrl"
    DownloadFileWithRetry $nugetDistUrl $localNugetExe -retries 3
}

# Create new NuGet packages based on data from an XML file.
$version = (Get-Content $packageVersionFile)
$packages = [Package]::GetPackages($packageDataFile)

foreach ($package in $packages) {
    CreateNugetPackage $package $version $packageArtifactsDir
}
