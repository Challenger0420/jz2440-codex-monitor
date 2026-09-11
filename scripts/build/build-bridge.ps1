$ErrorActionPreference = "Stop"
$root = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$csc = "C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe"
if (-not (Test-Path -LiteralPath $csc)) { throw ".NET Framework csc.exe not found: $csc" }
$sourceRoots = @(
    (Join-Path $root 'platform\host\board-controller'),
    (Join-Path $root 'apps\codex-monitor\host'),
    (Join-Path $root 'apps\codex-monitor\protocol')
)
$sources = Get-ChildItem $sourceRoots -Filter "*.cs" -File | Sort-Object FullName | ForEach-Object FullName
$outDir = Join-Path $root 'build\bridge'
$outExe = Join-Path $outDir 'CodexQuotaBridge.exe'
New-Item -ItemType Directory -Force -Path $outDir | Out-Null
$compilerArgs = @('/nologo', '/target:exe', ('/out:' + $outExe),
    '/reference:System.dll', '/reference:System.Runtime.Serialization.dll',
    '/reference:System.Management.dll') + $sources
& $csc @compilerArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Output "built build/bridge/CodexQuotaBridge.exe"
