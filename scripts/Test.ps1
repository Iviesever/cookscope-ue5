param(
  [ValidateSet('Release')]
  [string]$Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$mqb = Get-Command mqb -ErrorAction Stop
$profile = $Configuration.ToLowerInvariant()
$publicCore = 'Plugins/CookScope/Source/CookScopeCore/Public'
$privateCore = 'Plugins/CookScope/Source/CookScopeCore/Private'

function Invoke-MqbTest {
  param(
    [Parameter(Mandatory = $true)]
    [string[]]$Sources,
    [Parameter(Mandatory = $true)]
    [string]$Output
  )

  & $mqb.Source run @Sources -I $publicCore --std 20 --profile $profile -o $Output
  if ($LASTEXITCODE -ne 0) {
    throw "MQB test target $Output failed with exit code $LASTEXITCODE"
  }
}

Push-Location $repositoryRoot
try {
  Invoke-MqbTest -Sources @(
    'Tests/Core/BootstrapContractTests.cpp',
    "$privateCore/bootstrap.cpp"
  ) -Output 'CookScopeBootstrapContractTests'

  Invoke-MqbTest -Sources @(
    'Tests/Core/ArgumentsContractTests.cpp',
    "$privateCore/arguments.cpp"
  ) -Output 'CookScopeArgumentsContractTests'

  Invoke-MqbTest -Sources @(
    'Tests/Core/JsonContractTests.cpp',
    "$privateCore/json.cpp"
  ) -Output 'CookScopeJsonContractTests'

  Invoke-MqbTest -Sources @(
    'Tests/Core/RuleConfigContractTests.cpp',
    "$privateCore/rule_config.cpp",
    "$privateCore/json.cpp"
  ) -Output 'CookScopeRuleConfigContractTests'

  Invoke-MqbTest -Sources @(
    'Tests/Core/SnapshotContractTests.cpp',
    "$privateCore/snapshot.cpp",
    "$privateCore/json.cpp"
  ) -Output 'CookScopeSnapshotContractTests'

  & (Join-Path $PSScriptRoot 'Build.ps1') -Configuration $Configuration
  & (Join-Path $PSScriptRoot '..\Tests\CLI\BootstrapCliContract.ps1') -Executable '.mqb/bin/CookScopeCli.exe'
}
finally {
  Pop-Location
}
