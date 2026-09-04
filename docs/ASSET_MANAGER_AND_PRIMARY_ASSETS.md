# Asset Manager and Primary Assets

The sample registers `UCookScopeFixtureAsset` as Primary Asset type `CookScopeFixture`. `DA_Primary` and `DA_Candidate` expose stable Primary Asset IDs, the `Default` Bundle, and Chunk 1 through `DefaultGame.ini`.

CookScope asks `UAssetManager` for Primary ID, rules, Bundle entries, managed packages, and management dependencies, then merges those facts with the Asset Registry graph. Duplicate facts are normalized as sets.

Primary Assets are ownership/management roots. Secondary assets are the packages they reference or manage. A Soft reference does not load immediately, while a Manage edge records Asset Manager ownership; either can participate in a why-cooked path depending on the selected edge mask.

P0 Asset Manager rules cover required Primary IDs/types, required Bundles, required/conflicting Chunks, AlwaysCook/NeverCook conflicts, and expected/unexpected Cook membership. Missing data is a diagnostic, not a guessed value.
