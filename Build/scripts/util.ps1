#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

function UseValueOrDefault() {
    foreach ($value in $args) {
        if ($value) {
            return $value
        }
    }
    return ""
}

function GetGitPath() {
    $gitExe = "git.exe"

    if (!(Get-Command $gitExe -ErrorAction SilentlyContinue)) {
        $gitExe = "C:\1image\Git\bin\git.exe"
        if (!(Test-Path $gitExe)) {
            throw "git.exe not found in path -- aborting."
        }
    }

    return $gitExe
}

function GetRepoRoot() {
    $gitExe = GetGitPath
    return Invoke-Expression "$gitExe rev-parse --show-toplevel"
}

function WriteMessage($str) {
    Write-Output $str
    if ($logFile) {
        Write-Output $str | Out-File $logFile -Append
    }
}

function WriteErrorMessage($str) {
    $host.ui.WriteErrorLine($str)
    if ($logFile) {
        Write-Output $str | Out-File $logFile -Append
    }
}

function ExecuteCommand($cmd) {
    if ($cmd -eq "") {
        return
    }
    WriteMessage "-------------------------------------"
    WriteMessage "Running $cmd"
    if ($noaction) {
        return
    }
    Invoke-Expression $cmd
    if ($lastexitcode -ne 0) {
        WriteErrorMessage "ERROR: Command failed: exit code $LastExitCode"
        $global:exitcode = $LastExitCode
    }
    WriteMessage ""
}
