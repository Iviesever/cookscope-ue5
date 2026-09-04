param(
  [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8'
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$project = Join-Path $repositoryRoot 'SampleProject\CookScopeSample.uproject'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$evidenceRoot = Join-Path $repositoryRoot 'Artifacts\Evidence\PACT-00\UE'
New-Item -ItemType Directory -Path $evidenceRoot -Force | Out-Null

& (Join-Path $PSScriptRoot 'Build-Unreal.ps1') -EngineRoot $EngineRoot
& (Join-Path $repositoryRoot 'Tests\UE\FixtureBuilderContract.ps1') -EngineRoot $EngineRoot

$automationLog = Join-Path $evidenceRoot 'automation-current.log'
$automationArguments = @(
  $project,
  '-unattended',
  '-nop4',
  '-nosplash',
  '-nullrhi',
  '-nosound',
  '-stdout',
  '-FullStdOutLogOutput',
  '-ExecCmds=Automation RunTests CookScope.PACT;Quit',
  '-TestExit=Automation Test Queue Empty'
)
& $editorCmd @automationArguments *> $automationLog
if ($LASTEXITCODE -ne 0) {
  throw "UE Automation process failed with exit code $LASTEXITCODE; log: $automationLog"
}

$automationText = [System.IO.File]::ReadAllText($automationLog)
if ($automationText -notmatch 'Found 3 automation tests' -or
    $automationText -notmatch 'Test Completed\. Result=\{Success\}.*CookScope\.PACT00\.EditorAndCommandletContracts' -or
    $automationText -notmatch 'Test Completed\. Result=\{Success\}.*CookScope\.PACT20\.RealAssetRegistryScan' -or
    $automationText -notmatch 'Test Completed\. Result=\{Success\}.*CookScope\.PACT30\.DataValidationReuse' -or
    $automationText -notmatch '\*\*\*\* TEST COMPLETE\. EXIT CODE: 0 \*\*\*\*') {
  throw "CookScope Automation success markers were not found; log: $automationLog"
}

& (Join-Path $repositoryRoot 'Tests\UE\CommandletContract.ps1') -EngineRoot $EngineRoot
& (Join-Path $repositoryRoot 'Tests\UE\FullAuditCommandletContract.ps1') -EngineRoot $EngineRoot

Write-Output "PASS: UE build, fixtures, PACT Automation, Registry, Data Validation, bootstrap/full audit Commandlets, and reports"
