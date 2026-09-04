# Interview Guide

## Core UE concepts

- **Asset Registry:** an indexed, mostly load-free view of assets, tags, packages, and dependency categories. CookScope converts it once into deterministic Core values.
- **Asset Manager:** policy/ownership above the Registry. Primary Assets have stable IDs and rules; Secondary assets are managed/referenced content.
- **Hard, Soft, Manage:** Hard is serialized object/package dependence, Soft is a deferred path reference, and Manage is Asset Manager ownership. Searchable Name is a separate Registry category.
- **Cook:** transforms project content for a target platform. CookScope reads the Development Asset Registry for real package membership/bytes.
- **Pak/IoStore:** later staging/container layers. CookScope v0.1 stops at Cook metadata; it does not infer container size.
- **Chunk and Bundle:** Chunk is packaging placement policy; Bundle is a named Primary Asset grouping. Both are retained per asset and exposed in Editor/HTML.
- **Data Validation:** `UCookScopeValidator` maps shared Core findings to UE valid/warning/invalid results.
- **Commandlet:** headless Editor entry for CI with strict arguments, atomic reports, deadlines, and stable exit codes.

## Engineering decisions

- **Editor/runtime isolation:** only pure `CookScopeCore` is Runtime. Slate, Asset Registry tooling, validation, tests, and Commandlet are Editor-only.
- **Slate:** the Nomad Tab is a thin presenter over `FCookScopeEditorSession`; rules/reporting never read widget state.
- **Asynchronous scan:** UE acquisition is scoped on the permitted thread; pure analysis owns a cancellable worker generation and is joined on shutdown.
- **Deterministic diff:** compatibility gates plus sorted value structures yield byte-stable output; renames require an explicit stable ID.
- **SARIF/JUnit:** SARIF is for code/security-style result viewers; JUnit is a broadly supported pass/fail testcase projection.
- **CI blocking:** each rule has Severity and FailThreshold; the process returns 2 only when configured blocking findings remain.
- **Estimated versus actual:** estimates are useful forecasts but are not Cook evidence. Only the Cook registry earns `actual-cooked`; absent facts stay unavailable.

## Demonstration path

Open the Editor tab, scan `/Game/CookScopeFixtures`, select `BadName`, explain `DA_Primary → DA_Target`, load the checked-in baseline, then export HTML. Follow with the full Commandlet and show exit 2 versus `-fail-on-violation=false` exit 0.
