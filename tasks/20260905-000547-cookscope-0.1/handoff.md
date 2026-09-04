# Handoff

## Current state

The dedicated local repository has a verified contract baseline at `2bede4a50f47ee332e11f2dfbb90beb596c9dc0e`. Work is on `feat/cookscope-0.1`. MQB Core/CLI, UE 5.8 Editor build, real Editor-Cmd Automation/Tab, five-way Commandlet E2E, and an exploratory BuildPlugin run are GREEN. Acceptance remains fail-closed until these results are rerun on the clean source commit.

## Immutable constraints

- Preserve `goal-objective.md` verbatim.
- One repository writer; final independent auditor read-only.
- MQB first where supported; official UE tools only at measured boundaries.
- Real Editor, Asset Registry, Cook, reports, package, and clean-source evidence is mandatory.
- Keep Alpha/WIP until every P0 passes.
- GitHub custom Release assets must be empty.
- The user declined GitHub login; do not launch or request authentication again unless the user explicitly reverses that instruction.

## Resume point

Commit the current UE source/scripts, rerun every PACT-00 check on the clean SHA, generate the authoritative source-bound plugin package, then start PACT-10 strict model/JSON tests.
