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

The deadline is checked after each bounded phase and before report publication. An expired audit writes no reports. Each report is written beside its destination as `.tmp` and atomically replaced. The current UE Asset Registry synchronous call cannot be preempted mid-call; the deadline is enforced immediately after it.

`Tests/UE/FullAuditCommandletContract.ps1` verifies 2/0/5 full-mode behavior and absence of partial/temp files. `Tests/UE/CommandletContract.ps1` separately proves the complete 0/2/3/4/5 bootstrap matrix.

The checked-in workflow targets a self-hosted Windows runner with UE 5.8 and MQB. No hosted GitHub run is claimed in this local-only delivery.
