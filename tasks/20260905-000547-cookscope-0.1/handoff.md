# Handoff

## Current state

The dedicated local repository has a verified contract baseline at `2bede4a50f47ee332e11f2dfbb90beb596c9dc0e`. Work is on `feat/cookscope-0.1`. MQB Core/CLI, UE 5.8 Editor build, real Editor-Cmd Automation/Tab, five-way Commandlet E2E, PACT-10 strict models, PACT-20 typed Registry/graph, PACT-30 core rule families, and shared UE Data Validation are GREEN. A source-bound package at `7f5d7c1ea9ec940f133291467dfe5543c0478eed` passed BuildPlugin and fresh extracted Commandlet load. Clean-source replay remains outstanding.

## Immutable constraints

- Preserve `goal-objective.md` verbatim.
- One repository writer; final independent auditor read-only.
- MQB first where supported; official UE tools only at measured boundaries.
- Real Editor, Asset Registry, Cook, reports, package, and clean-source evidence is mandatory.
- Keep Alpha/WIP until every P0 passes.
- GitHub custom Release assets must be empty.
- The user declined GitHub login; do not launch or request authentication again unless the user explicitly reverses that instruction.

## Resume point

Commit the Data Validation slice. Next add real UE resource metadata fixtures/extraction and then PACT-40 real Cook/diff. Do not rerun long BuildPlugin on every Core commit; final verification requires a new package at the final source SHA.
