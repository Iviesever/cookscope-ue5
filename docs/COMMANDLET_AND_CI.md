# Commandlet and CI

Full mode requires `-config`, `-output`, and `-source-sha`. It also accepts `-scope`, `-baseline`, `-cook-registry`, `-cook-platform`, `-cook-configuration`, `-fail-on-violation`, and `-timeout-seconds`. Unknown, duplicate, malformed, or ambiguous arguments fail closed.

Exit codes are stable:

| Code | Meaning |
|---:|---|
| 0 | Clean or non-blocking successful audit |
| 2 | Configured violation threshold reached |
| 3 | Invocation/config/baseline error |
| 4 | Scan, diagnostic, internal, or report I/O error |
| 5 | Timeout/cancellation |

The in-process deadline is checked during the bounded Registry loop, after each phase, and before report publication. An expired audit writes no reports. Each report is written beside its destination as `.tmp` and atomically replaced. Input files and Registry result sizes also have hard limits.

For a strict wall-clock CI boundary, use `scripts/Invoke-CookScopeAudit.ps1`. It starts the exact `UnrealEditor-Cmd` process without a shell and always targets a unique sibling staging directory. On success it verifies all five reports, writes a `.cookscope-output` ownership marker, and switches the whole directory. An existing directory is replaceable only when every immediate entry is one of those five reports or the marker; unknown files and all subdirectories are preserved and rejected with exit 4. This also permits a legacy five-report directory without the marker to be upgraded safely. On timeout the wrapper terminates that process tree, deletes staging, preserves any prior report set, and returns 5.

`Tests/UE/FullAuditCommandletContract.ps1` verifies 2/0/5 full-mode behavior, clean and violation publication, foreign-directory rejection/preservation, the ownership marker, and a hard timeout during a deterministic 30-second stall after the first staged JSON write. The old five-file report set stays byte-identical and no staging directory remains. `Tests/UE/CommandletContract.ps1` separately proves the complete 0/2/3/4/5 bootstrap matrix.

The checked-in workflow targets a self-hosted Windows runner with UE 5.8 and MQB. No hosted GitHub run is claimed in this local-only delivery.
