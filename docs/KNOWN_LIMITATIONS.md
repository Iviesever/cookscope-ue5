# Known Limitations

- No GitHub remote, Draft PR, hosted CI result, merge, tag publication, or Release is claimed. Authentication was explicitly declined; local Git work is unaffected.
- The local version remains an Alpha candidate until the final independent audit passes and a source-only remote Release is authorized.
- Actual-cooked size depends on a compatible UE Development Asset Registry. Without it, Cook measurements are `unavailable`; package-disk size is not substituted.
- Editor Registry acquisition uses indexed state and never triggers disk discovery during a scan. The module refreshes/snapshots Asset Manager ownership once at PostEngineInit; this engine call is synchronous. Per-scan work fails closed above 4,096 assets or 65,536 typed dependencies, and Core work remains asynchronous/cancellable.
- The in-process Commandlet deadline is cooperative between bounded phases. `scripts/Invoke-CookScopeAudit.ps1` is the strict CI wall-clock guard: it terminates the exact Editor process tree, returns 5, and prevents timed-out report publication.
- v0.1 does not parse Asset Loading Insights/Memory Insights traces, build Pak/IoStore containers, or correlate MQB object files with UE asset dependencies.
- The checked-in default rule file intentionally demonstrates one naming rule. The engine supports the documented P0 families through user configuration.
