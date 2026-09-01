[CmdletBinding()]
param(
    [Parameter()]
    [string]$OutputPath = 'artifacts\content-report.md',

    [Parameter()]
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',

    [Parameter()]
    [ValidateSet('x64', 'x86')]
    [string]$Platform = 'x64'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$repositoryPrefix = $repositoryRoot.TrimEnd([System.IO.Path]::DirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
$resolvedOutput = if ([System.IO.Path]::IsPathRooted($OutputPath)) {
    [System.IO.Path]::GetFullPath($OutputPath)
}
else {
    [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot $OutputPath))
}
if (-not $resolvedOutput.StartsWith($repositoryPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'OutputPath must stay inside the repository.'
}

$testPlatform = if ($Platform -eq 'x64') { 'x64' } else { 'Win32' }
$toolPath = Join-Path $repositoryRoot "$Configuration\tests\$testPlatform\PeglinCoreTests.exe"
if (-not (Test-Path -LiteralPath $toolPath -PathType Leaf)) {
    throw "Content report executable was not found. Build $Configuration|$Platform first."
}

$stageCatalog = Join-Path $repositoryRoot 'FinalProject_Peglin\content\stages.v1.ini'
$gameplayCatalog = Join-Path $repositoryRoot 'FinalProject_Peglin\content\gameplay.v1.ini'
& $toolPath --content-report $stageCatalog $gameplayCatalog $resolvedOutput
if ($LASTEXITCODE -ne 0) {
    throw "Content report validation failed. ExitCode=$LASTEXITCODE"
}
Write-Output "CONTENT REPORT PASS: $resolvedOutput"
