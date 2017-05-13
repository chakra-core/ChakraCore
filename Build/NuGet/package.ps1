#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

$root = (split-path -parent $MyInvocation.MyCommand.Definition) + '\..'

$packageRoot = "$root\NuGet"
$packageVersionFile = "$packageRoot\.pack-version"
$packageArtifacts = "$packageRoot\Artifacts"
$targetNugetExe = "$packageRoot\nuget.exe"

If (Test-Path $packageArtifacts)
{
    # Delete any existing output.
    Remove-Item $packageArtifacts\*.nupkg
}

If (!(Test-Path $targetNugetExe))
{
    $sourceNugetExe = "https://dist.nuget.org/win-x86-commandline/latest/nuget.exe"

    Write-Host "NuGet.exe not found - downloading latest from $sourceNugetExe"

    $sourceNugetExe = "https://dist.nuget.org/win-x86-commandline/latest/nuget.exe"

    Invoke-WebRequest $sourceNugetExe -OutFile $targetNugetExe
}

$versionStr = (Get-Content $packageVersionFile) 

Write-Host "Setting .nuspec version tag to $versionStr"

$compiledNuspec = "$root\nuget\compiled.nuspec"

# Create new packages for any nuspec files that exist in this directory.
Foreach ($nuspec in $(Get-Item $packageRoot\*.nuspec))
{
    $content = (Get-Content $nuspec)
    $content = $content -replace '\$version\$',$versionStr
    $content | Out-File $compiledNuspec

    & $targetNugetExe pack $compiledNuspec -outputdirectory $packageArtifacts
}

# Delete compiled temporary nuspec.
If (Test-Path $compiledNuspec)
{
    Remove-Item $compiledNuspec
}