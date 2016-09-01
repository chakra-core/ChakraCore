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
    # Get the git remote path and construct the REST API URI
    $gitExe = GetGitPath
    $remote = (iex "$gitExe remote -v" | ? { $_.contains("_git") })[0].split()[1].replace("_git", "_apis/git/repositories")
    $remote = $remote.replace("mshttps", "https")

    # Get the pushId and push date time to use that for build number and build date time
    $uri = ("{0}/commits/{1}?api-version=1.0" -f $remote, $commitHash)
    $oauthToken = Get-Content $oauth
    $header = @{Authorization=("Basic {0}" -f $oauthToken) }
    $info = Invoke-RestMethod -Headers $header -Uri $uri -Method GET

    return $info
}

function GetBuildPushId($info) {
    $buildPushId = $info.push.pushId
    $buildPushIdPart1 = [int]([math]::Floor($buildPushId / 65536))
    $buildPushIdPart2 = [int]($buildPushId % 65536)
    $buildPushIdString = "{0}.{1}" -f $buildPushIdPart1.ToString("00000"), $buildPushIdPart2.ToString("00000")

    return @($buildPushId, $buildPushIdPart1, $buildPushIdPart2, $buildPushIdString)
}

function ConstructBuildName($arch, $flavor, $subtype) {
    if ($subtype -eq "codecoverage") {
        # TODO eliminate tools' dependency on this particular formatting exception
        # Normalize the $BuildName of even if the $BuildType is e.g. x64_test_codecoverage
        return "${arch}_codecoverage"
    } elseif ($subtype -eq "pogo") {
        return "${arch}_${flavor}_${subtype}"
    } else {
        return "${arch}_${flavor}"
    }
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
