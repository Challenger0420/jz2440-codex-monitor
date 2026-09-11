param(
    [switch]$RestartTask,
    [string]$TaskName = 'JZ2440 Codex Monitor Bridge'
)

$ErrorActionPreference = 'Stop'

$installDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$bridgeExe = Join-Path $installDir 'CodexQuotaBridge.exe'

$task = Get-ScheduledTask -TaskName $TaskName -ErrorAction SilentlyContinue
if ($null -ne $task) {
    Stop-ScheduledTask -TaskName $TaskName -ErrorAction SilentlyContinue
}

$deadline = (Get-Date).AddSeconds(10)
do {
    $bridgeProcesses = @(Get-CimInstance Win32_Process -Filter "Name = 'CodexQuotaBridge.exe'" |
        Where-Object { $_.ExecutablePath -eq $bridgeExe }
    )
    if ($bridgeProcesses.Count -eq 0) { break }
    Start-Sleep -Milliseconds 250
} while ((Get-Date) -lt $deadline)

if ($bridgeProcesses.Count -ne 0) {
    throw "Bridge process did not release the serial port within 10 seconds."
}

& $bridgeExe --quit
if ($LASTEXITCODE -ne 0) {
    throw "CQMQUIT helper failed with exit code $LASTEXITCODE."
}

if ($RestartTask -and $null -ne $task) {
    Start-ScheduledTask -TaskName $TaskName
}
