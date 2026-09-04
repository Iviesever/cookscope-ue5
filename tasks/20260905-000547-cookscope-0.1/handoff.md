# Handoff

## Current state

The dedicated local repository has a verified contract baseline at `2bede4a50f47ee332e11f2dfbb90beb596c9dc0e`. Work is on `feat/cookscope-0.1`. At source SHA `4dbbbaebadca79944b38ee52f4e3981d90e1dd01`, MQB Core/CLI, UE 5.8 Editor build, real Editor-Cmd Automation/Tab, and five-way Commandlet E2E are GREEN. Source-bound BuildPlugin is externally blocked by an unrelated AuthorityArena UE pipeline taking the global UBT mutex between RunUAT stages; do not terminate it.

## Immutable constraints

- Preserve `goal-objective.md` verbatim.
- One repository writer; final independent auditor read-only.
- MQB first where supported; official UE tools only at measured boundaries.
- Real Editor, Asset Registry, Cook, reports, package, and clean-source evidence is mandatory.
- Keep Alpha/WIP until every P0 passes.
- GitHub custom Release assets must be empty.
- The user declined GitHub login; do not launch or request authentication again unless the user explicitly reverses that instruction.

## Resume point

Continue PACT-10 strict model/JSON work under MQB while the unrelated UE pipeline runs. Retry source-bound BuildPlugin from a fresh package path only after the global UBT mutex is free.
