# Architecture

## Selected approach

CookScope uses one plugin with four responsibility-separated modules and one shared pure C++ model/rule/report core:

```text
Sample Project / UE 5.8
        |
        +-- CookScopeCore (Runtime-safe UE module shell)
        |      `-- Pure C++ deterministic model, graph, rules, diff, reports
        |
        +-- CookScopeCommandlet (Editor module)
        |      `-- strict CLI -> UE scan/cook provenance -> canonical result -> exit code
        |
        `-- CookScopeEditor (Editor module)
               |-- Asset Registry / Asset Manager adapter
               |-- Data Validation adapter
               `-- async bounded scan + Slate presentation/actions

        `-- CookScopeTests (Editor development Automation module)
```

Pure C++ translation units avoid Unreal types, exceptions, RTTI, global mutable state, wall-clock decisions, and unordered serialization. MQB compiles those exact files into the standalone test/CLI target. UBT compiles the same files inside `CookScopeCore`; UE-only adapters translate `FAssetData`, dependency categories, Primary Asset data, and Cook metadata at the boundary.

## Considered alternatives

1. **Selected: shared pure C++ core inside a layered UE plugin.** Highest determinism and MQB coverage while retaining official UE adapters. It requires explicit UTF-8/path conversions at one boundary.
2. **Rejected: monolithic Editor-only plugin.** Faster scaffolding, but rules would drift into Slate/commandlet code, Runtime isolation would be unprovable, and MQB could not own meaningful targets.
3. **Rejected: external analyzer plus a thin UE exporter.** Excellent isolation, but duplicates orchestration, weakens Data Validation reuse, and makes interactive Editor workflows depend on an extra executable.

## Components

### Pure core

- `Model`: versioned snapshots, assets, typed edges, Primary Asset/Bundle/Chunk metadata, findings, limits, and provenance.
- `Json`: strict duplicate-key/unknown-field-aware parsing plus stable canonical writing.
- `Graph`: deterministic adjacency indexes, direct/reverse queries, bounded shortest/all paths, Tarjan strongly connected components, and explicit truncation/error metadata.
- `Rules`: compiled include/exclude scopes, stable Rule IDs, exceptions, baseline policy, severity and fail thresholds.
- `Diff`: compatibility gate followed by stable asset/edge/metadata/size/finding comparison and explicit uncertainty.
- `Reports`: JSON is canonical; SARIF, JUnit, and HTML consume the same immutable result.

### UE adapters

- Registry scan maps official UE dependency categories without collapsing edge types.
- Asset Manager enrichment adds Primary Asset IDs, management edges, bundles, and chunks.
- Cook snapshot reader distinguishes source/package/estimated/actual-cooked sizes and records platform/configuration.
- Data Validation and commandlet call the same rule engine.
- Slate owns presentation and cancellation only; it never owns rule semantics.

## Data flow

```text
Strict config
    + UE Asset Registry/Asset Manager scan
    + optional actual Cook metadata
    + optional compatible baseline
        -> immutable canonical analysis request
        -> graph + rules + diff
        -> immutable canonical result
        -> JSON / SARIF / JUnit / HTML / Slate / CI exit policy
```

## Error and boundary policy

- Invalid config, unknown fields, duplicate Rule IDs, illegal thresholds, corrupt snapshots, schema mismatch, incompatible engine/platform/cook settings, overflowed limits, and report serialization failure fail closed.
- Bounded graph operations always report `complete`, `truncated`, or `failed`; truncation is never silently presented as a complete answer.
- Commandlet result codes are stable: `0` success, `2` policy violation, `3` invalid invocation/config, `4` internal/IO/scan error, and `5` timeout/cancel.
- Editor operations are cancellable and have explicit lifetime ownership. Registry acquisition is scoped and synchronous on the permitted Editor thread; traversal/rule/diff/report work runs off-thread and drains before module shutdown.

## Test strategy

- MQB pure-core tests: parser, canonical JSON, graph/cycle/path limits, rules, budgets, diff, reports, Unicode/path case.
- UBT/Automation tests: UE adapter typing, fixture generation, Asset Registry, Asset Manager, Data Validation, commandlet outcomes, Editor tab/actions/cancel/unload.
- E2E: real Cook, report browser desktop/narrow viewport with zero console warnings/errors, BuildPlugin, local package, clean extraction, and clean source checkout.
