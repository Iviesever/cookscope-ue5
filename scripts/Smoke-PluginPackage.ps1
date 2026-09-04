param(
  [Parameter(Mandatory = $true)]
  [string]$PackagePath,
  [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8'
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$packageBase = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot 'Artifacts\Packages'))
$resolvedPackage = [System.IO.Path]::GetFullPath($PackagePath)
if (-not $resolvedPackage.StartsWith($packageBase, [System.StringComparison]::OrdinalIgnoreCase)) {
  throw "Package path must remain under $packageBase"
}
$descriptor = Join-Path $resolvedPackage 'CookScope.uplugin'
if (-not (Test-Path -LiteralPath $descriptor -PathType Leaf)) {
  throw "Packaged CookScope descriptor not found: $descriptor"
}

$runId = [guid]::NewGuid().ToString('N')
$smokeBase = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot 'Artifacts\Smoke'))
$smokeRoot = [System.IO.Path]::GetFullPath((Join-Path $smokeBase ("plugin-" + $runId)))
$archiveBase = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot 'Artifacts\Archives'))
if (-not $smokeRoot.StartsWith($smokeBase, [System.StringComparison]::OrdinalIgnoreCase)) {
  throw "Refusing unsafe smoke path: $smokeRoot"
}
New-Item -ItemType Directory -Path $smokeRoot -Force | Out-Null
New-Item -ItemType Directory -Path $archiveBase -Force | Out-Null

$archive = Join-Path $archiveBase ("CookScope-" + $runId + '.zip')
[System.IO.Compression.ZipFile]::CreateFromDirectory(
  $resolvedPackage,
  $archive,
  [System.IO.Compression.CompressionLevel]::Optimal,
  $false)
$archiveHash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash
$archiveBytes = (Get-Item -LiteralPath $archive).Length

$hostProject = Join-Path $smokeRoot 'HostProject'
$extractedPlugin = Join-Path $hostProject 'Plugins\CookScope'
New-Item -ItemType Directory -Path $extractedPlugin -Force | Out-Null
[System.IO.Compression.ZipFile]::ExtractToDirectory($archive, $extractedPlugin)

$sourceDescriptorHash = (Get-FileHash -LiteralPath $descriptor -Algorithm SHA256).Hash
$extractedDescriptor = Join-Path $extractedPlugin 'CookScope.uplugin'
$extractedDescriptorHash = (Get-FileHash -LiteralPath $extractedDescriptor -Algorithm SHA256).Hash
if ($sourceDescriptorHash -ne $extractedDescriptorHash) {
  throw 'Fresh extraction changed the plugin descriptor identity'
}

$projectPath = Join-Path $hostProject 'HostProject.uproject'
$project = [ordered]@{
  FileVersion = 3
  EngineAssociation = '5.8'
  Category = 'CookScope Smoke'
  Description = 'Generated local host for fresh extracted CookScope package smoke testing.'
  Plugins = @([ordered]@{ Name = 'CookScope'; Enabled = $true })
}
$projectJson = $project | ConvertTo-Json -Depth 5
[System.IO.File]::WriteAllText($projectPath, $projectJson + "`n", [System.Text.UTF8Encoding]::new($false))

$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (-not (Test-Path -LiteralPath $editorCmd -PathType Leaf)) {
  throw "UnrealEditor-Cmd.exe not found: $editorCmd"
}
$reportPath = Join-Path $smokeRoot 'report.json'
$logPath = Join-Path $smokeRoot 'commandlet-load.log'
$arguments = @(
  $projectPath,
  '-run=CookScopeAudit',
  '-unattended',
  '-nop4',
  '-nosplash',
  '-nullrhi',
  '-nosound',
  '-stdout',
  '-FullStdOutLogOutput',
  "-output=$reportPath"
)
& $editorCmd @arguments *> $logPath
$commandletExitCode = $LASTEXITCODE
if ($commandletExitCode -ne 0) {
  throw "Fresh extracted plugin Commandlet load failed with exit code $commandletExitCode; log: $logPath"
}

$logText = [System.IO.File]::ReadAllText($logPath)
if ($logText -notmatch 'Mounting Project plugin CookScope' -or
    $logText -notmatch "InternalLoadLibrary: 'CookScopeCore'" -or
    $logText -notmatch "InternalLoadLibrary: 'CookScopeCommandlet'" -or
    $logText -notmatch "InternalLoadLibrary: 'CookScopeEditor'") {
  throw "Fresh extracted plugin load markers were not found; log: $logPath"
}
$expectedReport = '{"schema":"cookscope.result/1","status":"clean","summary":{"errors":0,"warnings":0}}' + "`n"
if (-not (Test-Path -LiteralPath $reportPath -PathType Leaf) -or
    [System.IO.File]::ReadAllText($reportPath) -ne $expectedReport) {
  throw 'Fresh extracted plugin Commandlet did not produce canonical clean JSON'
}

$summary = [ordered]@{
  schema = 'cookscope.plugin-smoke/1'
  packagePath = $resolvedPackage
  archivePath = $archive
  archiveBytes = [int64]$archiveBytes
  archiveSha256 = $archiveHash
  smokeRoot = $smokeRoot
  sourceDescriptorSha256 = $sourceDescriptorHash
  extractedDescriptorSha256 = $extractedDescriptorHash
  commandletExitCode = $commandletExitCode
  reportSha256 = (Get-FileHash -LiteralPath $reportPath -Algorithm SHA256).Hash
  coreLoaded = $true
  commandletLoaded = $true
  editorLoaded = $true
}
$summaryPath = Join-Path $smokeRoot 'summary.json'
$summaryJson = $summary | ConvertTo-Json -Depth 5
[System.IO.File]::WriteAllText($summaryPath, $summaryJson + "`n", [System.Text.UTF8Encoding]::new($false))
$summary | Format-List
