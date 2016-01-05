function WriteMessage($str)
{
    Write-Output $str
    if ($logFile -ne "")
    {
        Write-Output $str | Out-File $logFile -Append
    }
}

function WriteErrorMessage($str)
{
    $host.ui.WriteErrorLine($str);
    if ($logFile -ne "")
    {
        Write-Output $str | Out-File $logFile -Append
    }
}

function ExecuteCommand($cmd) {
    if ($cmd -eq "") {
        return;
    }
    WriteMessage "-------------------------------------"
    WriteMessage "Running $cmd"
    if ($noaction) {
        return;
    }
    Invoke-Expression $cmd
    if($lastexitcode -ne 0) {
        WriteErrorMessage "ERROR: Command failed: exit code $LastExitCode"
        $global:exitcode = $LastExitCode;
    }
    WriteMessage ""
}
