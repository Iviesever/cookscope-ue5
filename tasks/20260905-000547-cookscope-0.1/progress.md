# Progress

## 2026-09-05 — State recovery

- Target local directory was absent; initialized the new dedicated repository at `D:\program\cookscope-ue5` on local `main` without touching existing directories.
- Git author: `Iviesever <3549874980@qq.com>`.
- GitHub CLI configured account name: `Iviesever`; authentication token was invalid. The user explicitly declined login, so no further authentication is permitted and remote repository/PR/Release gates remain blocked.
- Target GitHub repository was not found by `gh repo view`; no public repository state was changed.
- UE 5.8.0 CL 55116800 verified at `D:\program\UnrealEngine\Epic Games\UE_5.8`; Editor, Editor-Cmd, UBT, RunUAT, and ShaderCompileWorker paths exist.
- Visual Studio Community 2026 18.7.3; MSVC 14.44 and 14.51; Windows SDK 10.0.26100.0; PowerShell 7.6.0; Python 3.13.0; Git 2.51.2; MQB help reports 5.4.0.
- `mqb --version` is unsupported and correctly returned an unknown-option error; `mqb --help` is the version authority.
- No UE/UBT/RunUAT/Cook/ShaderCompileWorker build process was active during the snapshot. The inspecting PowerShell process was the only regex match and was not terminated.
- Repository contract, architecture, rule model, acceptance matrix, and executable task plan created. No production functionality is claimed.

## Current PACT slice

```text
Objective: Implement PACT-30 shared rule evaluation in reviewable rule-family slices.
Gap: Rule configs and snapshots parse, but they did not produce findings, measurement diagnostics, or scope decisions.
Scope: Shared scope/glob and finding model first; naming/path/budget, dependency, resource metadata, then Asset Manager/Cook families.
Done when: every P0 rule family has positive/negative fixtures, stable findings, explicit unavailable data, and UE Data Validation reuse.
```

## 2026-09-05 — PACT-30 shared scope, naming, path, and asset-size rules

- RED: `mqb run Tests/Core/RuleEngineContractTests.cpp --no-discover -I Plugins/CookScope/Source/CookScopeCore/Public --std 20 --profile release -o CookScopeRuleEngineContractTests` exited `1` with C1083 because `cookscope/rules.h` did not exist.
- GREEN: the test plus `rules.cpp`, strict Rule Config, and JSON sources exited `0` and printed `PASS: shared naming, path, budget, scope, and measurement rule engine contract`.
- Proven behavior: deterministic `**`/`*`/`?` globs, Include then Exclude then explicit Exception precedence, type-specific asset prefixes, forbidden paths, and per-asset byte budgets.
- Budget findings preserve requested measurement kind, observed bytes, and limit. Missing actual-cooked data produces stable `MeasurementUnavailable` diagnostics rather than a zero or estimate.
- Findings and diagnostics sort by Rule ID, Asset Path, then message. Unsupported/invalid parameter diagnostics are explicit.
- Fresh `scripts/Test.ps1`: exit `0`; eight Core/file test targets plus process CLI passed; MQB discovered exactly eight production translation units.

## 2026-09-05 — PACT-30 dependency rule family

- RED: `DependencyRuleContractTests` failed compilation because shared Finding lacked related asset, typed dependency kind, and dependency path evidence fields.
- GREEN: forbidden boundary, Runtime-to-Editor, cycle, maximum fan-out, and maximum depth produced five stable findings with zero diagnostics.
- Forbidden-edge finding retains source, related target, Hard kind, and one-step evidence. Maximum-depth finding retains the deterministic Soft -> Hard two-step violating path.
- The first full suite after implementation failed only because the pre-existing RuleEngine test target's exact source set had not added `graph.cpp`; its three unresolved graph symbols were corrected by updating the build target, not implementation code.
- Repeated `scripts/Test.ps1`: exit `0`; all rule/graph/core/file/CLI tests passed with exactly eight MQB production translation units.
- UE Editor Development build then compiled and linked `rules.cpp` into CookScopeCore successfully, covering the UBT Unity/DLL path before commit.

## 2026-09-05 — PACT-30 resource metadata rules

- RED: `ResourceRuleContractTests` failed compilation because Finding lacked metric, numeric observed/limit, and text observed/expected evidence fields.
- GREEN: Texture width, height, Mip count, and format; Static Mesh triangles; Skeletal Mesh vertices; Sound duration and format produced eight measured findings with zero diagnostics.
- Numeric metadata findings keep a metric and unsigned observed/limit values while measurement kind stays `Unavailable`; they are not mislabeled as disk or actual-cooked byte sizes. Format findings keep explicit observed/allowed text.
- Missing or malformed resource tags produce `MeasurementUnavailable` diagnostics rather than zero-valued violations.
- Fresh `scripts/Test.ps1`: exit `0`; all Core/file/rule/CLI tests passed, including the new resource target; MQB production discovery remains eight translation units.

## 2026-09-05 — PACT-30 Asset Manager and Cook rule family

- RED: `AssetManagerCookRuleContractTests` built and ran, then failed because all twelve new Rule IDs were still reported as unsupported diagnostics.
- GREEN: Primary required/type, required Bundle, Chunk conflict/required, unexpected Cook, missing expected Cook, Editor-only Cook leak, AlwaysCook/NeverCook conflict, redirector, unresolved reference, and ambiguous naming each produced one isolated finding with zero diagnostics.
- Missing-reference findings retain unresolved target and dependency kind. Ambiguous-name findings choose the lexicographically first and related paths. Cook findings only treat `ActualCooked` measurements as proof of inclusion.
- Fresh full MQB suite exited `0`; all Core/file/rule/CLI tests passed. UE Editor Development then compiled and linked the expanded shared rules into CookScopeCore and exited `0`.

## 2026-09-05 — PACT-30 aggregate budgets

- RED: `AggregateBudgetRuleContractTests` built and ran, then failed because existing per-asset budget evaluation could not produce the expected aggregate results.
- GREEN: directory, asset type, and project package-disk totals were exactly 1,100, 1,100, and 1,400 bytes and produced stable aggregate findings against 1,000/1,000/1,300-byte budgets.
- A project actual-cooked aggregate encountered one unavailable Sound measurement, emitted a stable diagnostic for that asset, and emitted no partial-total finding even though known assets alone exceeded the threshold.
- Summation detects uint64 overflow and fails closed via diagnostics.
- Fresh `scripts/Test.ps1`: exit `0`; all Core/file/rule/aggregate/CLI tests passed with eight MQB production translation units.

## 2026-09-05 — PACT-30 baseline suppression and Soft-to-Hard worsening

- RED: `BaselineRuleContractTests` failed compilation because Finding lacked baseline state and `Evaluate` had no baseline overload.
- GREEN: new naming violation remained; unchanged naming violation was suppressed; package size growth 1,500 -> 2,000 bytes was `Worsened`; Soft -> Hard edge was a typed `Worsened` finding; `report-all` retained an unchanged forbidden-path finding as `Existing`.
- Finding baseline states are explicit `NotApplicable`, `New`, `Existing`, or `Worsened`. Numeric/byte/Severity increases count as worsening; identity uses Rule ID, asset, related asset, metric, and message.
- Fresh full MQB suite exited `0`. UE build waited for an unrelated Build.bat via the official mutex, then compiled and linked CookScopeCore successfully with exit `0`; no other process was terminated.

## 2026-09-05 — PACT-30 UE Data Validation reuse

- RED: SampleEditor Data Validation Automation failed compilation because `CookScopeValidator.h` did not exist.
- Implemented `UCookScopeValidator` against UE 5.8's `CanValidateAsset_Implementation(FAssetData, UObject, Context)` and `ValidateLoadedAsset_Implementation` signatures.
- Validator loads the plugin's strict `CookScopeRules.json`, scans through `FCookScopeAssetScanner`, evaluates through the shared Core, and maps error findings to `AssetFails`, lower severities to `AssetWarning`, and clean assets to `AssetPasses`. No naming rule logic is duplicated in the validator.
- First UBT failed because the public validator header exposed DataValidation while the module dependency was private; DataValidation moved to CookScopeEditor's Public dependencies. Second UBT compiled but SampleEditor directly called a DataValidation symbol without its own link dependency; adding the direct private dependency fixed the link. Final UBT exited `0`.
- Added eighth deterministic `BadName` fixture and plugin default prefix rule. Fixture regeneration twice produced `CHANGED_COUNT=0` across all eight assets; BadName is 1,381 bytes, SHA-256 `5BEE24387BD4EFB59B8563443F756AF89B1DE0ED3524EBCC86E8E885E370D0B3`.
- Unified UE run found exactly three PACT tests. `CookScope.PACT30.DataValidationReuse` reported `Result={Success}`: BadName returned Invalid with an error, while DA_Target returned Valid with zero errors. Automation and five-way Commandlet suite exited `0`.

## 2026-09-05 — PACT-40 real UE 5.8 Cook

- Cook process RED 1: `Tests/UE/CookContract.ps1` failed because `scripts/Cook.ps1` did not exist.
- Added a bounded `RunUAT BuildCookRun -clean -build -cook -skipstage -platform=Win64 -clientconfig=Development -unattended` entry. No Pak/IoStore stage or upload was requested.
- First real clean run exposed another Unity-only duplicate anonymous `ToUtf8` helper across AssetScanner/Validator. The helpers received responsibility-specific names; Unity was not disabled. The next UBT build exited `0`.
- Cook process RED 2: RunUAT itself completed `BUILD SUCCESSFUL`, 495 packages and 0 errors/warnings, but the test expected an old loose `DA_Primary.uasset`. UE 5.8 used Zen Store and correctly produced Asset Registry/metadata instead.
- The test was corrected to require `AssetRegistry.bin`, `Metadata/DevelopmentAssetRegistry.bin`, `Metadata/zenfs.manifest`, and Primary fixture identity inside the Development Registry. It does not use filename heuristics to fabricate loose package output.
- Final `CookContract.ps1` exited `0`: clean Game+Editor build succeeded; 495 packages cooked, 7 platform-skipped, 502 total; Cook commandlet reported 0 errors and 0 warnings.
- Real Zen output: 29 files, 40,305,689 bytes. Development Asset Registry: 408,532 bytes with 504 assets and Primary fixture identity. Zen manifest: 460,074 bytes.
- Cook log: ignored `Artifacts/Evidence/PACT-40/Cook/cook-4faa1276b5a2423d9631fb4bf3ff9ec2.log`. A final-source-SHA replay remains part of PACT-70.

## 2026-09-05 — PACT-40 deterministic snapshot diff core

- RED: `SnapshotDiffContractTests` failed C1083 because `cookscope/diff.h` did not exist.
- GREEN: compatible baseline/candidate fixtures produced Added, Removed, Modified, and StableAssetId Renamed asset changes; Soft -> Hard TypeChanged edge; added edge; five semantic modified fields; and exact +50 actual-cooked bytes.
- Compatibility gate fails closed for schema, engine version, platform, or Cook configuration mismatch while allowing source SHA changes. Reversed asset input order produced byte-identical canonical `cookscope.diff/1` JSON.
- Rename matching uses explicit `StableAssetId`; it does not guess rename from similar filenames. Cook size deltas require ActualCooked on both sides and signed-int64 representability.
- Fresh `scripts/Test.ps1`: exit `0`; all Core/file/rule/diff/CLI tests passed with nine MQB production translation units. UE Editor Development compiled and linked `diff.cpp` into CookScopeCore with exit `0`.
- Finding/Severity delta integration and a controlled real-Cook baseline/candidate pair remain outstanding; the PACT-40-02 aggregate gate stays fail-closed.

## 2026-09-05 — PACT-50 canonical JSON, SARIF, JUnit, and offline HTML core

- RED: `ReportContractTests` failed C1083 because `cookscope/reports.h` did not exist.
- GREEN: one immutable Snapshot/RuleConfig/AnalysisResult/Diff rendered byte-identical JSON, SARIF, JUnit, and HTML on repeated calls.
- Canonical JSON carries provenance, summary, rules, assets, findings, diagnostics, typed dependency paths, measurement/budget evidence, baseline state, and diff. SARIF reports version 2.1.0 with stable Rule IDs/messages/asset locations. JUnit uses one stable failing testcase per finding.
- HTML embeds the same canonical JSON, has no CDN/font/script/style URL, neutralizes `</script>` with `\\u003c`, and exposes severity/rule/class/path/size controls plus dependency details and size delta.
- Targeted test wrote ignored evidence: JSON 1,289 bytes SHA-256 `8EEE10F8243C65F449921FF89444896A27A07FCEF17E11F8B7873CC27B958014`; SARIF 538 `061523CB42429E261E3844576672F2C2D2D988DD5B27B4164BBCCBF973AD8E21`; JUnit 349 `FCEFC91F828678E16D0B9F1D3FD513325F4F1D9DAFC1C9BAA1731CECFC36152E`; HTML 5,759 `4BE6F0D4B16E5587D9ED7230DCC491430CBCFCDF2D1AF2DCF83AADA28F0B52C4`.
- Python standard `json` parsed JSON/SARIF and confirmed SARIF 2.1.0; `xml.etree.ElementTree` parsed JUnit and confirmed one failure. No external package was installed.
- First UBT report build found anonymous `DependencyName` collisions between Diff/Snapshot in Unity. Snapshot/Diff/Report/Rule formatter helpers received file-responsibility prefixes; UBT then exited `0` without disabling Unity.
- Real in-app browser loaded the localhost HTML at desktop and 390x844. Severity and Path Search hid/shown rows correctly; narrow controls formed two columns and table remained horizontally accessible. Console warning/error lists were empty before and after interactions. The temporary tab/server were closed.
- Chunk/Bundle-specific HTML presentation and full baseline/candidate interaction remain outstanding, so PACT-50-02 stays fail-closed despite the verified responsive foundation.

## 2026-09-05 — PACT-70 strict full-audit Commandlet

- Core argument RED: `AuditArgumentsContractTests` failed compilation because the full-audit parser, value object, and MissingConfig error did not exist.
- Core argument GREEN: strict config/output/source-SHA, optional scope/baseline, boolean fail policy, and 1-86400 second timeout parsed correctly; duplicate, unknown, invalid SHA, and zero timeout failed closed.
- E2E RED: real `CookScopeAudit` with full arguments returned `3` because only bootstrap mode existed.
- Full mode now executes strict args -> config -> real Asset Registry/Manager scan -> optional baseline/diff -> shared rule engine -> one canonical result -> JSON/SARIF/JUnit/HTML/Snapshot files -> per-rule fail threshold exit decision. Bootstrap fixture mode remains separate for its stable exit matrix.
- UBT compiled and linked the Commandlet's shared scanner/report dependencies with exit `0`.
- Real full-audit E2E GREEN: blocking run returned `2`; non-blocking run returned `0`; both scanned `/Game/CookScopeFixtures`. Five files were written, source SHA provenance matched Git HEAD, and canonical JSON contained exactly one real BadName `naming.asset-prefix` finding.
- Unified regression then exited `0`: complete MQB suite; UBT; 8 fixtures; three PACT Automation tests; bootstrap `0/2/3/4/5`; full audit `2/0`; report assertions.
- Timeout is checked fail-closed after synchronous scan/analysis and returns `5`; cancellable in-flight Editor scanning remains PACT-60 work.

## 2026-09-05 — PACT-20 deterministic typed graph core

- RED: `mqb run Tests/Core/GraphContractTests.cpp --no-discover -I Plugins/CookScope/Source/CookScopeCore/Public --std 20 --profile release -o CookScopeGraphContractTests` exited `1` with C1083 because `cookscope/graph.h` did not exist.
- GREEN: the test with `Private/graph.cpp` exited `0` and printed `PASS: deterministic typed dependency graph contract`.
- Proven behavior: stable sorted node/adjacency indexes; typed direct dependencies and reverse referencers; edge masks; deterministic BFS shortest path; bounded simple all-path traversal; Tarjan strongly connected components; explicit unresolved targets.
- Build node/edge overrun returns `Failed`; traversal depth/node/edge/path incompleteness returns `Truncated`. No truncated result is labeled complete.
- Hard-only path tests prove traversal does not silently cross a Manage edge. Hard, Soft, Manage, and Searchable Name remain distinct enum values and sort order.
- Fresh `scripts/Test.ps1`: exit `0`; seven Core/file tests plus process CLI passed; MQB discovered exactly seven production translation units.

## 2026-09-05 — PACT-20 why-cooked explanation

- RED: the graph contract added three calls to missing `ExplainWhyCooked`; MQB compilation exited `1` with C2039/C3861.
- GREEN: the multi-root explanation sorts/deduplicates roots, reuses the bounded shortest-path engine, and chooses by path length, root name, then complete typed-step order. The targeted graph test exited `0`.
- Root input order reversal produced byte-equivalent root/step results. A one-edge Searchable Name route beat a two-edge mixed route. A zero-depth limit propagated `Truncated` and returned no fabricated complete explanation.
- Fresh `scripts/Test.ps1`: exit `0`; all Core/file/CLI tests passed with seven MQB production translation units.

## 2026-09-05 — Source-bound BuildPlugin and fresh-extraction smoke

- After the unrelated UE pipeline released the global UBT mutex, `scripts/Build-Plugin.ps1` ran from clean SHA `7f5d7c1ea9ec940f133291467dfe5543c0478eed` and exited `0`.
- RunUAT built UnrealEditor plus UnrealGame Win64 Development and Shipping. Game targets compiled only the seven pure `CookScopeCore` translation units; Editor/Commandlet/Tests stayed excluded from non-Editor targets.
- Authoritative local package: `Artifacts/Packages/CookScope-7f5d7c1ea9ec`, 58 files, 207,349,925 bytes. It remains ignored and will not be uploaded to a Release.
- Identity files: Commandlet DLL 84,992 bytes SHA-256 `A606CAEF93FBF6A7C20A4DDF3DE665F6D8D547B73D5A37B08368B8B2FC132C44`; Core DLL 454,144 bytes `92D54AB43BC8AA406E9FEC5FD48182DDE34CEB38EE358B2217F52915B5503DDF`; Editor DLL 96,256 bytes `9C588E1D059B166AC670A4052725AEE6147ACA610A6F82F99B459CFE69D15123`; Tests DLL 59,392 bytes `5A8B93CE69FADDCA8EF174EC7D516732209CF0B2C11E62F22BDA2A5BEC502321`; descriptor 835 bytes `C8D7DB92CB68EFC313BCF632F8D3293A46EEB0B98609E072D857230EB902E869`.
- Package summary originally serialized `totalBytes` as `207349925.0` because PowerShell returned a floating aggregate. The checked-in script now casts to `Int64`; a later final package will verify the corrected field type.
- Fresh smoke attempt 1 created and extracted the local ZIP and loaded Core/Editor/Tests, but `-ExecCmds=Quit` did not terminate after Editor initialization. After more than three minutes, PID 47628 was revalidated as this exact CookScope smoke process and stopped; no unrelated process was touched. The attempt is recorded as failed.
- Fresh smoke attempt 2 used a new archive and HostProject, then ran the packaged `CookScopeAudit` Commandlet instead of relying on console Quit. It exited `0`, loaded packaged Core/Commandlet/Editor, and produced canonical clean JSON.
- Successful local ZIP: `Artifacts/Archives/CookScope-e750bf43c31f4051818c620942ad06f1.zip`, 47,534,630 bytes, SHA-256 `C835CAB7E2909B363018CC6EE4D7DE5F258ABF931EEA74BF7B9FCDDF924F9A91`.
- Extracted descriptor SHA-256 exactly matched source package descriptor. Fresh report SHA-256: `A286DFB4CE419C6BEC6CB8C48450404F1478046A3E86AB7CB187DA97DFAC8A53`.
- Fresh extraction is proven for this source package; clean-source checkout replay remains outstanding, so the combined PACT-70-04 gate is not marked PASS.

## 2026-09-05 — PACT-20 real UE fixtures and Asset Registry/Manager adapter

- Fixture Builder RED: real Editor-Cmd `-run=CookScopeFixtureBuilder` exited `1`; UE recognized the commandlet name but could not find the class.
- Implemented a Sample-only Runtime `UCookScopeFixtureAsset`, SampleEditor-only builder commandlet, Primary Asset scan config, and seven tiny project-owned assets. The production plugin does not depend on Sample modules.
- Fixture Builder GREEN: process test exited `0` and found Target, Hard, Soft, Searchable, Primary, CycleA, and CycleB `.uasset` files. Immediate regeneration changed zero of seven SHA-256 values, proving byte-idempotence for the current UE/toolchain.
- Fixture sizes/hashes: CycleA 1,595 bytes `171F1DAD67CBBF233AB8B5D5847AC4EE46F7289E8999193AD3142D0E684ABCC4`; CycleB 1,595 `461E6F1D567EB07C13DF763F52BD1E00E7208862FB5F8295F59730DBCAB22208`; Primary 2,328 `E9A0C122E1710AB7DC7C039F6E52FECC9773011724219BB2381DDA316B25D21E`; Hard 1,585 `8D27C01BA993A2AFD082A36E223B9061BADB5823F80D2BF4C7FC9FFCBA8151E7`; Searchable 1,754 `87837E712B2026A4C5AB03C88850EBA54B00BC718132B73A257B33851F1BAFDC`; Soft 1,561 `85999D17759C85D40935D358AFBEB2BA8574FC6CB443C9CFC63513A0BC116642`; Target 1,377 `67DDDB63C069706E886D3EDEF5AAC9EE83A27DA18A9DBA35D046C687DF7221C9`.
- First UBT after adding fixtures exposed anonymous helper collisions that only occur when UBT Unity combines Core `.cpp` files; MQB's separate-TU build had passed. File-local helpers received responsibility-specific names rather than disabling Unity. A later UBT run exited `0`.
- Registry scan RED: SampleEditor Automation failed C1083 because `CookScopeAssetScanner.h` did not exist.
- First scanner UBT linked all new code except `DependencyMask::All`, revealing a cross-DLL export missing from the class. `COOKSCOPECORE_API` was added to the class; repeated UBT exited `0`.
- Scanner uses UE 5.8 `IAssetRegistry::GetAssetsByPath`, typed `FAssetDependency` category/property flags, `TryGetAssetPackageData`, and `UAssetManager` Primary/Bundle/Manage/Chunk APIs, then canonical-writes and strict-parses through Core before returning data.
- Initial real scan passed every assertion except Searchable Name. Diagnostics proved `UPROPERTY(AssetRegistrySearchable)` creates a tag, not an `EDependencyCategory::SearchableName` edge.
- UE source tracing showed `FGameplayTag::Serialize` calls `FArchive::MarkSearchableName`. The fixture changed to a configured real Gameplay Tag instead of fabricating an edge in the scanner. After adding explicit `GameplayTags` module dependencies and regenerating assets, the real edge appeared as `/Script/GameplayTags.GameplayTag::CookScope.Search.Target` with Searchable Name kind.
- Final `CookScope.PACT20.RealAssetRegistryScan` Automation exited `0`: seven on-disk assets, package disk sizes, cooked size unavailable, Hard/Soft/Manage/Searchable Name, Primary ID, Default Bundle, Chunk 1, real hard cycle, graph build, and Primary-to-Target why-cooked all passed.
- Unified `scripts/Test-Unreal.ps1` now runs UBT, deterministic fixture generation, two PACT tests, and five Commandlet processes. Fresh top-level result exited `0`.

## 2026-09-05 — PACT-10 strict JSON syntax layer

- RED: `mqb run Tests/Core/JsonContractTests.cpp --no-discover -I Plugins/CookScope/Source/CookScopeCore/Public --std 20 --release -o CookScopeJsonContractTests` exited `1` with C1083 because `cookscope/json.h` did not exist.
- GREEN: the test plus `Private/json.cpp` exited `0` and printed `PASS: strict JSON parser and canonical writer contract`.
- Proven syntax behavior: object keys canonicalize in UTF-8 byte order; duplicate keys fail at stable JSON paths; trailing content fails; raw UTF-8 is validated/preserved; escaped UTF-16 requires paired surrogates; controls and escapes serialize deterministically; recursion is bounded at 128 by default.
- `scripts/Test.ps1` now runs three Core tests plus the process CLI. Fresh result: exit `0`; all tests passed, MQB discovered exactly four production translation units, and no UE adapter/package source entered the MQB target.

## 2026-09-05 — PACT-10 strict Rule Config domain

- RED: `mqb run Tests/Core/RuleConfigContractTests.cpp --no-discover -I Plugins/CookScope/Source/CookScopeCore/Public --std 20 --profile release -o CookScopeRuleConfigContractTests` exited `1` with C1083 because `cookscope/rule_config.h` did not exist.
- GREEN: the test with `Private/rule_config.cpp` and `Private/json.cpp` exited `0` and printed `PASS: strict versioned Rule Config contract`.
- Proven domain behavior: schema `cookscope.rules/1`; required Rule fields; strict root/Rule/Scope/Exception field sets; non-empty descriptive fields/selectors; lowercase dotted IDs; duplicate ID rejection; stable Severity/Baseline/FailThreshold enums; unsigned 64-bit `budgetBytes`; Rule ID and Exception canonical ordering; terminal LF.
- Rule-specific parameter field schemas remain a PACT-30 responsibility; the PACT-10 reader preserves the strict JSON object and validates the common byte budget without claiming rule evaluation.
- Fresh `scripts/Test.ps1`: exit `0`; four Core test executables and process CLI passed; MQB discovered exactly five production translation units.

## 2026-09-05 — PACT-10 strict Asset Snapshot domain

- RED: `mqb run Tests/Core/SnapshotContractTests.cpp --no-discover -I Plugins/CookScope/Source/CookScopeCore/Public --std 20 --profile release -o CookScopeSnapshotContractTests` exited `1` with C1083 because `cookscope/snapshot.h` did not exist.
- GREEN: the test with `Private/snapshot.cpp` and `Private/json.cpp` exited `0` and printed `PASS: strict deterministic Asset Snapshot contract`.
- Proven snapshot behavior: schema `cookscope.snapshot/1`; engine/platform/Cook/source-SHA provenance; Object/Package/Class/Path; optional Primary Asset ID; disk and cooked measurement kinds; Chunk IDs; Bundles; UTF-8 Tags; Hard/Soft/Manage/Searchable Name typed edges; per-asset source provenance.
- Available sizes require unsigned 64-bit bytes; unavailable sizes reject bytes. The model never maps estimated or package size to actual-cooked.
- Parser canonicalizes asset, dependency, Chunk, and Bundle ordering; rejects duplicate object paths, invalid SHA/numeric values, unknown fields, and contradictory size records.
- Fresh `scripts/Test.ps1`: exit `0`; five Core test executables and process CLI passed; MQB discovered exactly six production translation units.

## 2026-09-05 — PACT-10 public Schema and example artifacts

- File-artifact RED: `CookScopeSchemaArtifactsContractTests` built successfully and then failed because both public Schema files and both example files were absent.
- GREEN: after adding Draft 2020-12 Rule/Snapshot Schemas plus explicitly hand-authored fixture examples, the same executable printed `PASS: public Schema and canonical example artifacts`.
- The test validates both Schema files as strict JSON with stable `$id`, then sends both examples through domain parse -> canonical write -> domain parse.
- `Examples/snapshots/cookscope-snapshot.json` explicitly labels its asset source as `hand-authored-contract-fixture-not-an-ue-scan`; it is not presented as Asset Registry or Cook evidence.
- Independent Python Draft 2020-12 validation was attempted but could not run because global Python 3.13 lacks the `jsonschema` module. No dependency was installed into the user's global environment and no external-validator success is claimed.
- Fresh `scripts/Test.ps1`: exit `0`; all six Core/file-artifact test executables plus the CLI process contract passed; MQB production discovery remained six translation units.

## 2026-09-05 — Local baseline and PACT-00 core contract

- Local `main` baseline commit: `2bede4a50f47ee332e11f2dfbb90beb596c9dc0e`.
- Feature branch: `feat/cookscope-0.1`, created directly from that baseline; no implementation was written on `main`.
- RED command: `mqb run Tests/Core/BootstrapContractTests.cpp --no-discover -I Source/Core/Public --std 20 --release -o CookScopeBootstrapContractTests`.
- RED result: exit `1`; MSVC `C1083` because `cookscope/bootstrap.h` did not exist. This was the intended missing-contract failure.
- GREEN/refactor command: `mqb run Tests/Core/BootstrapContractTests.cpp Plugins/CookScope/Source/CookScopeCore/Private/bootstrap.cpp -I Plugins/CookScope/Source/CookScopeCore/Public --std 20 --release -o CookScopeBootstrapContractTests --timings=json`.
- Clean-after-layout-change result: exit `0`; 2 compile misses, 1 link miss, test printed `PASS: bootstrap status, exit code, and canonical JSON contract`.
- Immediate identical no-op result: exit `0`; 2 compile hits, 1 link hit, 0 misses, test passed; total reported MQB time 23.006 ms.
- Local ignored artifact: `.mqb/bin/CookScopeBootstrapContractTests.exe`, 19,968 bytes, SHA-256 `D7C64C8C092AA9855B65177155D079843797FC4D220B49D2903693C1F77C7130`.
- Proven behavior is intentionally narrow: five status-to-exit mappings and byte-stable bootstrap JSON. Full result Schema, rule, graph, UE commandlet, and Editor behavior remain unimplemented.

## 2026-09-05 — Strict helper CLI and checked-in MQB probe

- Argument RED: `mqb run Tests/Core/ArgumentsContractTests.cpp --no-discover -I Plugins/CookScope/Source/CookScopeCore/Public --std 20 --release -o CookScopeArgumentsContractTests` exited `1` because `cookscope/arguments.h` did not exist.
- Argument GREEN: the same test with `Private/arguments.cpp` added exited `0` and printed `PASS: strict bootstrap argument contract`.
- CLI process RED: `pwsh -NoLogo -NoProfile -File Tests/CLI/BootstrapCliContract.ps1 -Executable .mqb/bin/CookScopeCli.exe` exited `1` because the executable did not exist.
- First CLI build with `/W4 /WX` failed because C++20 deprecates `std::filesystem::u8path`; the implementation was changed to the native `std::u8string` path constructor instead of suppressing the warning.
- The checked-in `scripts/Test.ps1` initially failed after both tests passed because `mqb.json` listed nonexistent excluded directories and selected `/MT` over the Release `/MD` preset. Strict config was corrected to list only existing directories and use `/MD`; the identical test entry then exited `0` with no warnings.
- Clean MQB probe source SHA: `bb83af3b0a4b7b3f675def019925ed17d86a6ad5`.
- Clean result: exit `0`, 3 compile misses, 1 link miss, total 3448.496 ms.
- Immediate identical no-op result: exit `0`, 3 compile hits, 1 link hit, 0 misses, total 7.648 ms.
- Intentional `#error COOKSCOPE_EXPECTED_MQB_FAILURE` result: exit `4`; failure was not mistaken for success.
- Bound artifact: `.mqb/bin/CookScopeCli.exe`, 47,616 bytes, SHA-256 `0A0254E35FB10357627E22DEBA2083AECD19F8A54033E7341280E2787F00C117`.
- After the destructive-cache probe, `scripts/Test.ps1` rebuilt the test targets and exited `0`; both core tests and the process-level CLI test passed.

## 2026-09-05 — PACT-00 UE plugin, tab, and commandlet implementation

- UE scaffold RED command: UE 5.8 `Build.bat CookScopeSampleEditor Win64 Development -Project=... -WaitMutex -NoHotReloadFromIDE`.
- Scaffold RED result: UBT reached `PACT00SmokeTest.cpp` and exited `1` with MSVC `C1083` because `CookScopeAuditCommandlet.h` did not exist. This proved the test module was compiled by the real Editor target.
- First implementation build exposed two module-boundary defects: `FSpawnTabArgs` was forward-declared as `struct` instead of UE 5.8's `class`, and pure Core functions lacked `COOKSCOPECORE_API`, causing three cross-DLL unresolved externals. Both were corrected without duplicating Core code.
- Repeated UBT command then exited `0`; UHT, Sample, Core, Editor, Commandlet, and Tests modules compiled and linked with UE 5.8, MSVC 14.44, and SDK 10.0.26100.0.
- Tab invocation RED: Automation source called missing `FCookScopeEditorModule::InvokeTab`; UBT exited `1` with C2039/C3861.
- Tab invocation GREEN: the module delegates to its registered GlobalTabmanager spawner; UBT exited `0`, and real Editor-Cmd Automation found one CookScope test, invoked/closed an `ETabRole::NomadTab`, reported `Result={Success}`, and exited `0`.
- Real Editor-Cmd startup mounted the external CookScope plugin, loaded CookScope Core/Commandlet/Editor/Tests, and completed an actual Asset Registry initial scan. Engine startup printed intentional `UnifiedErrorTest` error messages before CookScope loaded; raw logs remain ignored and this is not represented as a zero-error-log result.
- Commandlet process RED 1: exit `1`; plugin mounted but the class was undiscoverable because the Commandlet module used `PostEngineInit`. UE 5.8 reference commandlet plugins use `Default` or earlier; the module was changed to `Default`.
- Commandlet process RED 2: class executed but returned `3` because UE includes `-run` and host flags in `Main`. The UE adapter now filters only the explicit engine-owned allowlist and sends all other switches to the strict Core parser, preserving unknown-argument failure.
- Commandlet GREEN: five real Editor-Cmd processes produced clean `0`, violation `2`, invalid invocation `3`, internal IO error `4`, and cancelled `5`; canonical clean JSON matched the pure Core bytes.
- `Test-Unreal.ps1` initially misread the cancelled subprocess's residual `$LASTEXITCODE=5` as child-script failure even though all assertions passed. It now relies on PowerShell exceptions for script failure; a fresh external run exited `0` and printed the full PASS matrix.
- First `RunUAT.bat BuildPlugin` exited `0` after real UHT/Editor build plus UnrealGame Win64 Development and Shipping builds. Game targets compiled only `CookScopeCore`, excluding Editor/Commandlet/Tests as intended.
- Initial ignored package: `Artifacts/Packages/CookScope-PACT00`, 42 files, 194,994,074 bytes. It is exploratory and not yet bound to the forthcoming clean source commit; the committed BuildPlugin script will generate the authoritative source-bound package.
- UBT repeatedly reported that UBA could not bind local port 1345 and then used its local executor successfully. No unrelated process was terminated to suppress this environmental warning.
- MQB discovery regression after packaging: the initially valid exclusion list omitted then-nonexistent `Artifacts`/`SampleProject`, so discovery later found copied package and UE adapter sources. Once those directories existed, the strict list was updated to exclude them and the three UE-only module directories. MQB then reported exactly three translation units and the full Core/CLI suite passed.

## 2026-09-05 — PACT-00 clean source-SHA replay

- Source code SHA under test: `4dbbbaebadca79944b38ee52f4e3981d90e1dd01`; the worktree was clean when each replay began.
- `scripts/Probe-Mqb.ps1`: exit `0`; clean 3 compile misses/1 link miss in 3341.499 ms; immediate no-op 3 compile hits/1 link hit/0 misses in 7.947 ms; intentional compile failure returned `4`.
- Bound MQB artifact: `.mqb/bin/CookScopeCli.exe`, 47,616 bytes, SHA-256 `DA8A15BCB3F232008929DA6F115254AA4D1F5907EB208F028E52B02D4E72961F`.
- `scripts/Test.ps1`: exit `0`; bootstrap Core, strict arguments, and process-level CLI report/exit tests passed with exactly three MQB-discovered translation units.
- `scripts/Test-Unreal.ps1`: exit `0`; UBT Editor Development succeeded, real Editor-Cmd Automation found and passed exactly one PACT-00 test that invoked/closed the Nomad Tab, and five real Commandlet processes returned `0/2/3/4/5` with canonical JSON checks.
- Source-bound BuildPlugin attempt 1: HostProject UHT/UnrealEditor Development succeeded, then UnrealGame Development failed `ConflictingInstance`/exit `10` because an unrelated `AuthorityArenaEditor` UBT held the global mutex. AutomationTool labels exit 10 `Error_SDKNotFound`, but the direct cause in the log is the mutex, not a missing SDK.
- The unrelated process ended naturally; no process was terminated. Source-bound BuildPlugin attempt 2 used a new package path and again completed its Editor stage before the concurrently running `AuthorityArena.GAS` Editor/parent pipeline took the global UBT mutex at the Game stage.
- Incomplete package directories are retained under ignored `Artifacts/Packages/` as failure evidence and will not be overwritten or uploaded. The authoritative source-bound package gate remains blocked until the unrelated UE pipeline releases the global mutex.

## Next actions

1. Commit the deterministic graph slice after diff checks.
2. Add a why-cooked multi-root explanation RED test with stable root selection and limit metadata.
3. Create deterministic UE fixture assets, then add real Asset Registry/Asset Manager scan tests for all supported edge kinds.
4. Recheck the external UBT mutex owner before UE build or source-bound BuildPlugin retry.
