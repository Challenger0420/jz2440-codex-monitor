param([string]$Elf = "build/codex-monitor-oabi")
$ErrorActionPreference = "Stop"
if (-not (Test-Path -LiteralPath $Elf)) { throw "ELF not found: $Elf" }
$tools = @("file", "readelf", "objdump")
foreach ($tool in $tools) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        throw "$tool not found; run this script in the ARM toolchain environment."
    }
}
Write-Output "=== file ==="
file $Elf
Write-Output "=== readelf -h ==="
readelf -h $Elf
Write-Output "=== readelf -A ==="
readelf -A $Elf
Write-Output "=== objdump -f ==="
 $formatOutput = objdump -f $Elf 2>$null | Out-String
Write-Output $formatOutput
Write-Output "=== suspicious instruction mnemonics (manual review) ==="
if ($formatOutput -match "architecture:\s+UNKNOWN") {
    Write-Output "objdump cannot disassemble this ARM ELF with the current host binutils; rerun with target binutils."
} else {
    $disassembly = objdump -d $Elf 2>$null
    $disassembly | Select-String -Pattern "clz|blx|qadd|qsub|smlal|smul|ldrex|strex|cps|bkpt"
}
