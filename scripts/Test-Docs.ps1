$ErrorActionPreference = 'Stop'
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))

$required = @(
  'README.md',
  'README_ZH.md',
  'docs/ARCHITECTURE.md',
  'docs/ASSET_REGISTRY_MODEL.md',
  'docs/ASSET_MANAGER_AND_PRIMARY_ASSETS.md',
  'docs/COOK_PIPELINE.md',
  'docs/RULE_MODEL.md',
  'docs/SNAPSHOT_SCHEMA.md',
  'docs/DIFF_MODEL.md',
  'docs/REPORT_FORMATS.md',
  'docs/EDITOR_TOOLING.md',
  'docs/COMMANDLET_AND_CI.md',
  'docs/BUILD_SYSTEM.md',
  'docs/TESTING.md',
  'docs/KNOWN_LIMITATIONS.md',
  'docs/AI_ASSISTANCE.md',
  'docs/CODE_WALKTHROUGH.md',
  'docs/INTERVIEW_GUIDE.md',
  'docs/LIVE_CHANGE_DRILLS.md',
  'docs/RELEASE_NOTES_0.1.0.md',
  'docs/images/editor.png',
  'docs/images/report.png'
)
foreach ($relative in $required) {
  if (-not (Test-Path -LiteralPath (Join-Path $repositoryRoot $relative) -PathType Leaf)) {
    throw "Required portfolio artifact is missing: $relative"
  }
}

$readme = [System.IO.File]::ReadAllText((Join-Path $repositoryRoot 'README.md'))
foreach ($marker in @(
  'docs/images/editor.png',
  'docs/images/report.png',
  '-run=CookScopeAudit',
  '892 actual-cooked bytes',
  'AI assistance',
  'source-only'
)) {
  if (-not $readme.Contains($marker, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "README is missing required first-page marker: $marker"
  }
}

$markdownFiles = @(Get-ChildItem (Join-Path $repositoryRoot 'docs') -Filter '*.md' -File -Recurse) + @(Get-ChildItem $repositoryRoot -Filter 'README*.md' -File)
foreach ($file in $markdownFiles) {
  $text = [System.IO.File]::ReadAllText($file.FullName)
  foreach ($match in [regex]::Matches($text, '\[[^\]]+\]\(([^)]+)\)')) {
    $target = $match.Groups[1].Value
    if ($target.StartsWith('http', [System.StringComparison]::OrdinalIgnoreCase) -or $target.StartsWith('#')) {
      continue
    }
    $target = $target.Split('#')[0]
    $resolved = [System.IO.Path]::GetFullPath((Join-Path $file.DirectoryName $target))
    if (-not (Test-Path -LiteralPath $resolved)) {
      throw "Broken local Markdown link in $($file.FullName): $target"
    }
  }
}

foreach ($jsonPath in @(
  'Examples/reports/cookscope-sample.json',
  'Examples/reports/cookscope-sample.sarif',
  'Examples/snapshots/cookscope-real-baseline.json',
  'Examples/snapshots/cookscope-real-candidate.json',
  'Schemas/cookscope-rules.schema.json',
  'Schemas/cookscope-snapshot.schema.json'
)) {
  Get-Content -LiteralPath (Join-Path $repositoryRoot $jsonPath) -Raw | ConvertFrom-Json | Out-Null
}
[xml](Get-Content -LiteralPath (Join-Path $repositoryRoot 'Examples/reports/cookscope-sample.junit.xml') -Raw) | Out-Null

$html = [System.IO.File]::ReadAllText((Join-Path $repositoryRoot 'Examples/reports/cookscope-sample.html'))
foreach ($id in @('severity-filter', 'rule-filter', 'class-filter', 'path-search', 'chunk-filter', 'bundle-filter', 'comparison-summary', 'asset-table')) {
  if (-not $html.Contains("id=`"$id`"", [System.StringComparison]::Ordinal)) {
    throw "HTML sample is missing required control/view: $id"
  }
}
if ($html -match '<script\s+src=' -or $html -match '<link\s' -or $html -match 'https?://') {
  throw 'HTML sample is not self-contained'
}

$portfolioTextFiles = @(
  Get-Item (Join-Path $repositoryRoot 'README.md')
  Get-Item (Join-Path $repositoryRoot 'README_ZH.md')
  Get-ChildItem (Join-Path $repositoryRoot 'docs') -File -Recurse
  Get-ChildItem (Join-Path $repositoryRoot 'Examples') -File -Recurse
) | Where-Object { $_.Extension -in @('.md', '.json', '.html', '.sarif', '.xml') }
foreach ($file in $portfolioTextFiles) {
  $text = [System.IO.File]::ReadAllText($file.FullName)
  if ($text -match '(?i)[A-Z]:[\\/](Users|program)[\\/]') {
    throw "Host-absolute path leaked into portfolio artifact: $($file.FullName)"
  }
}

$tracked = @(& git -C $repositoryRoot ls-files)
$forbidden = @($tracked | Where-Object {
  $_ -match '(^|/)(Binaries|Intermediate|Saved|Artifacts)/' -or
  $_ -match '\.(dll|exe|pdb|pak|ucas|utoc|zip)$'
})
if ($forbidden.Count -ne 0) {
  throw "Generated/binary files are tracked: $($forbidden -join ', ')"
}

Write-Output "PASS: required docs, links, examples, HTML controls, path hygiene, and source-only boundary"
