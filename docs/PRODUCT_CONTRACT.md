# Product Contract

## Objective

Deliver CookScope v0.1.0 as a real Unreal Engine 5.8 Editor plugin, commandlet, and minimal reproducible sample project that explains typed asset dependency paths, audits configurable rules and actual Cook output, compares deterministic snapshots, exports CI reports, and provides a usable Slate workflow.

## Done when

Completion requires every P0 row in `ACCEPTANCE_MATRIX.md` to be supported by fresh reproducible evidence, a final independent read-only audit with no Blocker or High finding, a merged PR, annotated `v0.1.0` tag, and a public source-only GitHub Release whose custom asset list is empty.

## Constraints

- Engine: installed Unreal Engine 5.8.0, changelist 55116800.
- Platform/toolchain: Windows, MSVC, Windows SDK 10.0.26100.0, PowerShell 7.
- Build priority: MQB where it is correct and repeatable; UBT/RunUAT at verified Unreal-specific boundaries.
- One writing agent; independent final auditor is read-only.
- Analyzer behavior is read-only; no automatic asset or configuration mutation.
- Deterministic core behavior, strict versioned schemas, bounded graph traversal, stable output ordering, and distinct violation/internal-error exit codes.
- No binaries, cooked content, packages, large logs, traces, credentials, or custom Release assets in Git.
- If any P0 remains unverified, the project stays Alpha/WIP.

## P0 user outcomes

1. Explain why an asset is cooked through typed Hard, Soft, Manage, or Searchable Name paths.
2. Detect naming, path, dependency-boundary, cycle, Asset Manager, Chunk/Bundle, unexpected-Cook, and measurable resource-budget violations.
3. Compare compatible baseline and candidate snapshots and distinguish new, removed, changed, renamed/redirected, newly violated, fixed, and non-worsened findings.
4. Produce semantically consistent JSON, SARIF, JUnit, and offline self-contained HTML from one canonical result.
5. Run from a responsive Editor tab, Data Validation, and a strict unattended commandlet with reliable CI exit codes.
6. Reproduce real Asset Registry scanning, actual Sample Project Cook, BuildPlugin, Automation, local packaging, and clean-source/fresh-extraction smoke tests.

## Non-goals

CookScope does not reimplement Cooker, UnrealPak, Asset Registry, DDC, a custom asset format, a cloud/backend service, authentication, commercial features, or an asset-management platform. P1 Insights or interactive graph work cannot begin until every P0 gate passes.

## Provenance and authorship

The user defined the career direction, product direction, constraints, deadline, release policy, and acceptance target. Codex GPT-5.6 Sol performs architecture refinement, code, tests, debugging, Cook/report verification, audit, and documentation. The delivery must never be described as independently handwritten by the user.

