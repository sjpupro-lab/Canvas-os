# Canvas OS

**Deterministic Canvas-as-Memory Operating System**

Canvas OS는 2D 캔버스를 메모리 모델로 사용하는 결정론적 운영체제입니다.
모든 연산은 정수 기반(DK-2)이며, 셀 인덱스 오름차순(DK-3)으로 실행됩니다.

## Architecture

```
L0  Engine Core     Pure C deterministic engine (10K+ LOC, 160+ tests)
L1  Spatial Compress CVP1 format spatial compression research
L2  Bridge          TurboModule (JSI HostObject) for React Native
L3  UI Shell        React Native Expo — 9 panels + Zustand state
```

## Build

```bash
make -C engine/          # Build engine
make -C engine/ test     # Run 160+ tests
```

## Key Features

- **Rule Cell Engine** — 256-entry RuleTable + BpageChain
- **Determinism Kernel** — DK-1 tick boundary, DK-2 integer math, DK-3 ascending order, DK-4 clamp, DK-5 FNV-1a hash
- **WhiteHole/BlackHole** — 65536-record circular log + pattern compression
- **Multiverse Branching** — O(1) branch switch, 256 max branches
- **PixelCode + 한글코드** — Self-hosting visual programming
- **Tervas** — ANSI canvas terminal renderer

## Development (G-stack)

이 프로젝트는 [G-stack](https://github.com/garrytan/gstack) 워크플로우를 사용합니다:

```
/plan-eng-review → Code → /review → /ship
```

## Version

v1.0.8-patchH (2026-03-22)

## License

MIT
