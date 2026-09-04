# Acceptance Matrix

Status is fail-closed: `NOT RUN`, `FAIL`, `BLOCKED`, or `PASS`. A source file existing is not evidence of behavior.

| ID | Gate | Required evidence | Status |
|---|---|---|---|
| PACT-00-01 | Public repository, local baseline, feature branch, Draft PR | GitHub URLs and SHAs | BLOCKED: user declined authentication |
| PACT-00-02 | Plugin builds and loads in UE 5.8 Editor | Build log, Editor log, screenshot, source SHA | PASS: repeated UBT/Editor-Cmd plus real `docs/images/editor.png` |
| PACT-00-03 | Editor tab opens | Automation/manual evidence and screenshot | PASS: Automation asserts a real `SCookScopePanel` Nomad Tab |
| PACT-00-04 | Commandlet JSON and stable exit codes | Three E2E invocations | PASS: real Editor-Cmd exits `0/2/3/4/5` |
| PACT-00-05 | MQB capability boundary | Clean/no-op/failure/identity matrix | PASS: source `f26472e`, clean 10 misses, no-op 10/10 hits, intentional failure 4, stable SHA-256 |
| PACT-10-01 | Versioned snapshot and strict rule schemas | RED/GREEN core tests and canonical golden files | PASS: Core + public artifact round-trips; external Python validator unavailable |
| PACT-20-01 | Typed Asset Registry/Asset Manager graph | Real fixture scan and typed-edge tests | PASS: all 20 assets; Bundle and non-Bundle recursive Manage ownership, Hard/Soft/Searchable Name, stable de-duplication |
| PACT-20-02 | Direct/reverse/why-cooked/path/cycle/limit behavior | Deterministic graph tests and commandlet report | PASS: bounded Core tests + real Registry cycle/why-cooked Automation |
| PACT-30-01 | P0 naming/path/dependency/resource/Asset Manager/Cook rules | Positive/negative fixtures and rule tests | PASS: isolated Core plus 20 real Sample assets; 23 failing P0 rule IDs and legal positive paths verified in UE Automation |
| PACT-30-02 | UE Data Validation reuses canonical rules | Editor validation automation | PASS: BadName Invalid and DA_Target Valid through shared Core |
| PACT-40-01 | Real Sample Project Cook | RunUAT/Editor Cook log plus actual output metadata | PASS: candidate clean Cook 506 total/499 cooked, 0 errors/warnings; Registry + manifest verified |
| PACT-40-02 | Compatible deterministic baseline/candidate diff | Golden byte comparison and controlled fixtures | PASS: real baseline `66256ac` and candidate `f204b3b`; only Added 892-byte candidate |
| PACT-50-01 | JSON/SARIF/JUnit validity and semantic consistency | Schema/parser tests and cross-report assertions | PASS: deterministic Core + independent JSON/XML parsing |
| PACT-50-02 | Self-contained responsive offline HTML | Browser desktop/narrow tests; console warning/error count 0 | PASS: desktop/narrow Chrome captures, live filters/expansion, Console `[]` |
| PACT-60-01 | Usable cancellable Slate tool and Editor actions | Automation, screenshots, cancel/unload evidence | PASS: indexed bounded acquisition, async Core, cancel/shutdown, filters/diff/why-cooked/export, real screenshot |
| PACT-70-01 | Strict commandlet/CI timeout and failure policy | Success/violation/config/internal/timeout E2E | PASS: cooperative and hard process-tree timeout exit 5; no partial files; atomic report replacement |
| PACT-70-02 | Complete core and UE Automation suites | Fresh logs bound to source SHA | PASS: source `f26472e`, 16 Core + CLI, 7 UE PACT, transactional timeout and real-diff E2E |
| PACT-70-03 | BuildPlugin and local package | Paths, sizes, SHA-256, source SHA | PASS: source `f26472e`, 84 files, 229,251,282 bytes; no Release upload |
| PACT-70-04 | Fresh extraction and clean-source smoke | Commands, logs, checksums | PASS: source `f26472e`; package ZIP and no-`.git` source archive both load/build/test cleanly |
| DOC-01 | Portfolio/interview/release documentation matches facts | Link and claim audit | PASS: final `scripts/Test-Docs.ps1` validates docs, links, formats, path hygiene, and source-only boundary |
| AUDIT-01 | Independent read-only audit has no Blocker/High | Auditor report and post-fix rerun | FAIL: first rerun closed 5/6 original High but found 2 High; both fixed, final rerun pending |
| RELEASE-01 | Merge commit, annotated v0.1.0 and source-only Release with zero assets | Git/GitHub evidence | BLOCKED: user declined authentication |
