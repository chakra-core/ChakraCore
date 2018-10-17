#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

param (
    [string]$directory,
    [string]$logFile = ""
)

. "$PSScriptRoot\util.ps1"

if ((Test-Path $directory) -eq 0) {
    Write-Error ("ERROR: Directory {0} does not exist.  Cannot scan for prefast defects" -f $directory);
    Exit(-1)
}

if (Test-Path -Path $logFile) {
    Remove-Item $logFile -Force
}

# load all prefast results
$files = Get-ChildItem -recurse ("{0}\vc.nativecodeanalysis.all.xml" -f $directory)

$filecount = 0;
$count = 0;
foreach ($file in $files) {
    $filecount++;
    [xml]$x = Get-Content $file
    foreach ($d in $x.DEFECTS.DEFECT) {
        WriteErrorMessage ("{0}{1}({2}): warning C{3}: {4}" -f $d.SFA.FILEPATH, $d.SFA.FILENAME, $d.SFA.LINE, $d.DEFECTCODE, $d.DESCRIPTION)
        $count++;
    }
}

if ($count -ne 0) {
    WriteErrorMessage ("ERROR: {0} prefast warning detected" -f $count)
} elseif ($filecount -ne 0) {
    WriteMessage "No prefast warning detected"
} else {
    WriteMessage "No prefast result found"
}
exit $count

