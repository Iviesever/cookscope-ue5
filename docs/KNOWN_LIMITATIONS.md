# Known Limitations

- The public repository and normal PR merge are complete. No hosted GitHub Actions success is claimed: the checked-in UE 5.8 workflow requires a specifically labelled self-hosted Windows runner, while the release decision is supported by the recorded local full-suite, package, extraction, clean-source, and independent-audit evidence.
- GitHub Release contents are source only. Local BuildPlugin packages, Cook output, smoke archives, and logs remain ignored and are intentionally not uploaded.
- Actual-cooked size depends on a compatible UE Development Asset Registry. Without it, Cook measurements are `unavailable`; package-disk size is not substituted.
- Editor Registry acquisition uses indexed state and never triggers disk discovery during a scan. The module refreshes/snapshots Asset Manager ownership once at PostEngineInit; this engine call is synchronous. Per-scan work fails closed above 4,096 assets or 65,536 typed dependencies, and Core work remains asynchronous/cancellable.
- The in-process Commandlet deadline is cooperative between bounded phases. `scripts/Invoke-CookScopeAudit.ps1` is the strict CI wall-clock guard: it terminates the exact Editor process tree, returns 5, and prevents timed-out report publication.
- Rule-specific parsing rejects missing or invalid required parameters, but a `parameters` object that also contains an unknown extra key is not rejected in v0.1.
- `StableAssetId` is optional and is not required to be unique across one snapshot; producers should emit unique values because duplicate IDs can make rename pairing ambiguous.
- Soft-to-Hard baseline worsening is covered by pure Core contracts; the Sample Project does not include a real UE Soft-to-Hard migration fixture.
- v0.1 does not parse Asset Loading Insights/Memory Insights traces, build Pak/IoStore containers, or correlate MQB object files with UE asset dependencies.
- The checked-in default rule file intentionally demonstrates one naming rule. The engine supports the documented P0 families through user configuration.
