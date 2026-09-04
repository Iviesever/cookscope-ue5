param(
  [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
  [string]$PackagePath = ''
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$plugin = Join-Path $repositoryRoot 'Plugins\CookScope\CookScope.uplugin'
$runUat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
if (-not (Test-Path -LiteralPath $runUat -PathType Leaf)) {
  throw "UE RunUAT.bat not found: $runUat"
}
if (-not (Test-Path -LiteralPath $plugin -PathType Leaf)) {
  throw "CookScope plugin descriptor not found: $plugin"
}

Push-Location $repositoryRoot
try {
  $sourceSha = (& git rev-parse HEAD).Trim()
  if ($LASTEXITCODE -ne 0 -or $sourceSha -notmatch '^[0-9a-f]{40}$') {
    throw 'Unable to resolve source Git SHA'
  }
  $dirty = & git status --porcelain --untracked-files=all
  if ($dirty) {
    throw 'BuildPlugin requires a clean source tree so the package can be bound to HEAD'
  }

  if ([string]::IsNullOrWhiteSpace($PackagePath)) {
    $PackagePath = Join-Path $repositoryRoot ("Artifacts\Packages\CookScope-" + $sourceSha.Substring(0, 12))
  }
  $resolvedPackage = [System.IO.Path]::GetFullPath($PackagePath)
  $packageBase = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot 'Artifacts\Packages'))
  if (-not $resolvedPackage.StartsWith($packageBase, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Package path must remain under $packageBase"
  }
  if (Test-Path -LiteralPath $resolvedPackage) {
    throw "Package path already exists; choose a new path instead of overwriting it: $resolvedPackage"
  }

  & $runUat BuildPlugin "-Plugin=$plugin" "-Package=$resolvedPackage" -TargetPlatforms=Win64 -Rocket
  if ($LASTEXITCODE -ne 0) {
    throw "BuildPlugin failed with exit code $LASTEXITCODE"
  }

  $files = @(Get-ChildItem -LiteralPath $resolvedPackage -Recurse -File | Sort-Object FullName)
  $identity = @($files | Where-Object { $_.Extension -in @('.uplugin', '.dll') } | ForEach-Object {
    [ordered]@{
      path = $_.FullName.Substring($resolvedPackage.Length + 1).Replace('\', '/')
      bytes = $_.Length
      sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    }
  })
  $summary = [ordered]@{
    schema = 'cookscope.plugin-package/1'
    sourceSha = $sourceSha
    engine = '5.8.0-55116800'
    packagePath = $resolvedPackage
    fileCount = $files.Count
    totalBytes = [int64](($files | Measure-Object Length -Sum).Sum)
    identityFiles = $identity
  }
  $summaryPath = "$resolvedPackage.summary.json"
  $summaryJson = $summary | ConvertTo-Json -Depth 6
  [System.IO.File]::WriteAllText($summaryPath, $summaryJson + "`n", [System.Text.UTF8Encoding]::new($false))
  $summary | Format-List
}
finally {
  Pop-Location
}
