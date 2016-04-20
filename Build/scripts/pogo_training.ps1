param (
    [string[]]$scenarios = @(),

    [Parameter(Mandatory=$True)]
    [string]$binary,

    [Parameter(Mandatory=$True)]
    [string]$arch,

    # force callers to specify this in case of future use
    [Parameter(Mandatory=$True)]
    [string]$flavor = ""
)

$binpath = Split-Path -Path $binary -Parent
$pgodll = Join-Path $binpath "pgort140.dll";
if (-not (Test-Path ($pgodll))) {
    $vcarchpath = ""
    if ($arch -eq "x64") {
        $vcarchpath = "amd64"
    } elseif ($arch -eq "arm") {
        $vcarchpath = "arm"
    }
    $pgoSrcDll = Join-Path ${env:ProgramFiles(x86)} (Join-Path (Join-Path "Microsoft Visual Studio 14.0\VC\bin" $vcarchpath) "pgort140.dll")
    Copy-Item $pgoSrcDll $pgodll
}

for ($i=0; $i -lt $scenarios.Length; $i = $i+1) {
    $path = $scenarios[$i]

    $items = @()
    if (Test-Path $path -PathType Container) {
        # *.js files in directories
        $items = Get-ChildItem -Path $path -Filter "*.js" | % {join-path $path $_ }
    }
    else {
        $items = @($path)
    }

    for ($j=0; $j -lt $items.Length; $j = $j+1) {
        $testFile = $items[$j]
        Write-Host "$binary $testFile"
        iex "$binary $testFile"
    }
}
