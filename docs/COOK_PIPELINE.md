# Cook Pipeline

`scripts/Cook.ps1` runs UE 5.8 AutomationTool `BuildCookRun` with a clean Win64 Development build and Cook. The sample uses UE 5.8 Zen Store, so validation relies on `AssetRegistry.bin`, `Metadata/DevelopmentAssetRegistry.bin`, and `Metadata/zenfs.manifest` rather than assuming loose `.uasset` output.

`FCookScopeCookSnapshotReader` loads the Development Asset Registry with `FAssetRegistryState::LoadFromDisk`. Runtime package membership determines Cook inclusion; disk size from that registry becomes `actual-cooked`. Assets absent from the Cook registry stay `unavailable`.

The controlled baseline excludes `DA_Candidate`; the candidate includes it. Stable provenance fields (engine version, platform, configuration, source SHA) gate comparison. The checked-in examples show one added 892-byte candidate and preserve unavailable values for source-only fixtures.

Cook output is evidence only and remains under ignored `SampleProject/Saved` and `Artifacts`. Pak/IoStore staging is outside this v0.1 repository; the product audits Cook metadata before packaging.
