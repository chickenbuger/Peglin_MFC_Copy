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
    throw "The ZIP file was not found: $resolvedZipPath"
}

$temporaryBase = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath())
$temporaryPrefix = $temporaryBase.TrimEnd([System.IO.Path]::DirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
$temporaryRoot = [System.IO.Path]::GetFullPath((Join-Path $temporaryBase ("PeglinMFC-PackageTests-" + [Guid]::NewGuid().ToString('N'))))
if (-not $temporaryRoot.StartsWith($temporaryPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'The temporary test path escaped the system temporary directory.'
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
        throw "Preflight did not detect the expected failure: $Scenario"
    }
}

try {
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::OpenRead($resolvedZipPath)
    try {
        foreach ($entry in $archive.Entries) {
            $normalized = $entry.FullName.Replace('/', [System.IO.Path]::DirectorySeparatorChar)
            if ([System.IO.Path]::IsPathRooted($normalized) -or $normalized.Split([System.IO.Path]::DirectorySeparatorChar).Contains('..')) {
                throw "The ZIP contains an unsafe path: $($entry.FullName)"
            }
        }
    }
    finally {
        $archive.Dispose()
    }

    $originalRoot = Join-Path $temporaryRoot 'original'
    $tamperedRoot = Join-Path $temporaryRoot 'tampered'
    $missingRoot = Join-Path $temporaryRoot 'missing'
    $missingContentRoot = Join-Path $temporaryRoot 'missing-content'
    $missingGameplayRoot = Join-Path $temporaryRoot 'missing-gameplay-content'
    New-Item -ItemType Directory -Path $originalRoot -Force | Out-Null
    Expand-Archive -LiteralPath $resolvedZipPath -DestinationPath $originalRoot

    $actualVersion = (Get-Content -Raw -LiteralPath (Join-Path $originalRoot 'PACKAGE_VERSION.txt')).Trim()
    if ($actualVersion -ne $ExpectedVersion) {
        throw "Package version mismatch. Expected=$ExpectedVersion Actual=$actualVersion"
    }
    & (Join-Path $originalRoot 'Preflight.ps1') -PackageRoot $originalRoot | Out-Null

    New-Item -ItemType Directory -Path $tamperedRoot | Out-Null
    Copy-Item -Path (Join-Path $originalRoot '*') -Destination $tamperedRoot -Recurse
    Add-Content -LiteralPath (Join-Path $tamperedRoot 'README.txt') -Value 'tampered'
    Assert-PreflightFailure -PackageRoot $tamperedRoot -Scenario 'SHA-256 tampering'

    New-Item -ItemType Directory -Path $missingRoot | Out-Null
    Copy-Item -Path (Join-Path $originalRoot '*') -Destination $missingRoot -Recurse
    Remove-Item -LiteralPath (Join-Path $missingRoot 'mfc140u.dll') -Force
    Assert-PreflightFailure -PackageRoot $missingRoot -Scenario 'missing required MFC runtime'

    New-Item -ItemType Directory -Path $missingContentRoot | Out-Null
    Copy-Item -Path (Join-Path $originalRoot '*') -Destination $missingContentRoot -Recurse
    Remove-Item -LiteralPath (Join-Path $missingContentRoot 'content\stages.v1.ini') -Force
    Assert-PreflightFailure -PackageRoot $missingContentRoot -Scenario 'missing versioned stage content'

    New-Item -ItemType Directory -Path $missingGameplayRoot | Out-Null
    Copy-Item -Path (Join-Path $originalRoot '*') -Destination $missingGameplayRoot -Recurse
    Remove-Item -LiteralPath (Join-Path $missingGameplayRoot 'content\gameplay.v1.ini') -Force
    Assert-PreflightFailure -PackageRoot $missingGameplayRoot -Scenario 'missing gameplay definition content'

    Write-Output "PACKAGE TEST PASS: $resolvedZipPath"
    Write-Output 'ZIP path safety, version, normal preflight, tampering, missing-runtime, and both missing-content checks completed.'
}
finally {
    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}
