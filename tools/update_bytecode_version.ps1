# -------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
# -------------------------------------------------------------------------------------------------------

# Generates a new ByteCodeCacheReleaseFileVersion.h
$scriptRoot=Split-Path -Path $MyInvocation.MyCommand.Path
$versionHeader="$scriptRoot\..\lib\Runtime\ByteCode\ByteCodeCacheReleaseFileVersion.h"

Write-Host "Writing file to $versionHeader"
Remove-Item -Path $versionHeader -Force

function Write-Header() {
    Write-Output $args | Out-File -Encoding ASCII -Append $versionHeader
}

$copyright=@"
//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
// NOTE: If there is a merge conflict the correct fix is to make a new GUID.
// This file was generated with tools\update_bytecode_version.ps1

"@

$version=[Guid]::NewGuid().ToString().ToUpper()

Write-Header $copyright
Write-Header "// {$version}"
Write-Header "const GUID byteCodeCacheReleaseFileVersion ="

$version -match "^(\w{8})-(\w{4})-(\w{4})-(\w{4}-\w{12})$" | Out-Null
$majorParts=$Matches
$Matches[4] -match "^(\w{2})(\w{2})-(\w{2})(\w{2})(\w{2})(\w{2})(\w{2})(\w{2})$" | Out-Null
$minorParts=$Matches
$minorStr="0x{0}, 0x{1}, 0x{2}, 0x{3}, 0x{4}, 0x{5}, 0x{6}, 0x{7}" -f $minorParts[1], $minorParts[2], $minorParts[3], $minorParts[4], $minorParts[5], $minorParts[6], $minorParts[7], $minorParts[8]
$majorStr="0x{0}, 0x{1}, 0x{2}" -f $majorParts[1], $majorParts[2], $majorParts[3]
Write-Header "{ $majorStr, { $minorStr } };"
