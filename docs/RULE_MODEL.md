# Rule Model

## Envelope

Rule configuration schema `cookscope.rules/1` is strict JSON. Unknown fields and duplicate object keys are errors. Canonical output uses compact UTF-8 JSON, terminal LF, lexicographically ordered object keys, stable Rule ID ordering, and no wall-clock data.

```json
{
  "schema": "cookscope.rules/1",
  "rules": [
    {
      "id": "naming.asset-prefix",
      "name": "Asset type prefix",
      "description": "Requires configured prefixes for matched asset classes.",
      "severity": "error",
      "scope": {
        "include": ["/Game/**"],
        "exclude": ["/Game/Developers/**"]
      },
      "parameters": {
        "classPrefixes": {"/Script/Engine.Texture2D": "T_"}
      },
      "exceptions": [],
      "baseline": "report-new-or-worsened",
      "failThreshold": "error",
      "helpUri": "docs/rules/naming-asset-prefix.md"
    }
  ]
}
```

## Required fields and invariants

- `id`: non-empty stable lowercase dotted identifier; unique after Unicode-normalized exact comparison.
- `name`, `description`, `helpUri`: non-empty strings.
- `severity` and `failThreshold`: `note`, `warning`, or `error`; threshold ordering is explicit.
- `scope.include`: non-empty ordered glob list; `scope.exclude` is ordered and wins over include.
- `parameters`: preserved strict JSON object. Rule-specific unknown or incorrectly typed values produce explicit evaluation diagnostics and make the Commandlet fail closed.
- `exceptions`: stable entries requiring asset/path selector plus non-empty reason; duplicate selectors fail.
- `baseline`: `report-all`, `report-new-or-worsened`, or `suppress-existing`.

Numeric sizes are unsigned 64-bit byte counts. Ratios use decimal strings to avoid platform floating-point serialization drift. Negative values, overflow, NaN-like strings, and contradictory minimum/maximum values are errors.

## P0 rule families

- Naming/path: prefixes, forbidden/temp directories, Editor-only leakage, duplicate/ambiguous names.
- Dependency boundaries: forbidden/cross-layer/cross-module edges, cycles, Runtime-to-Editor, hardening of soft references, maximum depth/fan-out.
- Resource budgets: texture, static/skeletal mesh, sound, per-asset package/cooked size, per-directory, per-type, and project totals.
- Asset Manager/Cook: missing/conflicting Primary Asset config, invalid bundles, chunk conflicts/duplication, unexpected/missing Cook, NeverCook/AlwaysCook conflicts, redirectors, and missing references.

Every size finding names its measurement kind: `source-disk`, `package-disk`, `estimated`, `actual-cooked`, or `unavailable`. Estimated data is never labeled actual.
