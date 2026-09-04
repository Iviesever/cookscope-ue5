# Acceptance Matrix

Status is fail-closed: `NOT RUN`, `FAIL`, `BLOCKED`, or `PASS`. A source file existing is not evidence of behavior.

| ID | Gate | Required evidence | Status |
|---|---|---|---|
| PACT-00-01 | Public repository, local baseline, feature branch, Draft PR | GitHub URLs and SHAs | BLOCKED: user declined authentication |
| PACT-00-02 | Plugin builds and loads in UE 5.8 Editor | Build log, Editor log, screenshot, source SHA | PASS: repeated UBT/Editor-Cmd plus real `docs/images/editor.png` |
| PACT-00-03 | Editor tab opens | Automation/manual evidence and screenshot | PASS: Automation asserts a real `SCookScopePanel` Nomad Tab |
| PACT-00-04 | Commandlet JSON and stable exit codes | Three E2E invocations | PASS: real Editor-Cmd exits `0/2/3/4/5` |
| PACT-00-05 | MQB capability boundary | Clean/no-op/failure/identity matrix | NOT RUN |
| PACT-10-01 | Versioned snapshot and strict rule schemas | RED/GREEN core tests and canonical golden files | PASS: Core + public artifact round-trips; external Python validator unavailable |
| PACT-20-01 | Typed Asset Registry/Asset Manager graph | Real fixture scan and typed-edge tests | PASS: 9 graph fixtures plus 4 real resource assets; Hard/Soft/Manage/Searchable Name distinct and de-duplicated |
| PACT-20-02 | Direct/reverse/why-cooked/path/cycle/limit behavior | Deterministic graph tests and commandlet report | PASS: bounded Core tests + real Registry cycle/why-cooked Automation |
| PACT-30-01 | P0 naming/path/dependency/resource/Asset Manager/Cook rules | Positive/negative fixtures and rule tests | PASS: isolated Core families plus real Texture/StaticMesh/SkeletalMesh/Sound adapter Automation |
| PACT-30-02 | UE Data Validation reuses canonical rules | Editor validation automation | PASS: BadName Invalid and DA_Target Valid through shared Core |
| PACT-40-01 | Real Sample Project Cook | RunUAT/Editor Cook log plus actual output metadata | PASS: clean UE 5.8 Zen Cook, 495 packages, Registry + manifest verified |
| PACT-40-02 | Compatible deterministic baseline/candidate diff | Golden byte comparison and controlled fixtures | PASS: Core matrix + real Cook Added 892-byte candidate |
| PACT-50-01 | JSON/SARIF/JUnit validity and semantic consistency | Schema/parser tests and cross-report assertions | PASS: deterministic Core + independent JSON/XML parsing |
| PACT-50-02 | Self-contained responsive offline HTML | Browser desktop/narrow tests; console warning/error count 0 | PASS: desktop/narrow Chrome captures, live filters/expansion, Console `[]` |
| PACT-60-01 | Usable cancellable Slate tool and Editor actions | Automation, screenshots, cancel/unload evidence | PASS: scan/filter/diff/why-cooked/export/cancel/shutdown Automation plus real screenshot |
| PACT-70-01 | Strict commandlet/CI timeout and failure policy | Success/violation/config/internal/timeout E2E | PASS: full audit exits `2/0/5`, no timed-out partial files, atomic per-report replacement |
| PACT-70-02 | Complete core and UE Automation suites | Fresh logs bound to source SHA | NOT RUN |
| PACT-70-03 | BuildPlugin and local package | Paths, sizes, SHA-256, source SHA | PASS: source-bound package at `7f5d7c1`; no Release upload |
| PACT-70-04 | Fresh extraction and clean-source smoke | Commands, logs, checksums | NOT RUN |
| DOC-01 | Portfolio/interview/release documentation matches facts | Link and claim audit | PASS: `scripts/Test-Docs.ps1` checks required docs, links, formats, path hygiene, and source-only boundary |
| AUDIT-01 | Independent read-only audit has no Blocker/High | Auditor report and post-fix rerun | NOT RUN |
| RELEASE-01 | Merge commit, annotated v0.1.0 and source-only Release with zero assets | Git/GitHub evidence | BLOCKED: user declined authentication |
