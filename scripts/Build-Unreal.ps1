param(
  [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8'
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$project = Join-Path $repositoryRoot 'SampleProject\CookScopeSample.uproject'
$build = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
if (-not (Test-Path -LiteralPath $build -PathType Leaf)) {
  throw "UE Build.bat not found: $build"
}
if (-not (Test-Path -LiteralPath $project -PathType Leaf)) {
  throw "Sample Project not found: $project"
}

& $build CookScopeSampleEditor Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE -NoUBTMakefiles
if ($LASTEXITCODE -ne 0) {
  throw "UE Editor build failed with exit code $LASTEXITCODE"
}
