[CmdletBinding()]
param(
    [Parameter()]
    [string]$PackageRoot = $PSScriptRoot,

    [Parameter()]
    [switch]$Launch
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-PeMachine {
    param([Parameter(Mandatory)][string]$Path)

    $stream = [System.IO.File]::OpenRead($Path)
    try {
        $reader = [System.IO.BinaryReader]::new($stream)
        if ($reader.ReadUInt16() -ne 0x5A4D) {
            throw "Not a PE file: $Path"
        }
        $stream.Position = 0x3C
        $peOffset = $reader.ReadInt32()
        if ($peOffset -lt 0 -or $peOffset -gt ($stream.Length - 6)) {
            throw "Invalid PE header offset: $Path"
        }
        $stream.Position = $peOffset
        if ($reader.ReadUInt32() -ne 0x00004550) {
            throw "Invalid PE signature: $Path"
        }
        return $reader.ReadUInt16()
    }
    finally {
        $stream.Dispose()
    }
}

$resolvedRoot = [System.IO.Path]::GetFullPath($PackageRoot)
$requiredFiles = @(
    'FinalProject_Peglin.exe',
    'mfc140u.dll',
    'msvcp140.dll',
    'vcruntime140.dll',
    'vcruntime140_1.dll',
    'PACKAGE_VERSION.txt',
    'Preflight.ps1',
    'README.txt',
    'SHA256SUMS.txt',
    'content\stages.v1.ini',
    'content\gameplay.v1.ini'
)

if (-not [Environment]::Is64BitOperatingSystem) {
    throw 'This package requires a Windows x64 operating system.'
}

foreach ($relativePath in $requiredFiles) {
    $fullPath = Join-Path $resolvedRoot $relativePath
    if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
        throw "A required file is missing: $relativePath"
    }
}

$executablePath = Join-Path $resolvedRoot 'FinalProject_Peglin.exe'
if ((Get-PeMachine -Path $executablePath) -ne 0x8664) {
    throw 'The executable is not an x64 PE file.'
}

$hashListPath = Join-Path $resolvedRoot 'SHA256SUMS.txt'
$hashLines = Get-Content -LiteralPath $hashListPath
if ($hashLines.Count -eq 0) {
    throw 'The SHA-256 manifest is empty.'
}

foreach ($line in $hashLines) {
    if ($line -notmatch '^([0-9A-Fa-f]{64}) \*(.+)$') {
        throw "Invalid SHA-256 manifest entry: $line"
    }
    $expectedHash = $Matches[1].ToUpperInvariant()
    $relativePath = $Matches[2]
    if ([System.IO.Path]::IsPathRooted($relativePath) -or $relativePath.Contains('..')) {
        throw "The SHA-256 manifest contains an unsafe path: $relativePath"
    }
    $fullPath = Join-Path $resolvedRoot $relativePath
    if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
        throw "A SHA-256 target file is missing: $relativePath"
    }
    $actualHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $fullPath).Hash
    if ($actualHash -ne $expectedHash) {
        throw "SHA-256 mismatch: $relativePath"
    }
}

if ($Launch) {
    $process = Start-Process -FilePath $executablePath -WorkingDirectory $resolvedRoot -PassThru
    Start-Sleep -Seconds 3
    $process.Refresh()
    if ($process.HasExited) {
        throw "The smoke-test process exited too early. ExitCode=$($process.ExitCode)"
    }
    if (-not $process.Responding) {
        throw 'The smoke-test window is not responding.'
    }
    [void]$process.CloseMainWindow()
    if (-not $process.WaitForExit(3000)) {
        Stop-Process -Id $process.Id -Force
    }
}

Write-Output "PREFLIGHT PASS: $resolvedRoot"
Write-Output 'Windows x64, required files, PE architecture, and SHA-256 integrity verified.'
