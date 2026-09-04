# Handoff

## Current state

The dedicated local repository has a verified contract baseline at `2bede4a50f47ee332e11f2dfbb90beb596c9dc0e`. Work is on `feat/cookscope-0.1`. MQB Core/CLI, UE 5.8 Editor/Automation/Commandlet, PACT-10 models, PACT-20 Registry/graph, PACT-30 rules/Data Validation, BuildPlugin/fresh extraction, and a real clean UE 5.8 Zen Cook are GREEN. Diff, reports, full Editor workflow, clean-source replay, audit, and remote release remain outstanding.

## Immutable constraints

- Preserve `goal-objective.md` verbatim.
- One repository writer; final independent auditor read-only.
- MQB first where supported; official UE tools only at measured boundaries.
- Real Editor, Asset Registry, Cook, reports, package, and clean-source evidence is mandatory.
- Keep Alpha/WIP until every P0 passes.
- GitHub custom Release assets must be empty.
- The user declined GitHub login; do not launch or request authentication again unless the user explicitly reverses that instruction.

## Resume point

Commit the Cook slice, then implement deterministic PACT-40 snapshot diff and actual Cook-size capture. Do not rerun long BuildPlugin/Cook on every Core commit; final verification requires both at the final source SHA.
