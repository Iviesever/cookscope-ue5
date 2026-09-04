param(
  [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8'
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$cookScript = Join-Path $repositoryRoot 'scripts\Cook.ps1'
if (-not (Test-Path -LiteralPath $cookScript -PathType Leaf)) {
  throw "Cook entry script not found: $cookScript"
}

& $cookScript -EngineRoot $EngineRoot

$cookedRoot = Join-Path $repositoryRoot 'SampleProject\Saved\Cooked\Windows'
if (-not (Test-Path -LiteralPath $cookedRoot -PathType Container)) {
  throw "Real Cook output directory not found: $cookedRoot"
}
$files = @(Get-ChildItem -LiteralPath $cookedRoot -Recurse -File)
if ($files.Count -eq 0) {
  throw 'Real Cook produced no files'
}
$assetRegistries = @($files | Where-Object { $_.Name -in @('DevelopmentAssetRegistry.bin', 'AssetRegistry.bin') })
if ($assetRegistries.Count -lt 2) {
  throw 'Cook output must contain both DevelopmentAssetRegistry.bin and AssetRegistry.bin'
}
$developmentRegistry = $assetRegistries | Where-Object Name -eq 'DevelopmentAssetRegistry.bin' | Select-Object -First 1
$zenManifest = $files | Where-Object Name -eq 'zenfs.manifest' | Select-Object -First 1
if ($null -eq $developmentRegistry -or $null -eq $zenManifest) {
  throw 'UE 5.8 Zen Cook metadata is incomplete'
}

function Test-BinaryContainsUtf8 {
  param(
    [Parameter(Mandatory = $true)]
    [string]$Path,
    [Parameter(Mandatory = $true)]
    [string]$Text
  )
  $bytes = [System.IO.File]::ReadAllBytes($Path)
  $needle = [System.Text.Encoding]::UTF8.GetBytes($Text)
  for ($start = 0; $start -le $bytes.Length - $needle.Length; ++$start) {
    $matches = $true
    for ($offset = 0; $offset -lt $needle.Length; ++$offset) {
      if ($bytes[$start + $offset] -ne $needle[$offset]) {
        $matches = $false
        break
      }
    }
    if ($matches) {
      return $true
    }
  }
  return $false
}
if (-not (Test-BinaryContainsUtf8 -Path $developmentRegistry.FullName -Text 'DA_Primary') -or
    -not (Test-BinaryContainsUtf8 -Path $developmentRegistry.FullName -Text 'CookScopeFixtures')) {
  throw 'Development Asset Registry does not contain the expected Primary fixture identity'
}

[pscustomobject]@{
  Result = 'PASS'
  CookedRoot = $cookedRoot
  FileCount = $files.Count
  TotalBytes = [int64](($files | Measure-Object Length -Sum).Sum)
  DevelopmentAssetRegistry = $developmentRegistry.FullName
  DevelopmentAssetRegistryBytes = $developmentRegistry.Length
  ZenManifest = $zenManifest.FullName
  ZenManifestBytes = $zenManifest.Length
  PrimaryFixtureInRegistry = $true
  AssetRegistryFiles = @($assetRegistries.FullName)
} | Format-List
