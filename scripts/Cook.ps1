param(
  [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8'
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$project = Join-Path $repositoryRoot 'SampleProject\CookScopeSample.uproject'
$runUat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
if (-not (Test-Path -LiteralPath $runUat -PathType Leaf)) {
  throw "RunUAT.bat not found: $runUat"
}
if (-not (Test-Path -LiteralPath $project -PathType Leaf)) {
  throw "Sample Project not found: $project"
}

$evidenceRoot = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot 'Artifacts\Evidence\PACT-40\Cook'))
New-Item -ItemType Directory -Path $evidenceRoot -Force | Out-Null
$logPath = Join-Path $evidenceRoot ("cook-" + [guid]::NewGuid().ToString('N') + '.log')
$arguments = @(
  'BuildCookRun',
  "-project=$project",
  '-noP4',
  '-platform=Win64',
  '-clientconfig=Development',
  '-clean',
  '-build',
  '-cook',
  '-skipstage',
  '-unattended',
  '-utf8output'
)
& $runUat @arguments 2>&1 | Tee-Object -FilePath $logPath
if ($LASTEXITCODE -ne 0) {
  throw "Sample Project Cook failed with exit code $LASTEXITCODE; log: $logPath"
}
Write-Output "Cook log: $logPath"
