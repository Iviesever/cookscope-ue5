param(
  [ValidateSet('Release')]
  [string]$Configuration = 'Release',
  [switch]$Timings
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$mqb = Get-Command mqb -ErrorAction Stop
$arguments = @('build', '--profile', $Configuration.ToLowerInvariant())
if ($Timings) {
  $arguments += '--timings=json'
}

Push-Location $repositoryRoot
try {
  & $mqb.Source @arguments
  if ($LASTEXITCODE -ne 0) {
    throw "MQB build failed with exit code $LASTEXITCODE"
  }
}
finally {
  Pop-Location
}

