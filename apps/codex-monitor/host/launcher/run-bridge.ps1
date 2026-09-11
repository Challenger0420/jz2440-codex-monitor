$ErrorActionPreference = 'Stop'

$installDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$bridgeExe = Join-Path $installDir 'CodexQuotaBridge.exe'
$logDir = Join-Path $installDir 'logs'
$mutexName = 'JZ2440CodexMonitorBridge'

if (-not (Test-Path -LiteralPath $bridgeExe -PathType Leaf)) {
    throw "Bridge executable not found: $bridgeExe"
}
New-Item -ItemType Directory -Force -Path $logDir | Out-Null

$created = $false
$mutex = New-Object System.Threading.Mutex($true, $mutexName, [ref]$created)
if (-not $created) {
    exit 0
}

try {
    while ($true) {
        $oldLogs = Get-ChildItem -LiteralPath $logDir -Filter '*.log' -File | Sort-Object LastWriteTime -Descending
        if ($oldLogs.Count -gt 20) {
            $oldLogs | Select-Object -Skip 20 | Remove-Item -Force
        }

        $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
        $stdout = Join-Path $logDir ("bridge-$stamp.out.log")
        $stderr = Join-Path $logDir ("bridge-$stamp.err.log")
        $process = Start-Process -FilePath $bridgeExe `
            -ArgumentList @('--interval', '5') `
            -WorkingDirectory $installDir `
            -RedirectStandardOutput $stdout `
            -RedirectStandardError $stderr `
            -WindowStyle Hidden `
            -PassThru
        $process.WaitForExit()
        Start-Sleep -Seconds 5
    }
}
finally {
    if ($null -ne $mutex) {
        $mutex.ReleaseMutex() | Out-Null
        $mutex.Dispose()
    }
}
