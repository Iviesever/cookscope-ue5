# Known Limitations

- No GitHub remote, Draft PR, hosted CI result, merge, tag publication, or Release is claimed. Authentication was explicitly declined; local Git work is unaffected.
- The local version remains an Alpha candidate until the final clean-source/package audit is complete and a source-only remote Release is authorized.
- Actual-cooked size depends on a compatible UE Development Asset Registry. Without it, Cook measurements are `unavailable`; package-disk size is not substituted.
- UE Asset Registry acquisition is synchronous and scoped on the Editor thread. Long Core computation is asynchronous/cancellable, but an individual Registry call cannot be preempted.
- Commandlet deadlines are phase-bounded and cannot interrupt UE inside a synchronous Registry call; they fail immediately after that phase and publish no timed-out reports.
- v0.1 does not parse Asset Loading Insights/Memory Insights traces, build Pak/IoStore containers, or correlate MQB object files with UE asset dependencies.
- The checked-in default rule file intentionally demonstrates one naming rule. The engine supports the documented P0 families through user configuration.
