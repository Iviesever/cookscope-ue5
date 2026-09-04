# Code Walkthrough

Start at `Plugins/CookScope/Source/CookScopeCore/Public/cookscope`:

1. `json.h`, `rule_config.h`, and `snapshot.h` define strict input contracts.
2. `graph.h` builds typed direct/reverse indexes and bounded path/cycle queries.
3. `rules.h` evaluates shared P0 policies and keeps measurement/baseline evidence.
4. `diff.h` compares compatible snapshots and finding sets deterministically.
5. `reports.h` projects one result into JSON, SARIF, JUnit, and HTML.

Then cross the UE boundary in `CookScopeEditor`: `CookScopeAssetScanner.cpp` captures Registry/Manager facts; `CookScopeCookSnapshotReader.cpp` merges actual Cook size; `CookScopeEditorSession.cpp` owns asynchronous analysis; `SCookScopePanel.cpp` presents results; `CookScopeValidator.cpp` reuses the same rules.

`CookScopeAuditCommandlet.cpp` is the CI orchestration path. SampleEditor contains fixture construction and Automation tests. `Tests/Core` stays UE-free and is compiled directly by MQB.
