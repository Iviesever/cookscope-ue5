param(
  [Parameter(Mandatory = $true)]
  [string]$Executable
)

$ErrorActionPreference = 'Stop'
$resolvedExecutable = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Executable)
if (-not (Test-Path -LiteralPath $resolvedExecutable -PathType Leaf)) {
  throw "CookScope CLI executable not found: $resolvedExecutable"
}

$tempBase = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath())
$testRoot = [System.IO.Path]::GetFullPath((Join-Path $tempBase ("cookscope-cli-contract-" + [guid]::NewGuid().ToString('N'))))
if (-not $testRoot.StartsWith($tempBase, [System.StringComparison]::OrdinalIgnoreCase)) {
  throw "Refusing unsafe temporary path: $testRoot"
}
New-Item -ItemType Directory -Path $testRoot | Out-Null

try {
  & $resolvedExecutable 2>$null | Out-Null
  if ($LASTEXITCODE -ne 3) {
    throw "Expected missing arguments to exit 3, got $LASTEXITCODE"
  }

  $cleanPath = Join-Path $testRoot 'clean.json'
  & $resolvedExecutable "-output=$cleanPath" | Out-Null
  if ($LASTEXITCODE -ne 0) {
    throw "Expected clean run to exit 0, got $LASTEXITCODE"
  }
  $expectedClean = '{"schema":"cookscope.result/1","status":"clean","summary":{"errors":0,"warnings":0}}' + "`n"
  if ([System.IO.File]::ReadAllText($cleanPath) -ne $expectedClean) {
    throw 'Clean report bytes do not match the canonical contract'
  }

  $violationPath = Join-Path $testRoot 'violation.json'
  & $resolvedExecutable "-output=$violationPath" '-fixture-status=violation' | Out-Null
  if ($LASTEXITCODE -ne 2) {
    throw "Expected violation run to exit 2, got $LASTEXITCODE"
  }
  $expectedViolation = '{"schema":"cookscope.result/1","status":"violation","summary":{"errors":1,"warnings":0}}' + "`n"
  if ([System.IO.File]::ReadAllText($violationPath) -ne $expectedViolation) {
    throw 'Violation report bytes do not match the canonical contract'
  }

  & $resolvedExecutable '-surprise=true' 2>$null | Out-Null
  if ($LASTEXITCODE -ne 3) {
    throw "Expected unknown argument to exit 3, got $LASTEXITCODE"
  }

  Write-Output 'PASS: bootstrap CLI process, report bytes, and exit codes'
}
finally {
  if (Test-Path -LiteralPath $testRoot) {
    Remove-Item -LiteralPath $testRoot -Recurse -Force
  }
}
