# CookScope 0.1 Technical Blueprint

## Delivery shape

Build a source-only UE 5.8 sample repository containing `Plugins/CookScope`, `SampleProject`, pure-core tests and CLI probes, deterministic sample reports, PowerShell build/verification entry points, portfolio documentation, and ignored local evidence/packages. The plugin separates pure deterministic behavior, UE scanning/metadata, commandlet orchestration, and Editor presentation.

## Stable core interfaces

Namespace `cookscope` exposes value types `AssetId`, `AssetRecord`, `DependencyEdge`, `Snapshot`, `RuleConfig`, `Finding`, `AnalysisResult`, `SnapshotDiff`, and `OperationLimits`. Fallible operations return `Result<T>` with a stable error code and message; they do not throw. Core collections are sorted before observation and serialization.

Primary operations:

```cpp
Result<RuleConfig> ParseRuleConfig(std::string_view utf8Json);
std::string WriteCanonicalRuleConfig(const RuleConfig& config);
Result<Snapshot> ParseSnapshot(std::string_view utf8Json);
std::string WriteCanonicalSnapshot(const Snapshot& snapshot);
GraphResult BuildGraph(const Snapshot& snapshot, OperationLimits limits);
PathResult FindShortestPath(const DependencyGraph&, AssetId from, AssetId to, EdgeMask, OperationLimits);
PathsResult FindAllPaths(const DependencyGraph&, AssetId from, AssetId to, EdgeMask, OperationLimits);
std::vector<Cycle> FindCycles(const DependencyGraph&, EdgeMask, OperationLimits);
AnalysisResult Evaluate(const Snapshot&, const RuleConfig&, const std::optional<Snapshot>& baseline);
Result<SnapshotDiff> Diff(const Snapshot& baseline, const Snapshot& candidate);
ReportSet RenderReports(const AnalysisResult&, const ReportOptions&);
```

## UE boundary

`FCookScopeAssetScanner` captures `FAssetData`, official dependency categories, referencers, Primary Asset IDs, bundles, chunks, redirectors, package sizes, and availability/provenance. It converts once into immutable core records. `UCookScopeAuditCommandlet`, `UCookScopeEditorSubsystem`, `UCookScopeValidator`, and `SCookScopePanel` consume shared services; no rule logic lives in those classes.

Long scans use a generation-owned worker future and cancellation token. UObject/Asset Registry access is marshalled on permitted threads; pure graph/rule/report computation runs off the game thread. Shutdown cancels, joins, unregisters delegates, and releases Slate commands in deterministic order.

## Sample and Cook strategy

A fixture-builder commandlet deterministically generates tiny project-owned assets with stable manifest IDs. Automation first asserts fixture presence and semantics, then a real Windows Cook captures `Development` actual Cook metadata. A controlled candidate mutation command produces added/fixed/worsened cases without committing Cook output. Clean-source scripts regenerate fixtures, build, scan, Cook, diff, report, package, extract, and smoke-test.

## Reporting and UI

JSON is the canonical result. SARIF, JUnit, and HTML are projections tested against it. HTML embeds escaped JSON, CSS, and JavaScript without network references. Browser QA covers filtering, search, dependency expansion, size sorting, comparison, chunk/bundle views, responsive layout, and zero console warnings/errors. Slate mirrors those user outcomes while delegating analysis/export to shared services.

## Error/exit policy

The commandlet rejects unknown arguments and ambiguous paths. Exit codes are `0` clean success, `2` threshold violation, `3` invocation/configuration error, `4` internal/IO/scan/report error, and `5` timeout/cancellation. It writes reports atomically to the explicit output directory and normalizes unrelated host paths in committed output.

## Verification sequence

1. PACT-00 bootstrap and bounded MQB/UE capability matrix.
2. Pure core RED/GREEN cycles for schema, graph, rules, diff, and reports.
3. UE adapter fixtures, Asset Registry/Manager, Data Validation, commandlet, and Editor tests.
4. Actual Cook, baseline/candidate evidence, all report/browser checks, packaging, clean-source replay.
5. Documentation claim audit and independent read-only repository audit.
6. Only when all P0 is green: remote merge/tag/source-only Release; otherwise retain Alpha/WIP.

## Blueprint self-review

- Red-flag token scan: no deferred acceptance language is present.
- Consistency: the module/data-flow/error/test sections use the same shared-core boundary and exit policy.
- Scope: P1 is explicitly excluded until all P0 passes.
- Ambiguity: GitHub-unavailable work is marked blocked, while local Git and verification continue.
