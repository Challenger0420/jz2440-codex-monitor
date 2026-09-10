param(
    [string]$Compiler = $env:JZ2440_ARM_GCC,
    [string]$BuildDir = "build"
)

$ErrorActionPreference = "Stop"
if ([string]::IsNullOrWhiteSpace($Compiler)) { $Compiler = "arm-linux-gnueabi-gcc" }
$resolved = Get-Command $Compiler -ErrorAction SilentlyContinue
if ($resolved) { $Compiler = $resolved.Source }
elseif (-not (Test-Path -LiteralPath $Compiler)) {
    throw "ARM GCC not found. Set JZ2440_ARM_GCC or pass -Compiler to an ARMv4T/OABI-capable gcc."
}

$configPath = Join-Path $PSScriptRoot "target-flags.txt"
$section = ""
$cflags = @()
$ldflags = @()
foreach ($line in Get-Content -LiteralPath $configPath) {
    if ($line -eq "[CFLAGS]") { $section = "cflags"; continue }
    if ($line -eq "[LDFLAGS]") { $section = "ldflags"; continue }
    if ([string]::IsNullOrWhiteSpace($line) -or $line.StartsWith("#")) { continue }
    if ($section -eq "cflags") { $cflags += $line }
    elseif ($section -eq "ldflags") { $ldflags += $line }
}

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
Remove-Item -LiteralPath (Join-Path $BuildDir "oabi_start.o"),(Join-Path $BuildDir "codex-monitor-oabi") -Force -ErrorAction SilentlyContinue

& $Compiler @cflags -c src/oabi_start.S -o (Join-Path $BuildDir "oabi_start.o")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $Compiler @cflags @ldflags -o (Join-Path $BuildDir "codex-monitor-oabi") `
    (Join-Path $BuildDir "oabi_start.o") src/oabi_runtime.c src/codex_monitor.c -lgcc
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Output "built $BuildDir/codex-monitor-oabi"
