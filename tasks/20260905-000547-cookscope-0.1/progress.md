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
Objective: Establish the local main contract baseline, then begin PACT-00 on feat/cookscope-0.1.
Gap: No source baseline commit, plugin, sample, test, build, Editor load, or commandlet exists yet.
Scope: Planning/evidence files first; then the smallest MQB and UE smoke RED tests.
Done when: PACT-00 has a reproducible MQB core/helper build, real UE plugin load, Editor tab, commandlet JSON, stable exit-code evidence, and a bounded MQB/UE capability matrix.
```

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

## Next actions

1. Commit the PACT-00 UE source and scripts after final diff/generated-file checks.
2. On the clean commit, rerun MQB probe, Core/CLI suite, UE suite, and source-bound BuildPlugin.
3. Verify local package identity and a fresh-host loading smoke before updating acceptance statuses.
4. Begin PACT-10 with strict model/JSON RED tests.
