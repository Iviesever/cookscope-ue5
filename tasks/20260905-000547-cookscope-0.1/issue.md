# CookScope 0.1 Requirement Brief

## Human intent

Create a public portfolio-quality Unreal Engine 5.8 project that demonstrates real asset dependency and Cook auditing rather than a UI mock, rule list, or documentation-only shell. The system must load in the real Editor, scan the real Asset Registry, perform a real Sample Project Cook, produce real deterministic reports, survive clean-source validation, undergo independent audit, and publish as a source-only `v0.1.0` Release.

The full unabridged human-authored requirements are preserved in `goal-objective.md`.

## PACT brief

```text
Objective: Deliver CookScope v0.1.0 as a verified UE 5.8 plugin, commandlet, sample, reports, and source-only release.
Gap: The target repository did not exist when work began; every functional P0 gate is initially unimplemented and unverified.
Scope: PACT-00 through PACT-70 and required portfolio/release documentation; no P1 until all P0 passes.
Done when: Every P0 matrix row passes with reproducible evidence, audit has no Blocker/High, and the zero-custom-asset release is published.
```

## Resolved decisions

- The human objective is sufficiently explicit to serve as design approval and directs autonomous continuation rather than another planning pause.
- Use the layered shared-core approach documented in `docs/ARCHITECTURE.md`.
- Keep a dedicated repository checkout at `D:\program\cookscope-ue5`; no nested worktree is needed because the new repository itself is isolated and has one writer.
- Local Git work continues without GitHub authentication. Remote-only gates remain accurately blocked until the user explicitly permits usable authentication.
- No visual companion is needed for requirements clarification because all current design choices are textual architecture and acceptance decisions.

