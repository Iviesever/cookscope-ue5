# Asset Registry Model

`FCookScopeAssetScanner` is the UE boundary. It scans an explicit package path, sorts assets by Object Path, and converts `FAssetData` into immutable Core records.

## Identity and measurements

- Object Path is the stable report identity; Package Name and Package Path remain separate fields.
- Asset Class uses the top-level class path (`/Script/Engine.Texture2D`, for example).
- `TryGetAssetPackageData` supplies `package-disk` bytes. It is never relabeled as Cook size.
- Cook size starts as `unavailable` and is replaced with `actual-cooked` only by `FCookScopeCookSnapshotReader` using a real Development Asset Registry.
- Asset Registry tags and their provenance remain in the snapshot.

## Typed edges

UE dependency category/property flags are preserved as Hard, Soft, Manage, or Searchable Name. Manage and Bundle data are also collected from `UAssetManager`. Identical Bundle IDs, Chunk IDs, and typed edges are de-duplicated before the record crosses into Core.

The sample proves each edge kind with real saved assets. Searchable Name is produced by a serialized `FGameplayTag`; it is not fabricated from an arbitrary searchable property.

## Resource metadata

The adapter normalizes authoritative UE facts to stable Core tag names:

| UE source | Core tag |
|---|---|
| Texture imported size/source mips/format | `TextureWidth`, `TextureHeight`, `TextureMips`, `TextureFormat` |
| Static Mesh `Triangles` tag | `MeshTriangles` |
| Skeletal Mesh `Vertices` tag | `MeshVertices` |
| SoundWave duration/runtime compression | `SoundDurationMs`, `SoundFormat` |

The four real resource fixtures are duplicated from small engine-owned assets into `/Game/CookScopeResourceFixtures`; a UE Automation test proves eight measured rule outcomes with no unavailable-data diagnostic.
