# Handoff

## Current state

The public repository is `Iviesever/cookscope-ue5`, and local `main` tracks `origin/main`. PR #1 was created as Draft, marked Ready, and normally merged as `2c3ae0a`; both feature branches were deleted. The merge-result Core/CLI and full UE suites pass. The final independent read-only audit reports 0 Blocker and 0 High. Annotated tag `v0.1.0` and the public source-only Release are published, with the GitHub API reporting an empty custom `assets` array.

## Immutable constraints

- Preserve `goal-objective.md` verbatim.
- One repository writer; final independent auditor read-only.
- MQB first where supported; official UE tools only at measured boundaries.
- Real Editor, Asset Registry, Cook, reports, package, and clean-source evidence is mandatory.
- Keep Alpha/WIP until every P0 passes.
- GitHub custom Release assets must be empty.
- GitHub authorization was explicitly restored by the user and `gh` verified the active identity as `Iviesever`.

## Resume point

No implementation or release action remains. Generated local build/test evidence may be cleaned according to the final cleanup inventory; future builds regenerate it.
