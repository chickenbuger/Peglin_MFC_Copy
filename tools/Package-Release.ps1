[CmdletBinding()]
param(
    [Parameter()]
    [ValidatePattern('^\d+\.\d+$')]
    [string]$Version = '4.3',

    [Parameter()]
    [string]$OutputRoot = 'dist',

    [Parameter()]
    [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$resolvedOutputRoot = if ([System.IO.Path]::IsPathRooted($OutputRoot)) {
    [System.IO.Path]::GetFullPath($OutputRoot)
}
else {
    [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot $OutputRoot))
}

$repositoryPrefix = $repositoryRoot.TrimEnd([System.IO.Path]::DirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
$outputPrefix = $resolvedOutputRoot.TrimEnd([System.IO.Path]::DirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
if ($resolvedOutputRoot -eq $repositoryRoot -or -not $resolvedOutputRoot.StartsWith($repositoryPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'OutputRoot는 저장소 내부의 전용 하위 폴더여야 합니다.'
}

$packageName = "PeglinMFC-$Version-win-x64"
$packageDirectory = [System.IO.Path]::GetFullPath((Join-Path $resolvedOutputRoot $packageName))
$zipPath = [System.IO.Path]::GetFullPath((Join-Path $resolvedOutputRoot "$packageName.zip"))
if (-not $packageDirectory.StartsWith($outputPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw '패키지 경로가 OutputRoot를 벗어났습니다.'
}

$vswherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswherePath -PathType Leaf)) {
    throw 'vswhere.exe를 찾지 못했습니다. Visual Studio 2022 Build Tools가 필요합니다.'
}
$visualStudioPath = (& $vswherePath -latest -products '*' -requires Microsoft.Component.MSBuild -property installationPath).Trim()
if ([string]::IsNullOrWhiteSpace($visualStudioPath)) {
    throw 'MSBuild가 설치된 Visual Studio 2022 인스턴스를 찾지 못했습니다.'
}

$msbuildPath = Join-Path $visualStudioPath 'MSBuild\Current\Bin\MSBuild.exe'
if (-not $SkipBuild) {
    & $msbuildPath (Join-Path $repositoryRoot 'FinalProject_Peglin.sln') /m /t:Build /p:Configuration=Release /p:Platform=x64 /nologo /v:minimal
    if ($LASTEXITCODE -ne 0) {
        throw "Release x64 빌드가 실패했습니다. ExitCode=$LASTEXITCODE"
    }
}

$executablePath = Join-Path $repositoryRoot 'x64\Release\FinalProject_Peglin.exe'
if (-not (Test-Path -LiteralPath $executablePath -PathType Leaf)) {
    throw 'Release x64 실행 파일을 찾지 못했습니다.'
}

$redistRoot = Join-Path $visualStudioPath 'VC\Redist\MSVC'
$redistCandidates = Get-ChildItem -LiteralPath $redistRoot -Directory | ForEach-Object {
    $parsedVersion = $null
    if ([Version]::TryParse($_.Name, [ref]$parsedVersion)) {
        [pscustomobject]@{ Path = $_.FullName; Version = $parsedVersion }
    }
} | Sort-Object Version -Descending

$redistDirectory = $null
foreach ($candidate in $redistCandidates) {
    $crtDirectory = Join-Path $candidate.Path 'x64\Microsoft.VC143.CRT'
    $mfcDirectory = Join-Path $candidate.Path 'x64\Microsoft.VC143.MFC'
    if ((Test-Path -LiteralPath (Join-Path $crtDirectory 'msvcp140.dll')) -and
        (Test-Path -LiteralPath (Join-Path $mfcDirectory 'mfc140u.dll'))) {
        $redistDirectory = $candidate.Path
        break
    }
}
if ($null -eq $redistDirectory) {
    throw 'Visual C++ v143 x64 CRT/MFC 재배포 파일을 찾지 못했습니다.'
}

New-Item -ItemType Directory -Path $resolvedOutputRoot -Force | Out-Null
if (Test-Path -LiteralPath $packageDirectory) {
    Remove-Item -LiteralPath $packageDirectory -Recurse -Force
}
if (Test-Path -LiteralPath $zipPath) {
    Remove-Item -LiteralPath $zipPath -Force
}
New-Item -ItemType Directory -Path $packageDirectory | Out-Null

Copy-Item -LiteralPath $executablePath -Destination (Join-Path $packageDirectory 'FinalProject_Peglin.exe')
$runtimeSources = @{
    'mfc140u.dll' = Join-Path $redistDirectory 'x64\Microsoft.VC143.MFC\mfc140u.dll'
    'msvcp140.dll' = Join-Path $redistDirectory 'x64\Microsoft.VC143.CRT\msvcp140.dll'
    'vcruntime140.dll' = Join-Path $redistDirectory 'x64\Microsoft.VC143.CRT\vcruntime140.dll'
    'vcruntime140_1.dll' = Join-Path $redistDirectory 'x64\Microsoft.VC143.CRT\vcruntime140_1.dll'
}
foreach ($runtime in $runtimeSources.GetEnumerator()) {
    if (-not (Test-Path -LiteralPath $runtime.Value -PathType Leaf)) {
        throw "재배포 파일이 없습니다: $($runtime.Value)"
    }
    Copy-Item -LiteralPath $runtime.Value -Destination (Join-Path $packageDirectory $runtime.Key)
}

Copy-Item -LiteralPath (Join-Path $repositoryRoot 'distribution\Preflight.ps1') -Destination $packageDirectory
Copy-Item -LiteralPath (Join-Path $repositoryRoot 'distribution\README.txt') -Destination $packageDirectory
Set-Content -LiteralPath (Join-Path $packageDirectory 'PACKAGE_VERSION.txt') -Value $Version -Encoding utf8NoBOM

$hashTargets = Get-ChildItem -LiteralPath $packageDirectory -File | Where-Object Name -ne 'SHA256SUMS.txt' | Sort-Object Name
$hashLines = foreach ($file in $hashTargets) {
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $file.FullName).Hash
    "$hash *$($file.Name)"
}
Set-Content -LiteralPath (Join-Path $packageDirectory 'SHA256SUMS.txt') -Value $hashLines -Encoding ascii

& (Join-Path $packageDirectory 'Preflight.ps1') -PackageRoot $packageDirectory
Compress-Archive -Path (Join-Path $packageDirectory '*') -DestinationPath $zipPath -CompressionLevel Optimal

Write-Output "PACKAGE PASS: $packageDirectory"
Write-Output "ZIP: $zipPath"
Write-Output "REDIST: $redistDirectory"
