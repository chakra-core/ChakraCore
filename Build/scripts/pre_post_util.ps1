#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

. $PSScriptRoot\util.ps1

function WriteCommonArguments() {
    WriteMessage "  Source Path: $srcpath"
    WriteMessage "   Build Root: $buildRoot"
    WriteMessage "  Object Path: $objpath"
    WriteMessage "Binaries Path: $binpath"
}

function GetVersionField($fieldname) {
    $gitExe = GetGitPath
    $query = "#define ${fieldname} (\d+)"
    $line = (Invoke-Expression "${gitExe} grep -P ""${query}"" :/")
    $matches = $line | Select-String $query
    if ($matches) {
        return $matches[0].Matches.Groups[1].Value
    }
    return ""
}

function GetBuildInfo($oauth, $commitHash) {
    # Get the git remote path and construct the REST API URI
    $gitExe = GetGitPath
    $remote = (Invoke-Expression "$gitExe remote -v" `
        | Where-Object { $_.contains("_git") })[0].split()[1].replace("_git", "_apis/git/repositories")
    $remote = $remote.replace("mshttps", "https")

    # Get the pushId and push date time to use that for build number and build date time
    $uri = ("{0}/commits/{1}?api-version=1.0" -f $remote, $commitHash)
    $oauthToken = Get-Content $oauth
    $header = @{ Authorization=("Basic {0}" -f $oauthToken) }
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

function EnsureVariables($functionName, $arch, $flavor, $OuterScriptRoot) {
    if (("$arch" -eq "") -or ("$flavor" -eq "") -or ("$OuterScriptRoot" -eq ""))
    {
        WriteErrorMessage @"

        ${functionName}: Required variables not set:
            `$arch = $arch
            `$flavor = $flavor
            `$OuterScriptRoot = $OuterScriptRoot

"@

        throw "Cannot continue: ${functionName}: required variables not set."
    }
}

function ConstructBuildName($arch, $flavor, $subtype) {
    EnsureVariables "ConstructBuildName" $arch $flavor "(OuterScriptRoot not needed)"

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

function ComputePaths($arch, $flavor, $subtype, $OuterScriptRoot, $srcpath = "", $buildRoot = "", $objpath = "", $binpath = "") {
    EnsureVariables "ComputePaths" $arch $flavor $OuterScriptRoot
    $buildName = ConstructBuildName $arch $flavor $subtype
    $srcpath = UseValueOrDefault $srcpath "$Env:TF_BUILD_SOURCESDIRECTORY" (Resolve-Path "${OuterScriptRoot}\..\..")
    $buildRoot = UseValueOrDefault $buildRoot "$Env:BinariesDirectory" "$Env:TF_BUILD_BINARIESDIRECTORY" (Join-Path $srcpath "Build\VcBuild")
    $objpath = UseValueOrDefault $objpath "$Env:TF_BUILD_BUILDDIRECTORY" (Join-Path $buildRoot "obj\${buildName}")
    $binpath = Join-Path $buildRoot "bin\${buildName}"
    return @($srcpath, $buildRoot, $objpath, $binpath)
}
