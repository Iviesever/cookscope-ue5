# CookScope 0.1 Implementation Plan

> **For the implementing agent:** Required process Skill is `executing-tasks`; use TDD for every production behavior and track each step with checkboxes. Exactly one agent writes this repository.

**Goal:** Deliver the verified CookScope UE 5.8 plugin, commandlet, sample, deterministic reports, local packages, audit, and source-only release defined by the product contract.

**Architecture:** A pure C++ deterministic model/graph/rule/diff/report core is compiled by both MQB and the UE `CookScopeCore` module. UE-only Editor/commandlet adapters provide real registry, manager, validation, Cook, and Slate workflows without duplicating rules.

**Technical stack:** C++20, Unreal Engine 5.8.0, MSVC, MQB 5.4.0, UBT/RunUAT, Slate, AssetRegistry, AssetManager, DataValidation, AutomationSpec, PowerShell 7, JSON/SARIF/JUnit/HTML.

## Global constraints

- MQB is the first build path for supported pure C++ targets; identical clean and no-op commands must be recorded.
- UHT, UE modules, Editor, commandlet, Automation, BuildPlugin, Cook/Stage/Pak/IoStore may use official UE tools after the boundary probe.
- No estimated size may be labeled actual cooked size.
- Every traversal is stably ordered and bounded by nodes, edges, depth, path count, and timeout.
- Analyzer behavior is read-only; no automatic user asset/config mutation.
- Release assets are empty; local generated packages stay in ignored `Artifacts/`.
- `PASS` requires fresh command output and artifact identity bound to a source SHA.

---

### Task 1: PACT-00 bootstrap, MQB boundary, plugin load, tab, and commandlet

**Files:** create `mqb.json`, `Source/Core/**`, `Tests/Core/**`, `Tools/CookScopeCli/main.cpp`, `Plugins/CookScope/CookScope.uplugin`, module Build.cs/C++ files, `SampleProject/CookScopeSample.uproject`, targets/config, and `scripts/Build.ps1`, `scripts/Test.ps1`, `scripts/Probe-Mqb.ps1`.

**Interfaces:** produce the `cookscope::Result<T>` error contract, a deterministic minimal JSON result, `FCookScopeCoreModule`, `FCookScopeEditorModule`, `UCookScopeAuditCommandlet`, and a tab spawner named `CookScope`.

- [ ] Write core and UE smoke tests before their implementations; run MQB/UBT/Automation commands and record the expected missing-symbol/module/tab/commandlet RED failures.
- [ ] Implement only enough core/plugin/sample code to build, load the Editor, open the tab, and emit JSON with exit codes `0`, `2`, `3`, and `4` under controlled cases.
- [ ] Run `mqb build --profile release --timings=json` twice, an intentional compile failure probe, UBT Editor Development, `RunUAT.bat BuildPlugin`, Editor load, Automation, and commandlet E2E; record exact artifact identities.
- [ ] Commit as focused bootstrap/capability slices after each RED/GREEN checkpoint.

### Task 2: PACT-10 strict versioned models and canonical JSON

**Files:** create focused files under `Source/Core/Model`, `Source/Core/Json`, and `Tests/Core/ModelJsonTests.cpp`; add schema examples under `Schemas/` and `Examples/`.

**Interfaces:** implement the blueprint's `AssetRecord`, `Snapshot`, `RuleConfig`, `Finding`, parse/write APIs, schema identifiers `cookscope.snapshot/1`, `cookscope.rules/1`, and `cookscope.result/1`.

- [ ] Add failing tests for every required field, duplicate keys/Rule IDs, unknown fields, illegal thresholds/budgets, Unicode, case policy, corrupt JSON, stable ordering, and byte-identical canonical output.
- [ ] Observe each RED because behavior is absent, not because the test cannot compile for an unrelated reason.
- [ ] Implement the smallest parser/model/writer increments and rerun the same targeted tests to GREEN, followed by the full MQB core suite.
- [ ] Commit parser, model, and canonical writer as separately reviewable slices.

### Task 3: PACT-20 deterministic typed dependency graph

**Files:** create `Source/Core/Graph/**`, `Tests/Core/GraphTests.cpp`, UE registry/manager adapters under `Plugins/CookScope/Source/CookScopeEditor/Private/Scan/**`, and UE automation tests.

**Interfaces:** typed `DependencyKind` bitmask, direct/reverse queries, bounded shortest/all paths, why-cooked root explanation, Tarjan cycles, truncation metadata, Primary Asset/Bundle/Chunk enrichment, and unresolved-edge records.

- [ ] Add RED fixtures for Hard/Soft/Manage/Searchable Name edges, stable order, cycles, cross-boundary paths, missing targets, and node/edge/depth/path limits.
- [ ] Implement pure graph behavior to targeted GREEN and run the full MQB suite.
- [ ] Add a deterministic UE fixture builder and observe RED for missing real Asset Registry/Manager mappings before implementing adapters.
- [ ] Run real Editor-Cmd fixture generation and scan Automation; save a canonical snapshot and typed path evidence, then commit.

### Task 4: PACT-30 rules and Data Validation

**Files:** create `Source/Core/Rules/**`, per-family tests, default `Config/CookScopeRules.json`, UE metadata extractors, and `UCookScopeValidator`.

**Interfaces:** compile/evaluate strict rule configs into stable findings with measurement kind, baseline state, dependency path, rule help, severity, and threshold decision.

- [ ] Add RED cases for every P0 naming/path, dependency, budget, Asset Manager, and Cook rule including unavailable measurements and baseline non-worsening.
- [ ] Implement one rule family at a time to targeted GREEN and full-suite GREEN.
- [ ] Add RED UE tests proving Data Validation does not yet reuse findings, implement the adapter, then run Validate Assets/Automation to GREEN.
- [ ] Commit each rule family and the Data Validation adapter independently.

### Task 5: PACT-40 actual Cook snapshot and deterministic diff

**Files:** create `Source/Core/Diff/**`, `Tests/Core/DiffTests.cpp`, Cook metadata adapter, controlled fixture mutation commandlet, and `scripts/Cook.ps1`, `scripts/Capture-Snapshot.ps1`, `scripts/Diff.ps1`.

**Interfaces:** compatibility gate plus stable added/removed/modified/renamed/redirected asset, edge type, size, directory/type budget, chunk, bundle, Primary Asset, and finding delta records.

- [ ] Add RED tests for every required delta, engine/platform/config incompatibility, missing data, schema migration, Unicode/case policy, and deterministic bytes.
- [ ] Implement compatible diff behavior to GREEN and full MQB regression GREEN.
- [ ] Run a real Development Windows Sample Cook, capture actual Cook metadata separately from estimates, apply the controlled candidate mutation, Cook again, and prove expected deltas.
- [ ] Save only small canonical examples in Git; record ignored Cook paths/checksums and commit the diff slice.

### Task 6: PACT-50 JSON, SARIF, JUnit, and offline HTML

**Files:** create `Source/Core/Reports/**`, report tests/goldens, `Schemas/cookscope-result.schema.json`, and browser validation scripts.

**Interfaces:** `RenderReports` projects one immutable `AnalysisResult` to valid JSON, SARIF 2.1.0, JUnit XML, and one self-contained HTML file.

- [ ] Add RED validity and cross-format semantic tests, escaping/injection tests, Unicode tests, deterministic golden comparisons, and no-external-resource assertions.
- [ ] Implement JSON, SARIF, JUnit, then HTML writers one at a time to targeted/full GREEN.
- [ ] Validate HTML in a real browser at desktop and narrow viewport for all filters/search/expansion/sorting/comparison/chunk-bundle behaviors and zero console warnings/errors.
- [ ] Commit small real sample reports and screenshots tied to source SHA.

### Task 7: PACT-60 production Slate workflow

**Files:** create editor subsystem/service/view-model/widgets/commands under `Plugins/CookScope/Source/CookScopeEditor`, styles/resources, and Editor automation tests.

**Interfaces:** scan scope/config selection, progress/cancel, filters/search, finding/path/why-cooked expansion, baseline comparison, size/chunk/bundle views, Content Browser sync, asset open, and four-format export.

- [ ] Add RED automation for tab/view-model data, filter semantics, cancellation, Content Browser/action dispatch, no dangling work, and unload cleanup.
- [ ] Implement service/view-model before widgets; run targeted Automation to GREEN after every behavior.
- [ ] Build the restrained Slate presentation, verify responsive Editor layout and real actions, then rerun commandlet/core guardrails.
- [ ] Capture real Editor screenshots only after the source SHA is fixed; commit code and small screenshots.

### Task 8: PACT-70 CI, packaging, clean-source, and documentation

**Files:** finish strict scripts, GitHub workflow source, all required `docs/*.md`, `README.md`, `README_ZH.md`, sample outputs/screenshots, release notes, interview guide, and live-change drills.

**Interfaces:** one bounded `scripts/Verify.ps1` orchestrates MQB, UBT, Automation, commandlet cases, Cook/diff/reports, BuildPlugin, package checksums, fresh extraction, clean-source replay, and claim linting.

- [ ] Add RED checks for unknown commandlet arguments, every exit code, timeout, privacy normalization, invalid reports, stale evidence, tracked generated files, and prohibited Release assets.
- [ ] Implement orchestration and documentation until all local P0 checks pass from a clean source clone/extraction.
- [ ] Record package paths, sizes, SHA-256 values, source SHA, exact commands, and fresh extraction results without committing packages.
- [ ] Commit complete fact-checked portfolio documentation only after its referenced evidence exists.

### Task 9: Independent audit and release gate

**Files:** create `tasks/20260905-000547-cookscope-0.1/audit.md`; update acceptance, progress, handoff, and release notes.

**Interfaces:** read-only audit grades Blocker/High/Medium/Low and maps every P0 requirement to source plus fresh evidence.

- [ ] Dispatch exactly one independent audit agent with explicit read-only constraints; do not allow it to edit or change Git/GitHub state.
- [ ] Reproduce and fix only confirmed findings through new RED/GREEN slices, then rerun the complete verification pipeline.
- [ ] If any P0 or Blocker/High remains, retain Alpha/WIP and do not tag/release.
- [ ] Only with usable user-authorized GitHub authentication and every gate GREEN: make PR ready, merge normally, delete feature branch, create annotated `v0.1.0`, publish source-only Release, and verify custom assets are empty.

