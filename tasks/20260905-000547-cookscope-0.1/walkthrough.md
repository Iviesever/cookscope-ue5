# CookScope 0.1 Walkthrough

## Verified outcome

CookScope is implemented as a source-only Unreal Engine 5.8 plugin, Commandlet, Sample Project, deterministic pure C++ Core/CLI, real Asset Registry/Cook adapter, four-format reporter, and bounded Slate workflow. Production behavior was verified at implementation source `4eb0f34856217440c8beecc708f27abb281851f5` and replayed after the normal PR merge at `2c3ae0ad79730185ec42da6c5b2d1b5360d765b8`.

## Real verification output

Final Core/CLI replay:

```text
PASS: bootstrap status, exit code, and canonical JSON contract
PASS: strict bootstrap argument contract
PASS: strict full audit argument contract
PASS: strict JSON parser and canonical writer contract
PASS: strict versioned Rule Config contract
PASS: strict deterministic Asset Snapshot contract
PASS: public Schema and canonical example artifacts
PASS: deterministic typed dependency graph contract
PASS: shared naming, path, budget, scope, and measurement rule engine contract
PASS: dependency boundary, cycle, fan-out, and depth rule contract
PASS: texture, mesh, and sound metadata budget contract
PASS: Asset Manager, Cook, redirector, missing reference, and ambiguous name rules
PASS: directory, type, project, and incomplete Cook aggregate budgets
PASS: baseline suppression, worsening, and Soft-to-Hard contract
PASS: deterministic compatible snapshot diff contract
PASS: JSON, SARIF, JUnit, and self-contained HTML report contract
PASS: bootstrap CLI process, report bytes, and exit codes
```

Final UE replay:

```text
PASS: generated 20 deterministic CookScope fixture assets
CleanExitCode             : 0
ViolationExitCode         : 2
InvalidInvocationExitCode : 3
InternalErrorExitCode     : 4
CancelledExitCode         : 5
BlockingExitCode          : 2
NonBlockingExitCode       : 0
TimeoutExitCode           : 5
HardTimeoutExitCode       : 5
HardTimeoutSeconds        : 17.189
ForeignDirectoryExitCode  : 4
WrapperCleanExitCode      : 0
WrapperViolationExitCode  : 2
CandidateAssets           : 9
CandidateActualCookedAssets : 3
AddedCandidateCookedBytes : 892
DiffAssetChanges          : 1
PASS: UE build, fixtures, PACT Automation, Registry, Data Validation, bootstrap/full audit, real Cook diff, and reports
```

Fresh no-`.git` source replay:

```text
sourceSha      : 4eb0f34856217440c8beecc708f27abb281851f5
archiveSha256  : CC2D98D697CCF7D0F26BC2AA5B55F0B59CA1DB9E49A150E94E7FCE2DC5A553E9
archiveBytes   : 451886
coreExitCode   : 0
ubtExitCode    : 0
fixtureExitCode: 0
editorExitCode : 0
```

Documentation replay:

```text
PASS: required docs, links, examples, HTML controls, path hygiene, and source-only boundary
```

## Change audit

- `Plugins/CookScope/Source/CookScopeCore`: strict deterministic JSON, snapshots, graphs, rules, diffs, and report projection shared by MQB and UE.
- `Plugins/CookScope/Source/CookScopeEditor`: real indexed Registry/Asset Manager scan, Data Validation reuse, asynchronous cancellable session, and production Slate panel.
- `Plugins/CookScope/Source/CookScopeCommandlet`: stable full audit invocation, Cook merge, deadline, and report publication behavior.
- `SampleProject`: twenty deterministic positive/negative fixtures plus real Development Cook metadata.
- `Tests/Core`, `Tests/CLI`, and `Tests/UE`: contract coverage across the pure Core, processes, UE Automation, Cook, diff, timeout, and output ownership.
- `Schemas`, `Examples`, `docs`, and `scripts`: public contracts, small canonical examples, portfolio documentation, reproducible build/test/package/smoke workflows.

## Acceptance evidence

- Independent final read-only audit: 0 Blocker, 0 High.
- Real controlled baseline/candidate diff: exactly one added candidate asset and 892 actual-cooked bytes.
- Local BuildPlugin package source: `f26472ee1113de5d60b2a87e04dc23063595cd8f`.
- Fresh package ZIP SHA-256: `833F75A27DC38C2E26FF4F0FE79219D970F66701F505BA7C079BE0804E5ED888`.
- No package, binary, Cook output, log, or custom Release asset is tracked.
