# Snapshot Schema

The versioned schema is `cookscope.snapshot/1`; its public JSON Schema is [`Schemas/cookscope-snapshot.schema.json`](../Schemas/cookscope-snapshot.schema.json).

Each record carries Object/Package/Class/Path identity, optional Primary Asset ID, disk and cooked measurements, Chunk IDs, Bundles, tags, typed dependencies, and source provenance. Snapshot provenance carries engine version, platform, Cook configuration, and a 40-hex source SHA.

Parsing is strict: unknown fields, malformed numbers, contradictory measurements, invalid SHA values, and duplicate Object Paths fail with a stable JSON path. Asset order, Bundle/Chunk order, typed-edge order, and canonical JSON keys are deterministic. Duplicate Bundle/edge facts normalize to one semantic value.

Measurement kinds are `source-disk`, `package-disk`, `estimated`, `actual-cooked`, and `unavailable`. Available kinds require unsigned bytes; unavailable rejects bytes.
