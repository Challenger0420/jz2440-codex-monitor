$ErrorActionPreference = "Stop"
$csc = "C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe"
if (-not (Test-Path -LiteralPath $csc)) { throw ".NET Framework csc.exe not found: $csc" }
$sources = Get-ChildItem bridge -Filter "*.cs" -File | ForEach-Object FullName
& $csc /nologo /target:exe /out:bridge\CodexQuotaBridge.exe `
    /reference:System.dll /reference:System.Runtime.Serialization.dll /reference:System.Management.dll $sources
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Output "built bridge/CodexQuotaBridge.exe"
