[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [ValidateSet('x64', 'x86')]
    [string]$Platform = 'x64',
    [ValidateRange(5, 3600)]
    [int]$DurationSeconds = 120,
    [ValidateRange(1, 60)]
    [int]$SampleIntervalSeconds = 5,
    [string]$ReportPath = ''
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($ReportPath)) {
    $ReportPath = Join-Path $projectRoot 'DEV_LOG\Performance_Report_8.7.md'
}
if ($Platform -eq 'x64') {
    $executable = Join-Path $projectRoot ("x64\{0}\FinalProject_Peglin.exe" -f $Configuration)
} else {
    $executable = Join-Path $projectRoot ("{0}\FinalProject_Peglin.exe" -f $Configuration)
}
if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
    throw "Long-run executable is missing. Build $Configuration $Platform first: $executable"
}

if (-not ('PeglinGuiResources' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class PeglinGuiResources {
    [DllImport("user32.dll")]
    public static extern uint GetGuiResources(IntPtr process, uint flags);
}
'@
}

$samples = [System.Collections.Generic.List[object]]::new()
$process = Start-Process -FilePath $executable -ArgumentList '--demo' -WindowStyle Hidden -PassThru
$started = Get-Date
try {
    Start-Sleep -Seconds 2
    while (((Get-Date) - $started).TotalSeconds -lt $DurationSeconds -and -not $process.HasExited) {
        $process.Refresh()
        $samples.Add([PSCustomObject]@{
            Elapsed = [math]::Round(((Get-Date) - $started).TotalSeconds, 1)
            Gdi = [PeglinGuiResources]::GetGuiResources($process.Handle, 0)
            User = [PeglinGuiResources]::GetGuiResources($process.Handle, 1)
            Handles = $process.HandleCount
            WorkingSetMiB = [math]::Round($process.WorkingSet64 / 1MB, 1)
            PrivateMiB = [math]::Round($process.PrivateMemorySize64 / 1MB, 1)
        })
        Start-Sleep -Seconds $SampleIntervalSeconds
    }
} finally {
    if (-not $process.HasExited) {
        [void]$process.CloseMainWindow()
        if (-not $process.WaitForExit(2000)) {
            Stop-Process -Id $process.Id
        }
    }
}

if ($samples.Count -lt 2) {
    throw 'Long-run probe did not collect enough samples.'
}
$first = $samples[0]
$last = $samples[$samples.Count - 1]
$gdiGrowth = [int]$last.Gdi - [int]$first.Gdi
$userGrowth = [int]$last.User - [int]$first.User
$handleGrowth = [int]$last.Handles - [int]$first.Handles
$passed = $gdiGrowth -le 5 -and $userGrowth -le 5 -and $handleGrowth -le 10

$reportTitle = [System.IO.Path]::GetFileNameWithoutExtension($ReportPath).Replace('_', ' ')
$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("# $reportTitle")
$lines.Add('')
$lines.Add("- Configuration: $Configuration $Platform")
$lines.Add("- Requested duration: $DurationSeconds seconds")
$lines.Add("- Samples: $($samples.Count)")
$lines.Add("- Result: **$(if ($passed) { 'PASS' } else { 'FAIL' })**")
$lines.Add("- Growth: GDI $gdiGrowth, USER $userGrowth, handles $handleGrowth")
$lines.Add('')
$lines.Add('| Elapsed (s) | GDI | USER | Handles | Working set (MiB) | Private (MiB) |')
$lines.Add('| ---: | ---: | ---: | ---: | ---: | ---: |')
foreach ($sample in $samples) {
    $lines.Add("| $($sample.Elapsed) | $($sample.Gdi) | $($sample.User) | $($sample.Handles) | $($sample.WorkingSetMiB) | $($sample.PrivateMiB) |")
}
$lines.Add('')
$lines.Add('PASS 기준은 첫 표본 대비 마지막 표본의 GDI/USER 증가가 각각 5개 이하이고 전체 핸들 증가가 10개 이하인 것이다.')
$reportDirectory = Split-Path -Parent $ReportPath
[void](New-Item -ItemType Directory -Path $reportDirectory -Force)
Set-Content -LiteralPath $ReportPath -Value $lines -Encoding utf8

[PSCustomObject]@{
    Passed = $passed
    ReportPath = $ReportPath
    GdiGrowth = $gdiGrowth
    UserGrowth = $userGrowth
    HandleGrowth = $handleGrowth
    Samples = $samples.Count
}
if (-not $passed) { exit 1 }
