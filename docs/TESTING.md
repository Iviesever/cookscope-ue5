# Testing

## Fast Core loop

`pwsh -File scripts/Test.ps1` runs 16 isolated C++ contract executables and a process-level CLI contract through MQB. Coverage includes strict JSON/config/snapshot schemas, graph queries and limits, every P0 rule family, baseline semantics, deterministic diff, and all report projections.

`pwsh -File scripts/Probe-Mqb.ps1` performs a clean build, immediate no-op build, intentional compile failure, and output identity check.

## UE integration

`pwsh -File scripts/Test-Unreal.ps1` builds with UBT, regenerates 20 deterministic fixtures, runs 7 Editor Automation tests, then executes the bootstrap Commandlet matrix, cooperative/hard audit timeout checks, and real Cook diff checks.

The Automation set covers production tab loading, typed Registry/Asset Manager edges, Data Validation reuse, real resource metadata, Cook registry merging, Editor filtering/comparison/why-cooked/export, cancellation, and shutdown.

## Heavy gates

- `scripts/Cook.ps1`: clean real Win64 Development Cook.
- `scripts/Build-Plugin.ps1`: source-bound local BuildPlugin package.
- `scripts/Smoke-PluginPackage.ps1`: ZIP, fresh extraction, new host project, packaged Commandlet smoke.
- `scripts/Smoke-CleanSource.ps1`: `git archive` extraction, MQB Core tests, fresh UBT build, fixture regeneration, and production-tab smoke without Git metadata.
- `scripts/Capture-EditorScreenshot.ps1` and `scripts/Capture-HtmlReport.ps1`: real UI artifacts.

Generated logs, packages, screenshots used only as evidence, and Cook output live under ignored paths. Checked-in examples are scrubbed of host paths.
