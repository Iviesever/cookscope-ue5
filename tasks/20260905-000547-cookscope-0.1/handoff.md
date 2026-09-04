# Handoff

## Current state

The dedicated local repository is being initialized. All functional acceptance gates are fail-closed and no implementation claim exists yet.

## Immutable constraints

- Preserve `goal-objective.md` verbatim.
- One repository writer; final independent auditor read-only.
- MQB first where supported; official UE tools only at measured boundaries.
- Real Editor, Asset Registry, Cook, reports, package, and clean-source evidence is mandatory.
- Keep Alpha/WIP until every P0 passes.
- GitHub custom Release assets must be empty.
- The user declined GitHub login; do not launch or request authentication again unless the user explicitly reverses that instruction.

## Resume point

Verify the initial contract files and goal hash, commit local `main`, create `feat/cookscope-0.1`, then begin the PACT-00 RED tests described in `task.md`.

