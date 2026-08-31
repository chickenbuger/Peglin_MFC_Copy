[CmdletBinding()]
param(
    [Parameter()]
    [string]$ManifestPath = 'FinalProject_Peglin\res\assets.v1.json',

    [Parameter()]
    [switch]$Rebuild,

    [Parameter()]
    [switch]$VerifyConversion
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$assetRepositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$assetRepositoryPrefix = $assetRepositoryRoot.TrimEnd([System.IO.Path]::DirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
$assetManifestFullPath = if ([System.IO.Path]::IsPathRooted($ManifestPath)) {
    [System.IO.Path]::GetFullPath($ManifestPath)
}
else {
    [System.IO.Path]::GetFullPath((Join-Path $assetRepositoryRoot $ManifestPath))
}

if (-not $assetManifestFullPath.StartsWith($assetRepositoryPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'The asset manifest must be inside the repository.'
}
if (-not (Test-Path -LiteralPath $assetManifestFullPath -PathType Leaf)) {
    throw "The asset manifest was not found: $assetManifestFullPath"
}

Add-Type -AssemblyName System.Drawing

function Write-AssetBitmap {
    param(
        [Parameter(Mandatory)]$SourceImage,
        [Parameter(Mandatory)]$Entry,
        [Parameter(Mandatory)][string]$OutputPath
    )

    $convertedBitmap = [System.Drawing.Bitmap]::new(
        [int]$Entry.width,
        [int]$Entry.height,
        [System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
    try {
        $convertedGraphics = [System.Drawing.Graphics]::FromImage($convertedBitmap)
        try {
            $convertedGraphics.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceOver
            $convertedGraphics.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
            $convertedGraphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
            $convertedGraphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
            $convertedGraphics.Clear($(if ($Entry.transparent) {
                [System.Drawing.Color]::Magenta
            }
            else {
                [System.Drawing.Color]::Black
            }))
            $convertedGraphics.DrawImage(
                $SourceImage,
                [System.Drawing.Rectangle]::new(0, 0, [int]$Entry.width, [int]$Entry.height))
        }
        finally {
            $convertedGraphics.Dispose()
        }
        $convertedBitmap.Save($OutputPath, [System.Drawing.Imaging.ImageFormat]::Bmp)
    }
    finally {
        $convertedBitmap.Dispose()
    }
}

$assetManifest = Get-Content -Raw -Encoding UTF8 -LiteralPath $assetManifestFullPath | ConvertFrom-Json
if ($assetManifest.version -ne 1 -or $null -eq $assetManifest.assets -or $assetManifest.assets.Count -eq 0) {
    throw 'The asset manifest version or asset list is invalid.'
}

$assetResourceCache = @{}
$assetValidatedCount = 0
$assetRebuiltCount = 0
$assetConversionCheckCount = 0
foreach ($assetEntry in $assetManifest.assets) {
    if ([string]::IsNullOrWhiteSpace($assetEntry.output) -or
        [string]::IsNullOrWhiteSpace($assetEntry.resourceFile) -or
        [string]::IsNullOrWhiteSpace($assetEntry.resourceId) -or
        $assetEntry.width -le 0 -or $assetEntry.height -le 0) {
        throw 'An asset manifest entry is incomplete.'
    }

    $assetOutputPath = [System.IO.Path]::GetFullPath((Join-Path $assetRepositoryRoot $assetEntry.output))
    $assetResourcePath = [System.IO.Path]::GetFullPath((Join-Path $assetRepositoryRoot $assetEntry.resourceFile))
    if (-not $assetOutputPath.StartsWith($assetRepositoryPrefix, [StringComparison]::OrdinalIgnoreCase) -or
        -not $assetResourcePath.StartsWith($assetRepositoryPrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "An asset path escaped the repository: $($assetEntry.output)"
    }

    $assetHasSource = -not [string]::IsNullOrWhiteSpace([string]$assetEntry.source)
    if ($assetHasSource) {
        $assetSourcePath = [System.IO.Path]::GetFullPath((Join-Path $assetRepositoryRoot $assetEntry.source))
        if (-not $assetSourcePath.StartsWith($assetRepositoryPrefix, [StringComparison]::OrdinalIgnoreCase) -or
            -not (Test-Path -LiteralPath $assetSourcePath -PathType Leaf)) {
            throw "An asset source is missing or unsafe: $($assetEntry.source)"
        }

        $assetSourceImage = [System.Drawing.Image]::FromFile($assetSourcePath)
        try {
            if ($assetSourceImage.Width -le 0 -or $assetSourceImage.Height -le 0) {
                throw "An asset source has invalid dimensions: $($assetEntry.source)"
            }
            if ($Rebuild) {
                Write-AssetBitmap -SourceImage $assetSourceImage -Entry $assetEntry -OutputPath $assetOutputPath
                ++$assetRebuiltCount
            }
            if ($VerifyConversion) {
                $assetVerificationPath = Join-Path (
                    [System.IO.Path]::GetTempPath()) (
                    'PeglinAsset-' + [Guid]::NewGuid().ToString('N') + '.bmp')
                try {
                    Write-AssetBitmap -SourceImage $assetSourceImage -Entry $assetEntry -OutputPath $assetVerificationPath
                    $assetVerificationImage = [System.Drawing.Image]::FromFile($assetVerificationPath)
                    try {
                        if ($assetVerificationImage.Width -ne [int]$assetEntry.width -or
                            $assetVerificationImage.Height -ne [int]$assetEntry.height -or
                            $assetVerificationImage.PixelFormat -ne [System.Drawing.Imaging.PixelFormat]::Format24bppRgb) {
                            throw "The conversion verification produced an invalid BMP: $($assetEntry.source)"
                        }
                    }
                    finally {
                        $assetVerificationImage.Dispose()
                    }
                    ++$assetConversionCheckCount
                }
                finally {
                    if (Test-Path -LiteralPath $assetVerificationPath) {
                        Remove-Item -LiteralPath $assetVerificationPath -Force
                    }
                }
            }
        }
        finally {
            $assetSourceImage.Dispose()
        }
    }

    if (-not (Test-Path -LiteralPath $assetOutputPath -PathType Leaf)) {
        throw "A runtime BMP is missing: $($assetEntry.output)"
    }
    $assetRuntimeImage = [System.Drawing.Image]::FromFile($assetOutputPath)
    try {
        if ($assetRuntimeImage.Width -ne [int]$assetEntry.width -or
            $assetRuntimeImage.Height -ne [int]$assetEntry.height -or
            $assetRuntimeImage.PixelFormat -ne [System.Drawing.Imaging.PixelFormat]::Format24bppRgb) {
            throw "A runtime BMP has the wrong size or pixel format: $($assetEntry.output)"
        }
    }
    finally {
        $assetRuntimeImage.Dispose()
    }

    if (-not $assetResourceCache.ContainsKey($assetResourcePath)) {
        if (-not (Test-Path -LiteralPath $assetResourcePath -PathType Leaf)) {
            throw "A resource file is missing: $($assetEntry.resourceFile)"
        }
        $assetResourceCache[$assetResourcePath] = Get-Content -Raw -LiteralPath $assetResourcePath
    }
    $assetResourceText = $assetResourceCache[$assetResourcePath]
    $assetOutputFileName = [System.IO.Path]::GetFileName($assetOutputPath)
    if (-not $assetResourceText.Contains([string]$assetEntry.resourceId) -or
        -not $assetResourceText.Contains($assetOutputFileName)) {
        throw "A runtime BMP is not referenced by its expected resource ID: $($assetEntry.resourceId)"
    }
    ++$assetValidatedCount
}

Write-Output "ASSET PIPELINE PASS: validated=$assetValidatedCount rebuilt=$assetRebuiltCount conversion_checks=$assetConversionCheckCount"
Write-Output 'PNG sources, 24-bit BMP dimensions, and Windows resource references are valid.'
