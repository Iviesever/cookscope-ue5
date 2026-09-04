# Live Change Drills

Each drill follows RED → implementation → focused test → full Core/UE regression → evidence → local commit. Remote push/PR steps require separate authorization.

## 1. Add an asset naming rule

Add an isolated positive/negative fixture to `RuleEngineContractTests.cpp`, extend strict parameter validation, implement evaluation in `rules.cpp`, add a help page, and prove Data Validation/Commandlet reuse.

## 2. Add a directory budget

Write an aggregate test with measured and unavailable assets, define overflow/incomplete semantics, implement a single-pass grouped sum, then verify report evidence and failure threshold behavior.

## 3. Change a dependency boundary

Create allowed and forbidden typed edges plus a stable path expectation in `DependencyRuleContractTests.cpp`. Update only the shared rule. Re-run Registry Automation to ensure UE category mapping stays unchanged.

## 4. Add a report field and migrate schema

First assert the field in canonical JSON and every affected projection. Introduce a new schema version or a backward-compatible optional field, update strict readers/writers and golden examples, then browser-test escaped rendering and Console output.

## 5. Establish a minimal baseline

Run a clean Cook on `main`, capture the Development Asset Registry, run the Commandlet to create a canonical snapshot, verify its source SHA, scrub host paths, commit the snapshot, and only then push `main` when authorized.

## 6. Start a real feature branch

Fetch the authenticated `origin/main`, verify its SHA, create `feat/cookscope-0.1` from that ref, and record both SHAs. Never simulate an origin with a local branch.

## 7. Create the Draft PR

After local gates pass, push the feature branch, create a Draft PR, attach exact commands/evidence, and keep it Draft until clean Cook, package, extraction, docs, and independent audit are green.
