param(
  [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8'
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$project = Join-Path $repositoryRoot 'SampleProject\CookScopeSample.uproject'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$evidenceRoot = Join-Path $repositoryRoot 'Artifacts\Evidence\PACT-20\Fixtures'
New-Item -ItemType Directory -Path $evidenceRoot -Force | Out-Null
$logPath = Join-Path $evidenceRoot 'fixture-builder.log'

$arguments = @(
  $project,
  '-run=CookScopeFixtureBuilder',
  '-unattended',
  '-nop4',
  '-nosplash',
  '-nullrhi',
  '-nosound',
  '-stdout',
  '-FullStdOutLogOutput'
)
& $editorCmd @arguments *> $logPath
if ($LASTEXITCODE -ne 0) {
  throw "Fixture builder commandlet failed with exit code $LASTEXITCODE; log: $logPath"
}

$expected = @(
  'Targets\DA_Target.uasset',
  'Sources\DA_Hard.uasset',
  'Sources\DA_Soft.uasset',
  'Sources\DA_Searchable.uasset',
  'Naming\BadName.uasset',
  'Primary\DA_Primary.uasset',
  'Primary\DA_Candidate.uasset',
  'Cycle\DA_CycleA.uasset',
  'Cycle\DA_CycleB.uasset'
)
$fixtureRoot = Join-Path $repositoryRoot 'SampleProject\Content\CookScopeFixtures'
foreach ($relativePath in $expected) {
  $assetPath = Join-Path $fixtureRoot $relativePath
  if (-not (Test-Path -LiteralPath $assetPath -PathType Leaf)) {
    throw "Fixture builder did not create expected asset: $assetPath"
  }
}

$resourceExpected = @(
  'Textures\T_Resource.uasset',
  'Meshes\SM_Resource.uasset',
  'Meshes\SK_Resource.uasset',
  'Audio\S_Resource.uasset'
)
$resourceRoot = Join-Path $repositoryRoot 'SampleProject\Content\CookScopeResourceFixtures'
foreach ($relativePath in $resourceExpected) {
  $assetPath = Join-Path $resourceRoot $relativePath
  if (-not (Test-Path -LiteralPath $assetPath -PathType Leaf)) {
    throw "Fixture builder did not create expected resource asset: $assetPath"
  }
}

Write-Output "PASS: generated $($expected.Count + $resourceExpected.Count) deterministic CookScope fixture assets"
