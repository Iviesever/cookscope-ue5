# Repository Workflows

## Scope and ownership

- The root Codex task is the sole writer for this repository.
- Any independent audit agent must use read-only commands and must not edit, stage, commit, push, comment, or change remote state.
- Existing user changes are never reset, overwritten, or deleted.

## PACT checkpoint

For each checkpoint, append to `tasks/20260905-000547-cookscope-0.1/progress.md`:

1. Contract slice and affected requirement IDs.
2. Exact RED command and expected failure reason.
3. Exact GREEN/guardrail commands and exit codes.
4. Artifact/log paths, sizes, SHA-256 values, and bound Git SHA when applicable.
5. Remaining limitations and the next smallest slice.

## Build policy

- Resolve MQB with `Get-Command mqb`; use its public CLI and strict `mqb.json`.
- Run identical MQB build commands twice to measure clean and no-op behavior.
- Do not invent `mqb test`; build and run the real test executable.
- UE-specific build, UHT, Editor, commandlet, Cook, BuildPlugin, and Automation work may use the installed UE 5.8 UBT/RunUAT tools only after the capability probe documents the boundary.
- Never replace a successful MQB/MSVC path with Clang, GCC, ordinary CMake, or direct `cl.exe`.

## Evidence policy

- `docs/ACCEPTANCE_MATRIX.md` starts fail-closed: `NOT RUN` is not `PASS`.
- Evidence must be reproducible from a clean source tree.
- Normalize unrelated local absolute paths from committed reports and logs.
- Screenshots must come from the real Editor/report UI produced by the source SHA named in evidence.

## Git policy

- `main` is the local baseline.
- Feature work belongs on `feat/cookscope-0.1` created from the verified baseline commit.
- Use focused conventional commits; never rewrite unknown history.
- Remote operations require explicit usable authentication. Local development and commits do not.

