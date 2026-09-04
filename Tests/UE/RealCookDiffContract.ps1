param(
  [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8'
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$project = Join-Path $repositoryRoot 'SampleProject\CookScopeSample.uproject'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$config = Join-Path $repositoryRoot 'Plugins\CookScope\Config\CookScopeRules.json'
$baseline = Join-Path $repositoryRoot 'Examples\snapshots\cookscope-real-baseline.json'
$cookRegistry = Join-Path $repositoryRoot 'SampleProject\Saved\Cooked\Windows\CookScopeSample\Metadata\DevelopmentAssetRegistry.bin'
$sourceSha = (& git -C $repositoryRoot rev-parse HEAD).Trim()
$evidenceBase = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot 'Artifacts\Evidence\PACT-40\RealDiff'))
$runRoot = [System.IO.Path]::GetFullPath((Join-Path $evidenceBase ("run-" + [guid]::NewGuid().ToString('N'))))
if (-not $runRoot.StartsWith($evidenceBase, [System.StringComparison]::OrdinalIgnoreCase)) {
  throw "Refusing unsafe real-diff evidence path: $runRoot"
}
New-Item -ItemType Directory -Path $runRoot -Force | Out-Null
$output = Join-Path $runRoot 'reports'
$log = Join-Path $runRoot 'commandlet.log'
$arguments = @(
  $project,
  '-run=CookScopeAudit',
  '-unattended',
  '-nop4',
  '-nosplash',
  '-nullrhi',
  '-nosound',
  '-stdout',
  '-FullStdOutLogOutput',
  "-config=$config",
  "-output=$output",
  "-source-sha=$sourceSha",
  '-scope=/Game/CookScopeFixtures',
  "-baseline=$baseline",
  "-cook-registry=$cookRegistry",
  '-cook-platform=Windows',
  '-cook-configuration=Development',
  '-fail-on-violation=false',
  '-timeout-seconds=120'
)
& $editorCmd @arguments *> $log
if ($LASTEXITCODE -ne 0) {
  throw "Real Cook diff Commandlet failed with exit code $LASTEXITCODE; log: $log"
}

$report = Get-Content -LiteralPath (Join-Path $output 'cookscope.json') -Raw | ConvertFrom-Json
$snapshot = Get-Content -LiteralPath (Join-Path $output 'cookscope.snapshot.json') -Raw | ConvertFrom-Json
$baselineDocument = Get-Content -LiteralPath $baseline -Raw | ConvertFrom-Json
$added = @($report.diff.assetChanges | Where-Object {
  $_.kind -eq 'added' -and $_.candidatePath -eq '/Game/CookScopeFixtures/Primary/DA_Candidate.DA_Candidate'
})
$candidate = @($snapshot.assets | Where-Object objectPath -eq '/Game/CookScopeFixtures/Primary/DA_Candidate.DA_Candidate')
if (-not $report.diff.comparable -or $added.Count -ne 1 -or $candidate.Count -ne 1 -or
    @($report.diff.assetChanges).Count -ne 1 -or
    $candidate[0].cookedSize.kind -ne 'actual-cooked' -or [int64]$candidate[0].cookedSize.bytes -le 0) {
  throw 'Real Cook diff does not contain one Added actual-cooked DA_Candidate asset'
}

[pscustomobject]@{
  Result = 'PASS'
  EvidenceRoot = $runRoot
  SourceSha = $sourceSha
  BaselineSha = $baselineDocument.provenance.sourceSha
  CandidateAssets = @($snapshot.assets).Count
  CandidateActualCookedAssets = @($snapshot.assets | Where-Object { $_.cookedSize.kind -eq 'actual-cooked' }).Count
  AddedCandidateCookedBytes = [int64]$candidate[0].cookedSize.bytes
  DiffAssetChanges = @($report.diff.assetChanges).Count
} | Format-List
