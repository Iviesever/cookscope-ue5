param(
  [string]$InputHtml = '',
  [string]$DesktopOutput = '',
  [string]$NarrowOutput = '',
  [string]$ChromePath = 'C:\Program Files\Google\Chrome\Application\chrome.exe'
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if ([string]::IsNullOrWhiteSpace($InputHtml)) {
  $InputHtml = Join-Path $repositoryRoot 'Examples\reports\cookscope-sample.html'
}
if ([string]::IsNullOrWhiteSpace($DesktopOutput)) {
  $DesktopOutput = Join-Path $repositoryRoot 'docs\images\report.png'
}
if ([string]::IsNullOrWhiteSpace($NarrowOutput)) {
  $NarrowOutput = Join-Path $repositoryRoot 'Artifacts\Evidence\PACT-50\Browser\report-narrow.png'
}
foreach ($path in @($InputHtml, $DesktopOutput, $NarrowOutput)) {
  $resolvedParent = [System.IO.Path]::GetFullPath((Split-Path -Parent $path))
  New-Item -ItemType Directory -Force -Path $resolvedParent | Out-Null
}
$InputHtml = [System.IO.Path]::GetFullPath($InputHtml)
$DesktopOutput = [System.IO.Path]::GetFullPath($DesktopOutput)
$NarrowOutput = [System.IO.Path]::GetFullPath($NarrowOutput)
if (-not (Test-Path -LiteralPath $InputHtml -PathType Leaf)) {
  throw "HTML report does not exist: $InputHtml"
}
if (-not (Test-Path -LiteralPath $ChromePath -PathType Leaf)) {
  throw "Chrome does not exist: $ChromePath"
}

$reportUrl = ([System.Uri]$InputHtml).AbsoluteUri
function Invoke-Capture([string]$OutputPath, [string]$WindowSize, [string]$ProfileName) {
  $profile = Join-Path $repositoryRoot "Artifacts\BrowserProfiles\$ProfileName-$([guid]::NewGuid().ToString('N'))"
  New-Item -ItemType Directory -Force -Path $profile | Out-Null
  $arguments = @(
    '--headless=new',
    '--disable-gpu',
    '--hide-scrollbars',
    '--run-all-compositor-stages-before-draw',
    '--virtual-time-budget=2000',
    "--user-data-dir=$profile",
    "--window-size=$WindowSize",
    "--screenshot=$OutputPath",
    $reportUrl
  )
  $process = Start-Process -FilePath $ChromePath -ArgumentList $arguments -Wait -PassThru -WindowStyle Hidden
  if ($process.ExitCode -ne 0) {
    throw "Chrome screenshot failed with exit code $($process.ExitCode)"
  }
  if (-not (Test-Path -LiteralPath $OutputPath -PathType Leaf)) {
    throw "Chrome did not write screenshot: $OutputPath"
  }
}

Invoke-Capture -OutputPath $DesktopOutput -WindowSize '1440,1200' -ProfileName 'report-desktop'
Invoke-Capture -OutputPath $NarrowOutput -WindowSize '390,844' -ProfileName 'report-narrow'

$desktop = Get-Item -LiteralPath $DesktopOutput
$narrow = Get-Item -LiteralPath $NarrowOutput
Write-Output "PASS: HTML report screenshots written; desktop=$($desktop.Length) bytes, narrow=$($narrow.Length) bytes"
