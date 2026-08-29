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
            throw "PE 파일이 아닙니다: $Path"
        }
        $stream.Position = 0x3C
        $peOffset = $reader.ReadInt32()
        if ($peOffset -lt 0 -or $peOffset -gt ($stream.Length - 6)) {
            throw "PE 헤더 위치가 잘못됐습니다: $Path"
        }
        $stream.Position = $peOffset
        if ($reader.ReadUInt32() -ne 0x00004550) {
            throw "PE 서명이 잘못됐습니다: $Path"
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
    'SHA256SUMS.txt'
)

if (-not [Environment]::Is64BitOperatingSystem) {
    throw '이 패키지는 Windows x64 운영체제가 필요합니다.'
}

foreach ($relativePath in $requiredFiles) {
    $fullPath = Join-Path $resolvedRoot $relativePath
    if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
        throw "필수 파일이 없습니다: $relativePath"
    }
}

$executablePath = Join-Path $resolvedRoot 'FinalProject_Peglin.exe'
if ((Get-PeMachine -Path $executablePath) -ne 0x8664) {
    throw '실행 파일이 x64 PE가 아닙니다.'
}

$hashListPath = Join-Path $resolvedRoot 'SHA256SUMS.txt'
$hashLines = Get-Content -LiteralPath $hashListPath
if ($hashLines.Count -eq 0) {
    throw 'SHA-256 목록이 비어 있습니다.'
}

foreach ($line in $hashLines) {
    if ($line -notmatch '^([0-9A-Fa-f]{64}) \*(.+)$') {
        throw "SHA-256 목록 형식이 잘못됐습니다: $line"
    }
    $expectedHash = $Matches[1].ToUpperInvariant()
    $relativePath = $Matches[2]
    if ([System.IO.Path]::IsPathRooted($relativePath) -or $relativePath.Contains('..')) {
        throw "SHA-256 목록에 안전하지 않은 경로가 있습니다: $relativePath"
    }
    $fullPath = Join-Path $resolvedRoot $relativePath
    if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
        throw "SHA-256 대상 파일이 없습니다: $relativePath"
    }
    $actualHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $fullPath).Hash
    if ($actualHash -ne $expectedHash) {
        throw "SHA-256 불일치: $relativePath"
    }
}

if ($Launch) {
    $process = Start-Process -FilePath $executablePath -WorkingDirectory $resolvedRoot -PassThru
    Start-Sleep -Seconds 3
    $process.Refresh()
    if ($process.HasExited) {
        throw "스모크 실행이 너무 일찍 종료됐습니다. ExitCode=$($process.ExitCode)"
    }
    if (-not $process.Responding) {
        throw '스모크 실행 중 창이 응답하지 않습니다.'
    }
    [void]$process.CloseMainWindow()
    if (-not $process.WaitForExit(3000)) {
        Stop-Process -Id $process.Id -Force
    }
}

Write-Output "PREFLIGHT PASS: $resolvedRoot"
Write-Output 'Windows x64, 필수 파일, PE 아키텍처와 SHA-256 무결성 검증 완료'
