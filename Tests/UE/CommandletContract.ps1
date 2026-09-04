param(
  [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8'
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$project = Join-Path $repositoryRoot 'SampleProject\CookScopeSample.uproject'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (-not (Test-Path -LiteralPath $editorCmd -PathType Leaf)) {
  throw "UnrealEditor-Cmd.exe not found: $editorCmd"
}
if (-not (Test-Path -LiteralPath $project -PathType Leaf)) {
  throw "Sample Project not found: $project"
}

$evidenceBase = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot 'Artifacts\Evidence\PACT-00\Commandlet'))
$runRoot = [System.IO.Path]::GetFullPath((Join-Path $evidenceBase ("run-" + [guid]::NewGuid().ToString('N'))))
if (-not $runRoot.StartsWith($evidenceBase, [System.StringComparison]::OrdinalIgnoreCase)) {
  throw "Refusing unsafe commandlet evidence path: $runRoot"
}
New-Item -ItemType Directory -Path $runRoot -Force | Out-Null

function Invoke-CookScopeCommandlet {
  param(
    [Parameter(Mandatory = $true)]
    [string[]]$CommandletArguments,
    [Parameter(Mandatory = $true)]
    [string]$LogName
  )

  $logPath = Join-Path $runRoot $LogName
  $commonArguments = @(
    $project,
    '-run=CookScopeAudit',
    '-unattended',
    '-nop4',
    '-nosplash',
    '-nullrhi',
    '-nosound',
    '-stdout',
    '-FullStdOutLogOutput'
  )
  & $editorCmd @commonArguments @CommandletArguments *> $logPath
  return $LASTEXITCODE
}

$cleanPath = Join-Path $runRoot 'clean.json'
$cleanExit = Invoke-CookScopeCommandlet -CommandletArguments @("-output=$cleanPath") -LogName 'clean.log'
if ($cleanExit -ne 0) {
  throw "Expected clean commandlet exit 0, got $cleanExit"
}
$expectedClean = '{"schema":"cookscope.result/1","status":"clean","summary":{"errors":0,"warnings":0}}' + "`n"
if ([System.IO.File]::ReadAllText($cleanPath) -ne $expectedClean) {
  throw 'Clean commandlet report bytes do not match the canonical contract'
}

$violationPath = Join-Path $runRoot 'violation.json'
$violationExit = Invoke-CookScopeCommandlet -CommandletArguments @(
  "-output=$violationPath",
  '-fixture-status=violation'
) -LogName 'violation.log'
if ($violationExit -ne 2) {
  throw "Expected violation commandlet exit 2, got $violationExit"
}

$invalidExit = Invoke-CookScopeCommandlet -CommandletArguments @('-surprise=true') -LogName 'invalid.log'
if ($invalidExit -ne 3) {
  throw "Expected invalid commandlet exit 3, got $invalidExit"
}

$blocker = Join-Path $runRoot 'not-a-directory'
[System.IO.File]::WriteAllText($blocker, 'file blocks directory creation')
$internalPath = Join-Path $blocker 'report.json'
$internalExit = Invoke-CookScopeCommandlet -CommandletArguments @("-output=$internalPath") -LogName 'internal.log'
if ($internalExit -ne 4) {
  throw "Expected internal commandlet exit 4, got $internalExit"
}

$cancelledPath = Join-Path $runRoot 'cancelled.json'
$cancelledExit = Invoke-CookScopeCommandlet -CommandletArguments @(
  "-output=$cancelledPath",
  '-fixture-status=cancelled'
) -LogName 'cancelled.log'
if ($cancelledExit -ne 5) {
  throw "Expected cancelled commandlet exit 5, got $cancelledExit"
}

[pscustomobject]@{
  Result = 'PASS'
  EvidenceRoot = $runRoot
  CleanExitCode = $cleanExit
  ViolationExitCode = $violationExit
  InvalidInvocationExitCode = $invalidExit
  InternalErrorExitCode = $internalExit
  CancelledExitCode = $cancelledExit
} | Format-List
