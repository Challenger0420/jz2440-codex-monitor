param(
    [string]$InstallDir = (Join-Path $env:LOCALAPPDATA 'JZ2440CodexMonitor')
)

$ErrorActionPreference = 'Stop'
$taskName = 'JZ2440 Codex Monitor Bridge'
$repoRoot = Split-Path -Parent $PSScriptRoot
$bridgeExe = Join-Path $repoRoot 'bridge\CodexQuotaBridge.exe'
$runner = Join-Path $InstallDir 'run-bridge.ps1'
$launcher = Join-Path $InstallDir 'start-bridge.cmd'

if (-not (Test-Path -LiteralPath $bridgeExe -PathType Leaf)) {
    throw "Bridge executable not found: $bridgeExe"
}

New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $InstallDir 'logs') | Out-Null
Copy-Item -LiteralPath $bridgeExe -Destination (Join-Path $InstallDir 'CodexQuotaBridge.exe') -Force
Copy-Item -LiteralPath (Join-Path $repoRoot 'scripts\run-bridge.ps1') -Destination $runner -Force
Copy-Item -LiteralPath (Join-Path $repoRoot 'scripts\stop-board-monitor.ps1') -Destination (Join-Path $InstallDir 'stop-board-monitor.ps1') -Force
Copy-Item -LiteralPath (Join-Path $repoRoot 'scripts\start-bridge.cmd') -Destination $launcher -Force

$user = [Security.Principal.WindowsIdentity]::GetCurrent().Name
$actionArgs = '/d /c call {0}' -f $launcher
$action = New-ScheduledTaskAction -Execute $env:ComSpec -Argument $actionArgs
$trigger = New-ScheduledTaskTrigger -AtLogOn -User $user
$principal = New-ScheduledTaskPrincipal -UserId $user -LogonType Interactive -RunLevel Limited
$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -StartWhenAvailable -ExecutionTimeLimit (New-TimeSpan -Seconds 0)

Register-ScheduledTask -TaskName $taskName -Action $action -Trigger $trigger -Principal $principal -Settings $settings -Force | Out-Null
Write-Output "installed Bridge files in $InstallDir"
Write-Output "registered scheduled task: $taskName"
