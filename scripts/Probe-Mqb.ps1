param(
  [ValidateSet('Release')]
  [string]$Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$cachePath = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot '.mqb'))
$expectedCachePath = [System.IO.Path]::GetFullPath("$repositoryRoot\.mqb")
if (-not $cachePath.Equals($expectedCachePath, [System.StringComparison]::OrdinalIgnoreCase)) {
  throw "Refusing unsafe MQB cache path: $cachePath"
}

$evidenceRoot = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot 'Artifacts\Evidence\PACT-00\MQB'))
if (-not $evidenceRoot.StartsWith([System.IO.Path]::GetFullPath((Join-Path $repositoryRoot 'Artifacts')), [System.StringComparison]::OrdinalIgnoreCase)) {
  throw "Refusing unsafe evidence path: $evidenceRoot"
}

$mqb = Get-Command mqb -ErrorAction Stop
New-Item -ItemType Directory -Path $evidenceRoot -Force | Out-Null
if (Test-Path -LiteralPath $cachePath) {
  Remove-Item -LiteralPath $cachePath -Recurse -Force
}

Push-Location $repositoryRoot
try {
  $cleanLog = Join-Path $evidenceRoot 'clean-build.log'
  & $mqb.Source build --profile $Configuration.ToLowerInvariant() --timings=json 2>&1 | Tee-Object -FilePath $cleanLog
  $cleanExitCode = $LASTEXITCODE
  if ($cleanExitCode -ne 0) {
    throw "MQB clean build failed with exit code $cleanExitCode"
  }

  $artifact = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot '.mqb\bin\CookScopeCli.exe'))
  if (-not (Test-Path -LiteralPath $artifact -PathType Leaf)) {
    throw "MQB did not produce the expected artifact: $artifact"
  }
  $cleanHash = (Get-FileHash -LiteralPath $artifact -Algorithm SHA256).Hash

  $noOpLog = Join-Path $evidenceRoot 'no-op-build.log'
  & $mqb.Source build --profile $Configuration.ToLowerInvariant() --timings=json 2>&1 | Tee-Object -FilePath $noOpLog
  $noOpExitCode = $LASTEXITCODE
  if ($noOpExitCode -ne 0) {
    throw "MQB no-op build failed with exit code $noOpExitCode"
  }
  $noOpHash = (Get-FileHash -LiteralPath $artifact -Algorithm SHA256).Hash
  if ($cleanHash -ne $noOpHash) {
    throw 'MQB no-op build changed the artifact identity'
  }

  $failureSource = Join-Path $evidenceRoot 'expected-failure.cpp'
  [System.IO.File]::WriteAllText($failureSource, "#error COOKSCOPE_EXPECTED_MQB_FAILURE`nint main() { return 0; }`n")
  $failureLog = Join-Path $evidenceRoot 'expected-failure.log'
  & $mqb.Source build $failureSource --no-discover --std 20 --release -o CookScopeExpectedFailure 2>&1 | Tee-Object -FilePath $failureLog
  $failureExitCode = $LASTEXITCODE
  if ($failureExitCode -eq 0) {
    throw 'MQB intentional failure probe unexpectedly succeeded'
  }

  $artifactInfo = Get-Item -LiteralPath $artifact
  $summary = [ordered]@{
    schema = 'cookscope.mqb-probe/1'
    configuration = $Configuration.ToLowerInvariant()
    cleanExitCode = $cleanExitCode
    noOpExitCode = $noOpExitCode
    expectedFailureExitCode = $failureExitCode
    artifact = $artifact
    artifactBytes = $artifactInfo.Length
    artifactSha256 = $cleanHash
    cleanLog = $cleanLog
    noOpLog = $noOpLog
    expectedFailureLog = $failureLog
  }
  $summary | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $evidenceRoot 'summary.json') -Encoding utf8NoBOM
  $summary | Format-List
}
finally {
  Pop-Location
}
