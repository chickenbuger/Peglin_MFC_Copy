[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$ZipPath,

    [Parameter(Mandatory)]
    [ValidatePattern('^\d+\.\d+$')]
    [string]$ExpectedVersion
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$resolvedZipPath = [System.IO.Path]::GetFullPath($ZipPath)
if (-not (Test-Path -LiteralPath $resolvedZipPath -PathType Leaf)) {
    throw "ZIP 파일이 없습니다: $resolvedZipPath"
}

$temporaryBase = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath())
$temporaryPrefix = $temporaryBase.TrimEnd([System.IO.Path]::DirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
$temporaryRoot = [System.IO.Path]::GetFullPath((Join-Path $temporaryBase ("PeglinMFC-PackageTests-" + [Guid]::NewGuid().ToString('N'))))
if (-not $temporaryRoot.StartsWith($temporaryPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw '임시 검증 경로가 시스템 임시 폴더를 벗어났습니다.'
}

function Assert-PreflightFailure {
    param(
        [Parameter(Mandatory)][string]$PackageRoot,
        [Parameter(Mandatory)][string]$Scenario
    )

    $failedAsExpected = $false
    try {
        & (Join-Path $PackageRoot 'Preflight.ps1') -PackageRoot $PackageRoot | Out-Null
    }
    catch {
        $failedAsExpected = $true
    }
    if (-not $failedAsExpected) {
        throw "사전 검사가 실패를 탐지하지 못했습니다: $Scenario"
    }
}

try {
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::OpenRead($resolvedZipPath)
    try {
        foreach ($entry in $archive.Entries) {
            $normalized = $entry.FullName.Replace('/', [System.IO.Path]::DirectorySeparatorChar)
            if ([System.IO.Path]::IsPathRooted($normalized) -or $normalized.Split([System.IO.Path]::DirectorySeparatorChar).Contains('..')) {
                throw "ZIP에 안전하지 않은 경로가 있습니다: $($entry.FullName)"
            }
        }
    }
    finally {
        $archive.Dispose()
    }

    $originalRoot = Join-Path $temporaryRoot 'original'
    $tamperedRoot = Join-Path $temporaryRoot 'tampered'
    $missingRoot = Join-Path $temporaryRoot 'missing'
    New-Item -ItemType Directory -Path $originalRoot -Force | Out-Null
    Expand-Archive -LiteralPath $resolvedZipPath -DestinationPath $originalRoot

    $actualVersion = (Get-Content -Raw -LiteralPath (Join-Path $originalRoot 'PACKAGE_VERSION.txt')).Trim()
    if ($actualVersion -ne $ExpectedVersion) {
        throw "패키지 버전이 다릅니다. Expected=$ExpectedVersion Actual=$actualVersion"
    }
    & (Join-Path $originalRoot 'Preflight.ps1') -PackageRoot $originalRoot | Out-Null

    New-Item -ItemType Directory -Path $tamperedRoot | Out-Null
    Copy-Item -Path (Join-Path $originalRoot '*') -Destination $tamperedRoot -Recurse
    Add-Content -LiteralPath (Join-Path $tamperedRoot 'README.txt') -Value 'tampered'
    Assert-PreflightFailure -PackageRoot $tamperedRoot -Scenario 'SHA-256 변조'

    New-Item -ItemType Directory -Path $missingRoot | Out-Null
    Copy-Item -Path (Join-Path $originalRoot '*') -Destination $missingRoot -Recurse
    Remove-Item -LiteralPath (Join-Path $missingRoot 'mfc140u.dll') -Force
    Assert-PreflightFailure -PackageRoot $missingRoot -Scenario '필수 MFC 런타임 누락'

    Write-Output "PACKAGE TEST PASS: $resolvedZipPath"
    Write-Output 'ZIP 경로 안전성, 버전, 정상 사전 검사, 변조 및 필수 파일 누락 탐지 완료'
}
finally {
    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}
