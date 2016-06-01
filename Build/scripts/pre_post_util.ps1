#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

. "$PSScriptRoot\util.ps1"
. "$PSScriptRoot\locate_msbuild.ps1"

function WriteCommonArguments() {
    WriteMessage "  Source Path: $srcpath"
    WriteMessage "  Object Path: $objpath"
    WriteMessage "Binaries Path: $binpath"
}

function GetBuildInfo($oauth, $commitHash) {
    # Get the git remote path and construct the rest API URI
    $gitExe = GetGitPath
    $remote = (iex "$gitExe remote -v")[0].split()[1].replace("_git", "_apis/git/repositories")
    $remote = $remote.replace("mshttps", "https")

    # Get the pushId and push date time to use that for build number and build date time
    $uri = ("{0}/commits/{1}?api-version=1.0" -f $remote, $commitHash)
    $oauthToken = Get-Content $oauth
    $header = @{Authorization=("Basic {0}" -f $oauthToken) }
    $info = Invoke-RestMethod -Headers $header -Uri $uri -Method GET

    return $info
}

# Compute paths

if (("$arch" -eq "") -or ("$flavor" -eq "") -or ("$OuterScriptRoot" -eq ""))
{
    WriteErrorMessage @"

    Required variables not set before script was included:
        `$arch = $arch
        `$flavor = $flavor
        `$OuterScriptRoot = $OuterScriptRoot

"@

    throw "Cannot continue - required variables not set."
}

$srcpath = UseValueOrDefault $srcpath "$env:TF_BUILD_SOURCESDIRECTORY" (Resolve-Path "$OuterScriptRoot\..\..")
$objpath = UseValueOrDefault $objpath "$env:TF_BUILD_BUILDDIRECTORY" "${srcpath}\Build\VcBuild\obj\${arch}_${flavor}"
$binpath = UseValueOrDefault $binpath "$env:TF_BUILD_BINARIESDIRECTORY" "${srcpath}\Build\VcBuild"
