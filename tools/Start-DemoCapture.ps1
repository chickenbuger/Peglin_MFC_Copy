[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [ValidateSet('x64', 'x86')]
    [string]$Platform = 'x64'
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
if ($Platform -eq 'x64') {
    $executable = Join-Path $projectRoot ("x64\{0}\FinalProject_Peglin.exe" -f $Configuration)
} else {
    $executable = Join-Path $projectRoot ("{0}\FinalProject_Peglin.exe" -f $Configuration)
}

if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
    throw "Demo executable is missing. Build $Configuration $Platform first: $executable"
}

$process = Start-Process -FilePath $executable -ArgumentList '--demo' -PassThru
[PSCustomObject]@{
    ProcessId = $process.Id
    Executable = $executable
    Mode = 'Deterministic demo'
    ToggleKey = 'F9'
}
