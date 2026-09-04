# Acceptance Matrix

Status is fail-closed: `NOT RUN`, `FAIL`, `BLOCKED`, or `PASS`. A source file existing is not evidence of behavior.

| ID | Gate | Required evidence | Status |
|---|---|---|---|
| PACT-00-01 | Public repository, local baseline, feature branch, Draft PR | GitHub URLs and SHAs | BLOCKED: user declined authentication |
| PACT-00-02 | Plugin builds and loads in UE 5.8 Editor | Build log, Editor log, screenshot, source SHA | PASS: UBT and Editor-Cmd at `4dbbbae`; final portfolio screenshot remains DOC scope |
| PACT-00-03 | Editor tab opens | Automation/manual evidence and screenshot | PASS: Automation invoked and closed real Nomad Tab at `4dbbbae` |
| PACT-00-04 | Commandlet JSON and stable exit codes | Three E2E invocations | PASS: real Editor-Cmd exits `0/2/3/4/5` at `4dbbbae` |
| PACT-00-05 | MQB capability boundary | Clean/no-op/failure/identity matrix | NOT RUN |
| PACT-10-01 | Versioned snapshot and strict rule schemas | RED/GREEN core tests and canonical golden files | PASS: Core + public artifact round-trips; external Python validator unavailable |
| PACT-20-01 | Typed Asset Registry/Asset Manager graph | Real fixture scan and typed-edge tests | PASS: 7 real UE assets; Hard/Soft/Manage/Searchable Name distinct |
| PACT-20-02 | Direct/reverse/why-cooked/path/cycle/limit behavior | Deterministic graph tests and commandlet report | PASS: bounded Core tests + real Registry cycle/why-cooked Automation |
| PACT-30-01 | P0 naming/path/dependency/resource/Asset Manager/Cook rules | Positive/negative fixtures and rule tests | NOT RUN |
| PACT-30-02 | UE Data Validation reuses canonical rules | Editor validation automation | NOT RUN |
| PACT-40-01 | Real Sample Project Cook | RunUAT/Editor Cook log plus actual output metadata | NOT RUN |
| PACT-40-02 | Compatible deterministic baseline/candidate diff | Golden byte comparison and controlled fixtures | NOT RUN |
| PACT-50-01 | JSON/SARIF/JUnit validity and semantic consistency | Schema/parser tests and cross-report assertions | NOT RUN |
| PACT-50-02 | Self-contained responsive offline HTML | Browser desktop/narrow tests; console warning/error count 0 | NOT RUN |
| PACT-60-01 | Usable cancellable Slate tool and Editor actions | Automation, screenshots, cancel/unload evidence | NOT RUN |
| PACT-70-01 | Strict commandlet/CI timeout and failure policy | Success/violation/config/internal/timeout E2E | NOT RUN |
| PACT-70-02 | Complete core and UE Automation suites | Fresh logs bound to source SHA | NOT RUN |
| PACT-70-03 | BuildPlugin and local package | Paths, sizes, SHA-256, source SHA | PASS: source-bound package at `7f5d7c1`; no Release upload |
| PACT-70-04 | Fresh extraction and clean-source smoke | Commands, logs, checksums | NOT RUN |
| DOC-01 | Portfolio/interview/release documentation matches facts | Link and claim audit | NOT RUN |
| AUDIT-01 | Independent read-only audit has no Blocker/High | Auditor report and post-fix rerun | NOT RUN |
| RELEASE-01 | Merge commit, annotated v0.1.0 and source-only Release with zero assets | Git/GitHub evidence | BLOCKED: user declined authentication |
