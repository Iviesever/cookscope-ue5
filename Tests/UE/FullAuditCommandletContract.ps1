param(
  [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8'
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$project = Join-Path $repositoryRoot 'SampleProject\CookScopeSample.uproject'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$config = Join-Path $repositoryRoot 'Plugins\CookScope\Config\CookScopeRules.json'
$cookRegistry = Join-Path $repositoryRoot 'SampleProject\Saved\Cooked\Windows\CookScopeSample\Metadata\DevelopmentAssetRegistry.bin'
if (-not (Test-Path -LiteralPath $cookRegistry -PathType Leaf)) {
  throw "Real Cook Development Asset Registry not found: $cookRegistry"
}
$sourceSha = (& git -C $repositoryRoot rev-parse HEAD).Trim()
$evidenceBase = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot 'Artifacts\Evidence\PACT-70\FullAudit'))
$runRoot = [System.IO.Path]::GetFullPath((Join-Path $evidenceBase ("run-" + [guid]::NewGuid().ToString('N'))))
if (-not $runRoot.StartsWith($evidenceBase, [System.StringComparison]::OrdinalIgnoreCase)) {
  throw "Refusing unsafe full-audit evidence path: $runRoot"
}
New-Item -ItemType Directory -Path $runRoot -Force | Out-Null

function Invoke-FullAudit {
  param(
    [Parameter(Mandatory = $true)]
    [string]$Output,
    [Parameter(Mandatory = $true)]
    [bool]$FailOnViolation,
    [Parameter(Mandatory = $true)]
    [string]$LogName
  )
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
    "-output=$Output",
    "-source-sha=$sourceSha",
    '-scope=/Game/CookScopeFixtures',
    "-cook-registry=$cookRegistry",
    '-cook-platform=Windows',
    '-cook-configuration=Development',
    "-fail-on-violation=$($FailOnViolation.ToString().ToLowerInvariant())",
    '-timeout-seconds=120'
  )
  & $editorCmd @arguments *> (Join-Path $runRoot $LogName)
  return $LASTEXITCODE
}

$blockingOutput = Join-Path $runRoot 'blocking'
$blockingExit = Invoke-FullAudit -Output $blockingOutput -FailOnViolation $true -LogName 'blocking.log'
if ($blockingExit -ne 2) {
  throw "Expected full audit violation exit 2, got $blockingExit"
}
$expectedFiles = @('cookscope.json', 'cookscope.sarif', 'cookscope.junit.xml', 'cookscope.html', 'cookscope.snapshot.json')
foreach ($name in $expectedFiles) {
  $path = Join-Path $blockingOutput $name
  if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
    throw "Full audit did not write expected report: $path"
  }
}
$json = Get-Content -LiteralPath (Join-Path $blockingOutput 'cookscope.json') -Raw | ConvertFrom-Json
if ($json.schema -ne 'cookscope.result/1' -or $json.provenance.sourceSha -ne $sourceSha -or
    @($json.findings | Where-Object { $_.ruleId -eq 'naming.asset-prefix' -and $_.assetPath -like '*/BadName.BadName' }).Count -ne 1) {
  throw 'Full audit JSON does not contain the real BadName rule result and source SHA'
}
$primary = @($json.assets | Where-Object objectPath -eq '/Game/CookScopeFixtures/Primary/DA_Primary.DA_Primary')
$badName = @($json.assets | Where-Object objectPath -eq '/Game/CookScopeFixtures/Naming/BadName.BadName')
if ($primary.Count -ne 1 -or $primary[0].cookedSizeKind -ne 'actual-cooked' -or
    $badName.Count -ne 1 -or $badName[0].cookedSizeKind -ne 'unavailable') {
  throw 'Full audit did not preserve actual Cook inclusion versus unavailable source-only asset'
}

$nonBlockingOutput = Join-Path $runRoot 'non-blocking'
$nonBlockingExit = Invoke-FullAudit -Output $nonBlockingOutput -FailOnViolation $false -LogName 'non-blocking.log'
if ($nonBlockingExit -ne 0) {
  throw "Expected non-blocking full audit exit 0, got $nonBlockingExit"
}

[pscustomobject]@{
  Result = 'PASS'
  EvidenceRoot = $runRoot
  SourceSha = $sourceSha
  BlockingExitCode = $blockingExit
  NonBlockingExitCode = $nonBlockingExit
  FindingCount = @($json.findings).Count
} | Format-List
