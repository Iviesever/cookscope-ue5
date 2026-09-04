param(
  [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
  [string]$OutputPath = ''
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if ([string]::IsNullOrWhiteSpace($OutputPath)) {
  $OutputPath = Join-Path $repositoryRoot 'docs\images\editor.png'
}
$OutputPath = [System.IO.Path]::GetFullPath($OutputPath)
$project = Join-Path $repositoryRoot 'SampleProject\CookScopeSample.uproject'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$evidenceRoot = Join-Path $repositoryRoot 'Artifacts\Evidence\PACT-60\Editor'
$logPath = Join-Path $evidenceRoot 'screenshot-current.log'
New-Item -ItemType Directory -Path $evidenceRoot -Force | Out-Null

& (Join-Path $PSScriptRoot 'Build-Unreal.ps1') -EngineRoot $EngineRoot

$arguments = @(
  $project,
  '-unattended',
  '-nop4',
  '-nosplash',
  '-nosound',
  '-RenderOffscreen',
  '-stdout',
  '-FullStdOutLogOutput',
  '-CookScopeScreenshotPath=' + $OutputPath,
  '-ExecCmds=Automation RunTests CookScope.PACT00.EditorAndCommandletContracts;Quit',
  '-TestExit=Automation Test Queue Empty'
)
& $editorCmd @arguments *> $logPath
if ($LASTEXITCODE -ne 0) {
  throw "UE screenshot process failed with exit code $LASTEXITCODE; log: $logPath"
}

$logText = [System.IO.File]::ReadAllText($logPath)
if ($logText -notmatch 'Test Completed\. Result=\{Success\}.*CookScope\.PACT00\.EditorAndCommandletContracts' -or
    $logText -notmatch '\*\*\*\* TEST COMPLETE\. EXIT CODE: 0 \*\*\*\*') {
  throw "CookScope screenshot Automation success markers were not found; log: $logPath"
}
if (-not (Test-Path -LiteralPath $OutputPath -PathType Leaf)) {
  throw "CookScope panel screenshot was not written: $OutputPath"
}
$imageInfo = Get-Item -LiteralPath $OutputPath
if ($imageInfo.Length -lt 1024) {
  throw "CookScope panel screenshot is unexpectedly small: $($imageInfo.Length) bytes"
}

Write-Output "PASS: real CookScope Slate panel screenshot written to $OutputPath ($($imageInfo.Length) bytes)"
