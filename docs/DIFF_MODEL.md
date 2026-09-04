# Diff Model

`DiffSnapshots` first checks schema, engine version, platform, and Cook configuration. Source SHA is expected to differ. Incompatible snapshots fail closed and produce no meaningful delta.

The deterministic `cookscope.diff/1` projection includes:

- Added, removed, modified, and explicit StableAssetId-based renamed assets.
- Added, removed, and type-changed dependency edges.
- Signed actual-cooked byte changes only when both sides contain real Cook measurements.
- Added, resolved, and severity-changed findings.

Renames are never inferred from similar filenames. Arrays are sorted before serialization. The sample pair intentionally contains one added `DA_Candidate` asset at 892 bytes.

Core identity comparison is bytewise and case-sensitive; it does not perform Unicode normalization or Windows path folding. UE adapters emit canonical Object Paths, and explicit `StableAssetId` is required to bridge a real rename. Case-only inputs without that ID remain removed/added rather than being guessed as equal.
