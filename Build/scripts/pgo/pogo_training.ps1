#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

# PGO Build Workflow:
# - pre_pgi.cmd
# - build (with PGI instrumentation enabled)
# - post_pgi.cmd
# * pogo_training.ps1
# - pre_pgo.cmd
# - build (using PGO profile)
# - post_pgo.cmd

param (
    [string[]]$scenarios = @(),

    [Parameter(Mandatory=$True)]
    [string]$binary,

    [Parameter(Mandatory=$True)]
    [string]$arch,

    # force callers to specify this in case of future use
    [Parameter(Mandatory=$True)]
    [string]$flavor,

    [ValidateSet("default", "codecoverage", "pogo")]
    [string]$subtype = "default",

    [string]$pogoArgs = "",

    [string]$vcinstallroot = ${Env:ProgramFiles(x86)},
    [string]$vcbinpath = "Microsoft Visual Studio 14.0\VC\bin",
    [string]$dllname = "pgort140.dll",
    [string]$dllCheckName = "pgort*.dll"
)

$pogoConfig = ($subtype -eq "pogo") -or (${Env:PogoConfig} -eq "True")
if (-not $pogoConfig) {
    Write-Host "---- Not a Pogo Config. Skipping step."
    return 0
}

$binpath = Split-Path -Path $binary -Parent
$pgoOutDllCheck = Join-Path $binpath $dllCheckName
$pgoOutDll = Join-Path $binpath $dllname
if (-not (Test-Path $pgoOutDllCheck)) {
    if ($arch -eq "x64") {
        $dllname = Join-Path "amd64" $dllname
    } elseif ($arch -eq "arm") {
        $dllname = Join-Path "arm" $dllname
    }
    $pgoSrcDll = Join-Path $vcinstallroot (Join-Path $vcbinpath $dllname)
    Copy-Item $pgoSrcDll $pgoOutDll
}

for ($i = 0; $i -lt $scenarios.Length; $i = $i + 1) {
    $path = $scenarios[$i]

    $items = @()
    if (Test-Path $path -PathType Container) {
        # *.js files in directories
        $items = Get-ChildItem -Path $path -Filter "*.js" | % { join-path $path $_ }
    } else {
        $items = @($path)
    }

    for ($j = 0; $j -lt $items.Length; $j = $j + 1) {
        $testFile = $items[$j]
        Write-Host "$binary $pogoArgs $testFile"
        iex "$binary $pogoArgs $testFile"
    }
}
