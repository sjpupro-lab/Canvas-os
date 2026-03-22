# Canvas OS

## G-stack Workflow

Use `/browse` from gstack for web navigation. Never use MCP Chrome tools.

Available commands:
- `/plan-eng-review` — Architecture review before coding
- `/review` — Pre-landing PR safety check
- `/ship` — Automated PR creation + test + push
- `/investigate` — Systematic root-cause debugging
- `/office-hours` — Product strategy review
- `/design-review` — UI/UX audit
- `/qa` — Browser-based QA testing
- `/careful` — Extra-cautious mode
- `/freeze` / `/unfreeze` — Code freeze control
- `/retro` — Sprint retrospective

## Build & Test

```bash
make -C engine/          # Build C engine
make -C engine/ test     # Run 348 determinism tests (0 failures expected)
```

## Architecture

```
L0: engine/     — Pure C deterministic engine (DK-1~5)
L1: spatial/    — CVP1 spatial compression
L2: bridge/     — TurboModule JSI HostObject (Spec 1)
L3: shell/      — React Native Expo UI (Spec 2)
```

## Conventions

- **DK-2**: Integer-only arithmetic — NO float/double anywhere in engine
- **DK-3**: cell_index ascending order — deterministic scan guarantee
- **DK-5**: FNV-1a hashing — reproducible state fingerprint
- **PROC_MAX=64**: Static process limit (engine/include/canvasos_sched.h)
- **GPU**: Stub only (engine/src/canvas_gpu_stub.c) — Phase 7+ target

## Specs

- `specs/SPEC_1_ENGINE_BRIDGE.md` — Native C ↔ React Native bridge contract
- `specs/SPEC_2_UI_SHELL.md` — UI panel system + state management

## Critical Gaps (from AUDIT.md)

- `canvas://` URI scheme — NOT IMPLEMENTED
- GPU acceleration — STUB ONLY
- PROC_MAX=64 bottleneck — needs dynamic expansion
