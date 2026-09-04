param(
  [Parameter(Mandatory = $true)]
  [string]$Project,
  [Parameter(Mandatory = $true)]
  [string]$Config,
  [Parameter(Mandatory = $true)]
  [string]$Output,
  [Parameter(Mandatory = $true)]
  [string]$SourceSha,
  [string]$Scope = '/Game',
  [ValidateRange(1, 86400)]
  [int]$TimeoutSeconds = 120,
  [ValidateSet('true', 'false')]
  [string]$FailOnViolation = 'true',
  [string]$Baseline = '',
  [string]$CookRegistry = '',
  [string]$CookPlatform = 'Windows',
  [string]$CookConfiguration = 'Development',
  [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
  [string]$LogPath = ''
)

$ErrorActionPreference = 'Stop'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (-not (Test-Path -LiteralPath $editorCmd -PathType Leaf)) {
  throw "UnrealEditor-Cmd.exe not found: $editorCmd"
}
foreach ($requiredPath in @($Project, $Config)) {
  if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
    throw "Required audit input not found: $requiredPath"
  }
}
if ($SourceSha -notmatch '^[0-9a-fA-F]{40}$') {
  throw 'SourceSha must contain exactly 40 hexadecimal characters'
}
$Project = [System.IO.Path]::GetFullPath($Project)
$Config = [System.IO.Path]::GetFullPath($Config)
$Output = [System.IO.Path]::GetFullPath($Output)
if ([string]::IsNullOrWhiteSpace($LogPath)) {
  $LogPath = $Output + '.process.log'
}
$LogPath = [System.IO.Path]::GetFullPath($LogPath)
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $LogPath) | Out-Null

$arguments = [System.Collections.Generic.List[string]]::new()
foreach ($argument in @(
  $Project,
  '-run=CookScopeAudit',
  '-unattended',
  '-nop4',
  '-nosplash',
  '-nullrhi',
  '-nosound',
  '-stdout',
  '-FullStdOutLogOutput',
  "-config=$Config",
  "-output=$Output",
  "-source-sha=$($SourceSha.ToLowerInvariant())",
  "-scope=$Scope",
  "-fail-on-violation=$($FailOnViolation.ToLowerInvariant())",
  "-timeout-seconds=$TimeoutSeconds"
)) {
  $arguments.Add($argument)
}
if (-not [string]::IsNullOrWhiteSpace($Baseline)) {
  $arguments.Add('-baseline=' + [System.IO.Path]::GetFullPath($Baseline))
}
if (-not [string]::IsNullOrWhiteSpace($CookRegistry)) {
  $arguments.Add('-cook-registry=' + [System.IO.Path]::GetFullPath($CookRegistry))
  $arguments.Add("-cook-platform=$CookPlatform")
  $arguments.Add("-cook-configuration=$CookConfiguration")
}

$startInfo = [System.Diagnostics.ProcessStartInfo]::new()
$startInfo.FileName = $editorCmd
$startInfo.UseShellExecute = $false
$startInfo.CreateNoWindow = $true
$startInfo.RedirectStandardOutput = $true
$startInfo.RedirectStandardError = $true
foreach ($argument in $arguments) {
  [void]$startInfo.ArgumentList.Add($argument)
}

$auditProcess = [System.Diagnostics.Process]::new()
$auditProcess.StartInfo = $startInfo
if (-not $auditProcess.Start()) {
  throw 'Failed to start UnrealEditor-Cmd audit process'
}
$stdoutTask = $auditProcess.StandardOutput.ReadToEndAsync()
$stderrTask = $auditProcess.StandardError.ReadToEndAsync()
$completed = $auditProcess.WaitForExit($TimeoutSeconds * 1000)
$hardTimedOut = -not $completed
if ($hardTimedOut) {
  $auditProcess.Kill($true)
  $auditProcess.WaitForExit()
}
$stdout = $stdoutTask.GetAwaiter().GetResult()
$stderr = $stderrTask.GetAwaiter().GetResult()
[System.IO.File]::WriteAllText(
  $LogPath,
  $stdout + $stderr,
  [System.Text.UTF8Encoding]::new($false))
$exitCode = if ($hardTimedOut) { 5 } else { $auditProcess.ExitCode }
$auditProcess.Dispose()
exit $exitCode
