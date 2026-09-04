# CookScope Agent Contract

This repository is a source-only Unreal Engine 5.8 portfolio project. These instructions apply to the entire repository.

## Non-negotiable rules

- Preserve `tasks/20260905-000547-cookscope-0.1/goal-objective.md` verbatim.
- Keep exactly one writing agent. A final independent audit agent is read-only.
- Use RED -> GREEN -> REFACTOR for production behavior and record the failing and passing commands in `progress.md`.
- Try MQB first for the pure C++ core and bounded capability probes. Use UBT or RunUAT only at the documented Unreal-specific boundary.
- Never claim Editor load, Asset Registry scan, Cook, packaging, clean extraction, PR, tag, or Release success without fresh evidence.
- Never commit generated Unreal directories, credentials, binaries, cooked output, packages, or large logs.
- Store local packages and evidence under ignored `Artifacts/`.
- The analyzer is read-only. It may emit suggestions but must not modify user assets or project configuration.
- Do not modify the Unreal Engine installation.
- Do not end unrelated processes.
- Keep user-facing session messages in Simplified Chinese.

## Required workflow

1. Read `.agents/AGENTS.md`, the active task contract, and the acceptance matrix before editing.
2. Frame a small PACT slice and add or select its failing test.
3. Observe the intended RED failure.
4. Apply the smallest production change.
5. Observe GREEN, then run the relevant guardrail suite.
6. Save concise evidence and update status without inflating claims.
7. Commit only the focused slice.

## Release boundary

GitHub Release assets must remain empty. Only GitHub-generated source archives are allowed. Local plugin/sample/report packages belong in ignored `Artifacts/` and are referenced by SHA-256, size, source SHA, command, and outcome.

