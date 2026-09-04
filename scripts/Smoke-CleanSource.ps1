param(
  [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8'
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$dirty = & git -C $repositoryRoot status --porcelain --untracked-files=all
if ($dirty) {
  throw 'Clean-source smoke requires a clean worktree'
}
$sourceSha = (& git -C $repositoryRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $sourceSha -notmatch '^[0-9a-f]{40}$') {
  throw 'Unable to resolve source SHA'
}

$smokeBase = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot 'Artifacts\Smoke\CleanSource'))
$runRoot = [System.IO.Path]::GetFullPath((Join-Path $smokeBase ("run-" + [guid]::NewGuid().ToString('N'))))
if (-not $runRoot.StartsWith($smokeBase, [System.StringComparison]::OrdinalIgnoreCase)) {
  throw "Refusing unsafe clean-source smoke path: $runRoot"
}
$sourceRoot = Join-Path $runRoot 'source'
$archivePath = Join-Path $runRoot 'source.zip'
New-Item -ItemType Directory -Path $sourceRoot -Force | Out-Null

& git -C $repositoryRoot archive --format=zip --output=$archivePath HEAD
if ($LASTEXITCODE -ne 0) {
  throw 'git archive failed'
}
[System.IO.Compression.ZipFile]::ExtractToDirectory($archivePath, $sourceRoot)
if (Test-Path -LiteralPath (Join-Path $sourceRoot '.git')) {
  throw 'Clean-source archive unexpectedly contains Git metadata'
}

$coreLog = Join-Path $runRoot 'core.log'
& (Join-Path $sourceRoot 'scripts\Test.ps1') *> $coreLog
if ($LASTEXITCODE -ne 0) {
  throw "Clean-source Core tests failed; log: $coreLog"
}

$buildLog = Join-Path $runRoot 'ubt.log'
& (Join-Path $sourceRoot 'scripts\Build-Unreal.ps1') -EngineRoot $EngineRoot *> $buildLog
if ($LASTEXITCODE -ne 0) {
  throw "Clean-source UBT build failed; log: $buildLog"
}

$fixtureLog = Join-Path $runRoot 'fixtures.log'
& (Join-Path $sourceRoot 'Tests\UE\FixtureBuilderContract.ps1') -EngineRoot $EngineRoot *> $fixtureLog
if ($LASTEXITCODE -ne 0) {
  throw "Clean-source fixture generation failed; log: $fixtureLog"
}

$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$project = Join-Path $sourceRoot 'SampleProject\CookScopeSample.uproject'
$automationLog = Join-Path $runRoot 'automation.log'
$arguments = @(
  $project,
  '-unattended',
  '-nop4',
  '-nosplash',
  '-nullrhi',
  '-nosound',
  '-stdout',
  '-FullStdOutLogOutput',
  '-ExecCmds=Automation RunTests CookScope.PACT00.EditorAndCommandletContracts;Quit',
  '-TestExit=Automation Test Queue Empty'
)
& $editorCmd @arguments *> $automationLog
if ($LASTEXITCODE -ne 0) {
  throw "Clean-source Editor smoke failed; log: $automationLog"
}
$automationText = [System.IO.File]::ReadAllText($automationLog)
if ($automationText -notmatch 'Test Completed\. Result=\{Success\}.*CookScope\.PACT00\.EditorAndCommandletContracts' -or
    $automationText -notmatch '\*\*\*\* TEST COMPLETE\. EXIT CODE: 0 \*\*\*\*') {
  throw "Clean-source Editor success markers were not found; log: $automationLog"
}

$summary = [ordered]@{
  schema = 'cookscope.clean-source-smoke/1'
  sourceSha = $sourceSha
  archiveSha256 = (Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash
  archiveBytes = (Get-Item -LiteralPath $archivePath).Length
  coreExitCode = 0
  ubtExitCode = 0
  fixtureExitCode = 0
  editorExitCode = 0
  runRoot = $runRoot
}
$summaryPath = Join-Path $runRoot 'summary.json'
[System.IO.File]::WriteAllText(
  $summaryPath,
  ($summary | ConvertTo-Json -Depth 4) + "`n",
  [System.Text.UTF8Encoding]::new($false))
$summary | Format-List
