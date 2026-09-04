# Diff Model

`DiffSnapshots` first checks schema, engine version, platform, and Cook configuration. Source SHA is expected to differ. Incompatible snapshots fail closed and produce no meaningful delta.

The deterministic `cookscope.diff/1` projection includes:

- Added, removed, modified, and explicit StableAssetId-based renamed assets.
- Added, removed, and type-changed dependency edges.
- Signed actual-cooked byte changes only when both sides contain real Cook measurements.
- Added, resolved, and severity-changed findings.

Renames are never inferred from similar filenames. Arrays are sorted before serialization. The sample pair intentionally contains one added `DA_Candidate` asset at 892 bytes.
