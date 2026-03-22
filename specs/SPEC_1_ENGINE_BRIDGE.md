# 명세서 1: Engine Bridge (C/C++ Native Layer)

> **담당**: Turbo Module, C++ JSI 래퍼, CMake 빌드, v2 C 엔진 통합
> **의존**: v2 C 소스 (수정 없음), React Native New Architecture (TurboModule)
> **산출물**: `modules/canvasos-engine/` 디렉토리 전체
> **통합 인터페이스**: `NativeCanvasOSEngine.ts` (Spec 2와의 계약)

---

## 0. SJ CANVAS 태생적 장점 — 엔진 측 보존 원칙

이 명세서의 모든 구현은 아래 원칙을 위반하면 안 된다:

| 장점 | 보존 방법 |
|------|----------|
| **Canvas-as-Memory** (그리드 = 메모리 = 실행환경 = 화면) | Cell 배열을 JS에 복사하지 않음. C 포인터를 Skia에 직접 전달 |
| **Determinism Kernel** (DK_INT, FNV-1a, 부동소수점 금지) | C++ 래퍼에서 float/double 연산 금지. 해시는 반드시 C의 `dk_canvas_hash()` 사용 |
| **WhiteHole/BlackHole** (시간 이벤트 로그 + 시간 압축) | WH 링버퍼를 JS로 복사하지 않음. `query()` 호출 시 C에서 JSON 직렬화하여 반환 |
| **Gate 시스템** (타일 단위 공간 접근 제어) | 게이트 상태는 C가 주인. JS는 `exec("gate open N")` 으로만 제어 |
| **이중 언어** (한글코드 + PixelCode) | 파싱은 반드시 C의 `pixelcode.c`가 수행. JS에서 재구현하지 않음 |
| **Branch/Multiverse** (O(1) 브랜치 전환) | PageSelector 메커니즘 보존. JS는 `exec("branch switch N")` 만 호출 |
| **결정론적 재생** (WH 기반 타임워프) | `exec("rewind N")` → C의 `engctx_replay()` 호출. JS 로직 개입 없음 |

---

## 1. 디렉토리 구조

```
modules/canvasos-engine/
├── android/
│   ├── CMakeLists.txt                 # [1.3] C 엔진 + C++ 래퍼 빌드
│   ├── build.gradle                   # [1.4] Android 모듈 설정
│   └── src/main/
│       ├── AndroidManifest.xml
│       └── java/com/canvasos/engine/
│           ├── CanvasOSEnginePackage.java   # [1.5] React Native 패키지 등록
│           └── CanvasOSEngineModule.java    # [1.5] TurboModule Java 바인딩
├── cpp/
│   ├── CanvasOSEngineTurbo.h          # [1.6] JSI HostObject 헤더
│   ├── CanvasOSEngineTurbo.cpp        # [1.7] JSI 구현 (핵심)
│   ├── QueryDispatcher.h              # [1.8] JSON-RPC 쿼리 라우터 헤더
│   ├── QueryDispatcher.cpp            # [1.8] JSON-RPC 쿼리 라우터 구현
│   └── OnLoad.cpp                     # [1.9] JNI_OnLoad + 모듈 등록
├── src/
│   ├── index.ts                       # [1.10] JS export
│   └── NativeCanvasOSEngine.ts        # [1.11] TurboModule Codegen 스펙 ★계약★
└── canvasos-engine.podspec            # iOS placeholder (빈 파일)
```

---

## 2. 인터페이스 계약 (Spec 2와 공유)

### `NativeCanvasOSEngine.ts` — Codegen 스펙

이 파일이 Spec 1과 Spec 2의 **유일한 접점**이다.
양쪽 모두 이 타입에 맞춰 구현한다.

```typescript
import type { TurboModule } from 'react-native';
import { TurboModuleRegistry } from 'react-native';

export interface Spec extends TurboModule {
  // ─── Core Lifecycle ───
  initialize(dataDir: string): boolean;
  shutdown(): void;

  // ─── Engine State ───
  tick(n: number): number;        // engctx_tick × n, returns current tick
  getHash(): string;              // dk_canvas_hash() → hex string
  getStatus(): string;            // JSON: {tick, hash, branchId, openGates, visMode}

  // ─── Rendering ───
  setViewport(x: number, y: number, w: number, h: number, cellPx: number): void;
  setVisMode(mode: number): void; // 0=ABGR 1=Energy 2=Opcode 3=Lane 4=Activity
  renderFrame(): void;            // bridge_render_frame → dirty flag 기반 렌더링
  getFrameBuffer(): string;       // JSON: {ptr, width, height, byteLength}

  // ─── Cell Access ───
  getCell(x: number, y: number): string;  // JSON: {a,b,g,r}

  // ─── Shell (만능 명령 실행) ───
  exec(command: string): string;  // shell_exec_line → stdout capture → 문자열 반환

  // ─── Binary Export ───
  exportBMP(filePath: string): boolean;

  // ─── Batch Query (확장 가능한 읽기 인터페이스) ───
  query(request: string): string;
  // request: JSON 문자열 {"method":"xxx","params":{...}}
  // response: JSON 문자열 {"ok":true,"data":{...}} or {"ok":false,"error":"..."}
  //
  // 지원 메서드 (Phase별 추가):
  //   Phase 1: getStatus
  //   Phase 3: branchList, snapshotList, whLog, bhStatus,
  //            diffBranch, diffTick, traceLog, gateList,
  //            sourceMap, processList, cellRange
  //   Phase 5+: 새 메서드는 QueryDispatcher.cpp에 handler 등록만으로 추가
}

export default TurboModuleRegistry.getEnforcing<Spec>('CanvasOSEngine');
```

### 반환값 JSON 스키마 (Spec 2가 파싱할 형식)

```typescript
// getStatus() 반환
interface EngineStatus {
  tick: number;
  hash: string;
  branchId: number;
  openGates: number;
  visMode: number;
  pcX: number;
  pcY: number;
}

// getFrameBuffer() 반환
interface FrameBufferInfo {
  ptr: number;        // 네이티브 포인터 (double로 전달, 48bit 안전)
  width: number;
  height: number;
  byteLength: number; // width * height * 4 (RGBA)
}

// getCell() 반환
interface CellData {
  a: number;  // uint32 (A 채널)
  b: number;  // uint8  (B 채널 — opcode)
  g: number;  // uint8  (G 채널 — energy/state)
  r: number;  // uint8  (R 채널 — stream/data)
}

// query("branchList") 반환
interface BranchListResult {
  ok: true;
  data: Array<{
    id: number;
    parentId: number;
    planeMask: number;
    xMin: number; xMax: number;
    yMin: number; yMax: number;
    laneId: number;
    gatePolicy: number;
  }>;
}

// query("whLog") 반환
interface WhLogResult {
  ok: true;
  data: Array<{
    tick: number;
    opcode: number;
    targetAddr: number;
    targetKind: number;
    flags: number;
    param0: number;
  }>;
}

// query("diffBranch") / query("diffTick") 반환
interface DiffResult {
  ok: true;
  data: Array<{
    x: number; y: number;
    beforeB: number; beforeR: number;
    afterB: number; afterR: number;
  }>;
}

// query("gateList") 반환
interface GateListResult {
  ok: true;
  data: Array<{
    id: number;
    isOpen: boolean;
    tileX: number;
    tileY: number;
  }>;
}

// query("traceLog") 반환
interface TraceLogResult {
  ok: true;
  data: Array<{
    tick: number;
    pcX: number; pcY: number;
    opcode: number;
    regR: number;
  }>;
}

// query("processList") 반환
interface ProcessListResult {
  ok: true;
  data: Array<{
    pid: number;
    state: string;
    energy: number;
    pcX: number; pcY: number;
  }>;
}

// query("snapshotList") 반환
interface SnapshotListResult {
  ok: true;
  data: Array<{
    id: number;
    tick: number;
    branchId: number;
    hash: number;
  }>;
}
```

---

## 3. CMakeLists.txt 상세

```cmake
cmake_minimum_required(VERSION 3.18)
project(canvasos_engine)

# ── v2 C 엔진 소스 (수정 없음) ──
set(NATIVE_ROOT ${CMAKE_SOURCE_DIR}/../../../native/build)

file(GLOB ENGINE_C_SOURCES
    ${NATIVE_ROOT}/src/*.c
    ${NATIVE_ROOT}/src/tervas/*.c
)

# ── C++ Turbo Module 래퍼 ──
set(CPP_ROOT ${CMAKE_SOURCE_DIR}/../cpp)
set(TURBO_SOURCES
    ${CPP_ROOT}/CanvasOSEngineTurbo.cpp
    ${CPP_ROOT}/QueryDispatcher.cpp
    ${CPP_ROOT}/OnLoad.cpp
)

# ── 통합 라이브러리 ──
add_library(canvasos_engine SHARED
    ${ENGINE_C_SOURCES}
    ${TURBO_SOURCES}
)

target_include_directories(canvasos_engine PRIVATE
    ${NATIVE_ROOT}/include
    ${NATIVE_ROOT}/src          # 내부 헤더 참조용
    ${CPP_ROOT}
)

# C 소스는 C11, C++ 소스는 C++17
set_source_files_properties(${ENGINE_C_SOURCES} PROPERTIES LANGUAGE C)
target_compile_options(canvasos_engine PRIVATE
    -DANDROID
    -DCANVASOS_TURBO_MODULE      # C 쪽에서 main() 제외하는 플래그
    -fvisibility=hidden
    -O2
    -Wall
)

# ── React Native 링크 ──
find_package(fbjni REQUIRED CONFIG)
find_package(ReactAndroid REQUIRED CONFIG)

target_link_libraries(canvasos_engine
    fbjni::fbjni
    ReactAndroid::jsi
    ReactAndroid::turbomodulejsijni
    ReactAndroid::react_nativemodule_core
    android
    log
)
```

### 빌드 핵심: `CANVASOS_TURBO_MODULE` 플래그

v2의 `canvasos_launcher.c`에는 `main()` 함수가 있다. Turbo Module로 빌드할 때 이를 제외해야 한다.

**방법**: `canvasos_launcher.c`의 `main()`을 `#ifndef CANVASOS_TURBO_MODULE` 으로 감싸는 것이 유일한 v2 수정.

```c
// canvasos_launcher.c 하단 — 유일한 수정 (2줄 추가)
#ifndef CANVASOS_TURBO_MODULE
int main(int argc, char **argv) {
    // ... 기존 main 코드 ...
}
#endif
```

이렇게 하면 기존 CLI 빌드(`gcc`로 직접 빌드)는 그대로 동작하고,
Turbo Module 빌드(`-DCANVASOS_TURBO_MODULE`)에서만 main이 제외된다.

---

## 4. build.gradle

```groovy
// modules/canvasos-engine/android/build.gradle
buildscript {
    ext.safeExtGet = { prop, fallback ->
        rootProject.ext.has(prop) ? rootProject.ext.get(prop) : fallback
    }
}

apply plugin: 'com.android.library'

android {
    namespace 'com.canvasos.engine'
    compileSdkVersion safeExtGet('compileSdkVersion', 34)

    defaultConfig {
        minSdkVersion safeExtGet('minSdkVersion', 24)
        targetSdkVersion safeExtGet('targetSdkVersion', 34)

        externalNativeBuild {
            cmake {
                cppFlags "-std=c++17 -frtti -fexceptions"
                arguments "-DANDROID_STL=c++_shared"
                abiFilters "arm64-v8a", "armeabi-v7a"
            }
        }
    }

    externalNativeBuild {
        cmake {
            path "CMakeLists.txt"
        }
    }

    buildFeatures {
        prefab true
    }
}

dependencies {
    implementation "com.facebook.react:react-android"
}
```

---

## 5. Java 바인딩

### CanvasOSEnginePackage.java

```java
package com.canvasos.engine;

import com.facebook.react.TurboReactPackage;
import com.facebook.react.bridge.NativeModule;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.module.model.ReactModuleInfoProvider;
import com.facebook.react.module.model.ReactModuleInfo;
import java.util.HashMap;
import java.util.Map;

public class CanvasOSEnginePackage extends TurboReactPackage {
    @Override
    public NativeModule getModule(String name, ReactApplicationContext reactContext) {
        if (name.equals(CanvasOSEngineModule.NAME)) {
            return new CanvasOSEngineModule(reactContext);
        }
        return null;
    }

    @Override
    public ReactModuleInfoProvider getReactModuleInfoProvider() {
        return () -> {
            Map<String, ReactModuleInfo> map = new HashMap<>();
            map.put(CanvasOSEngineModule.NAME, new ReactModuleInfo(
                CanvasOSEngineModule.NAME,
                CanvasOSEngineModule.NAME,
                false, // canOverrideExistingModule
                false, // needsEagerInit
                false, // isCxxModule
                true   // isTurboModule
            ));
            return map;
        };
    }
}
```

### CanvasOSEngineModule.java

```java
package com.canvasos.engine;

import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.module.annotations.ReactModule;
import com.facebook.react.turbomodule.core.interfaces.TurboModule;

@ReactModule(name = CanvasOSEngineModule.NAME)
public class CanvasOSEngineModule extends NativeCanvasOSEngineSpec {
    public static final String NAME = "CanvasOSEngine";

    public CanvasOSEngineModule(ReactApplicationContext reactContext) {
        super(reactContext);
    }

    @Override
    public String getName() {
        return NAME;
    }

    // C++ 측에서 JSI로 직접 구현하므로 Java 메서드는 빈 껍데기
    // Codegen이 NativeCanvasOSEngineSpec을 자동 생성함

    static {
        System.loadLibrary("canvasos_engine");
    }
}
```

---

## 6. CanvasOSEngineTurbo.h

```cpp
#pragma once

#include <jsi/jsi.h>
#include <string>

// v2 C 엔진 헤더
extern "C" {
    #include "canvasos_types.h"
    #include "canvasos_engine_ctx.h"
    #include "canvasos_gui.h"
    #include "gui_engine_bridge.h"
    #include "canvasos_shell.h"
    #include "canvasos_debug_trace.h"
    #include "canvas_branch.h"
    #include "canvas_bh_compress.h"
    #include "canvasos_branch_diff.h"
    #include "canvasos_source_map.h"
    #include "canvasos_world_cmds.h"

    // fd.c 에서 제공하는 stdout 캡처
    extern uint16_t fd_stdout_get(uint8_t *buf, uint16_t max);
    extern void fd_stdout_clear(void);

    // 각 서브시스템 init 함수
    extern void engctx_init(EngineContext *ctx, Cell *cells, uint32_t count,
                            GateState *gates, uint8_t *active, RuleTable *rules);
    extern int  engctx_tick(EngineContext *ctx);
    extern void tervas_init(Tervas *tv);
    extern void ptl_state_init(PtlState *ptl, uint16_t x, uint16_t y);
    extern void proctable_init(ProcTable *pt, EngineContext *ctx);
    extern void syscall_set_tables(ProcTable *pt, PipeTable *pipes);
    extern void syscall_init(void);
    extern void vm_bridge_init(ProcTable *pt, PipeTable *pipes);
    extern void fd_table_init(void);
    extern void fd_set_pipe_table(PipeTable *pipes);
    extern void shell_init(Shell *sh, ProcTable *pt, PipeTable *pipes, EngineContext *ctx);
    extern int  shell_exec_line(Shell *sh, EngineContext *ctx, const char *line);
    extern void bridge_init(GuiEngineBridge *br, EngineContext *eng, GuiContext *gui);
    extern void bridge_render_frame(GuiEngineBridge *br);
    extern void bridge_set_viewport(GuiEngineBridge *br,
                                    uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t cellPx);
    extern void bridge_set_vis_mode(GuiEngineBridge *br, int mode);
    extern uint32_t dk_canvas_hash(const Cell *cells, uint32_t count);

    // CVP 세이브/로드
    extern int cvp_save_ctx(const EngineContext *ctx, const char *path, bool with_wh);
    extern int cvp_load_ctx(EngineContext *ctx, const char *path, bool reset_tick, ...);

    // BMP 출력
    extern int gui_bmp_write(const GuiBuffer *buf, const char *path);
}

namespace canvasos {

class CanvasOSEngineTurbo : public facebook::jsi::HostObject {
public:
    CanvasOSEngineTurbo();
    ~CanvasOSEngineTurbo();

    // JSI HostObject
    facebook::jsi::Value get(facebook::jsi::Runtime &rt,
                             const facebook::jsi::PropNameID &name) override;
    std::vector<facebook::jsi::PropNameID> getPropertyNames(
        facebook::jsi::Runtime &rt) override;

private:
    bool initialized_ = false;
    std::string dataDir_;

    // ── v2 엔진 글로벌 상태 (canvasos_launcher.c의 static 변수와 동일) ──
    Cell          cells_[1024 * 1024];       // 8 MB
    GateState     gates_[4096];              // 4 KB
    uint8_t       active_[4096];             // 4 KB
    EngineContext ctx_;
    Tervas        tv_;
    PtlState      ptl_;
    ProcTable     proctable_;
    PipeTable     pipetable_;
    Shell         shell_;

    // ── GUI 브릿지 (렌더링용) ──
    GuiContext        gui_;
    GuiEngineBridge   bridge_;
    GuiBuffer         framebuffer_;

    // ── 메서드 구현 ──
    bool        doInitialize(const std::string &dataDir);
    void        doShutdown();
    int         doTick(int n);
    std::string doGetHash();
    std::string doGetStatus();
    void        doSetViewport(int x, int y, int w, int h, int cellPx);
    void        doSetVisMode(int mode);
    void        doRenderFrame();
    std::string doGetFrameBuffer();
    std::string doGetCell(int x, int y);
    std::string doExec(const std::string &command);
    bool        doExportBMP(const std::string &path);
    std::string doQuery(const std::string &request);
};

// 모듈 설치 함수
void installCanvasOSEngine(facebook::jsi::Runtime &rt);

} // namespace canvasos
```

---

## 7. CanvasOSEngineTurbo.cpp 상세 구현

이 파일이 전체 명세의 **핵심**이다. 각 메서드의 구현을 정확히 기술한다.

### 7.1 초기화 (`doInitialize`)

v2 `canvasos_launcher.c` main()의 부팅 시퀀스를 그대로 재현:

```
순서:
1. memset(cells_, 0, sizeof(cells_))
2. memset(gates_, 0, sizeof(gates_))
3. memset(active_, 0, sizeof(active_))
4. engctx_init(&ctx_, cells_, 1024*1024, gates_, active_, NULL)
5. tervas_init(&tv_)
6. ptl_state_init(&ptl_, 512, 512)     // ORIGIN_X, ORIGIN_Y
7. proctable_init(&proctable_, &ctx_)
8. memset(&pipetable_, 0, sizeof(pipetable_))
9. syscall_set_tables(&proctable_, &pipetable_)
10. syscall_init()
11. vm_bridge_init(&proctable_, &pipetable_)
12. fd_table_init()
13. fd_set_pipe_table(&pipetable_)
14. shell_init(&shell_, &proctable_, &pipetable_, &ctx_)
15. 세션 복원 시도: cvp_load_ctx(&ctx_, dataDir+"/session.cvp", false, ...)
16. engctx_tick(&ctx_)  // 첫 번째 tick
17. GUI 브릿지 초기화:
    - framebuffer_ 할당 (기본 뷰포트 64x64 cells @ 4px = 256x256 RGBA)
    - gui_ 초기화 (framebuffer_ 연결)
    - bridge_init(&bridge_, &ctx_, &gui_)
    - bridge_set_viewport(&bridge_, 480, 480, 64, 64, 4)  // 중앙 부근
    - bridge_set_vis_mode(&bridge_, 0)  // ABGR
    - bridge_render_frame(&bridge_)
18. initialized_ = true
```

### 7.2 Shell 실행 (`doExec`)

```
1. fd_stdout_clear()                     // 캡처 버퍼 비우기
2. int rc = shell_exec_line(&shell_, &ctx_, command.c_str())
3. uint8_t outbuf[4096]
4. uint16_t len = fd_stdout_get(outbuf, 4096)
5. return std::string(outbuf, len)        // 캡처된 stdout 반환
```

**주의**: `shell_exec_line`이 stdout에 출력한 모든 내용이 `fd_stdout_get`으로 캡처된다.
이것이 `exec()` API의 핵심 메커니즘이다.

### 7.3 렌더링 (`doRenderFrame` + `doGetFrameBuffer`)

```
doRenderFrame():
  bridge_render_frame(&bridge_)
  // dirty flag 기반 — 변경 없으면 no-op

doGetFrameBuffer():
  return JSON: {
    "ptr": (double)(uintptr_t)framebuffer_.pixels,
    "width": framebuffer_.w,
    "height": framebuffer_.h,
    "byteLength": framebuffer_.w * framebuffer_.h * 4
  }
```

**포인터 안전성**: ARM64 Android의 유저스페이스 포인터는 48비트.
`double`의 mantissa는 52비트이므로 정밀도 손실 없이 전달 가능.

### 7.4 뷰포트 변경 (`doSetViewport`)

```
1. 기존 framebuffer_.pixels 해제 (if owned)
2. 새 크기 계산: pixelW = w * cellPx, pixelH = h * cellPx
3. framebuffer_.pixels = malloc(pixelW * pixelH * 4)
4. framebuffer_.w = pixelW, framebuffer_.h = pixelH
5. bridge_set_viewport(&bridge_, x, y, w, h, cellPx)
6. bridge_render_frame(&bridge_)  // 즉시 다시 렌더링
```

**Spec 2 참고**: 뷰포트 변경 시 `getFrameBuffer()`를 다시 호출하여 새 포인터를 받아야 함.

### 7.5 BMP Export (`doExportBMP`)

```
1. bridge_render_frame(&bridge_)      // 최신 상태 보장
2. int ok = gui_bmp_write(&framebuffer_, path.c_str())
3. return ok == 0
```

### 7.6 해시 (`doGetHash`)

```
1. uint32_t h = dk_canvas_hash(cells_, 1024*1024)
2. char hex[9]; snprintf(hex, 9, "%08x", h)
3. return std::string(hex)
```

**중요**: JS의 `canvas-renderer.ts`에 있던 JS 해시 함수와 다른 결과를 냄.
반드시 C의 DK 해시만 사용해야 결정론성이 보존됨.

### 7.7 상태 조회 (`doGetStatus`)

```
JSON 직렬화:
{
  "tick": ctx_.tick,
  "hash": doGetHash(),
  "branchId": 현재 활성 브랜치 ID,
  "openGates": 열린 게이트 수 (gates_[] 순회),
  "visMode": bridge_.vis_mode,
  "pcX": ptl_.cursor_x,
  "pcY": ptl_.cursor_y
}
```

---

## 8. QueryDispatcher 상세

JSON-RPC 스타일의 확장 가능한 쿼리 라우터.

### 설계

```cpp
// QueryDispatcher.h
#pragma once
#include <string>
#include <unordered_map>
#include <functional>

struct EngineContext;
struct Shell;
struct GuiEngineBridge;

namespace canvasos {

using QueryHandler = std::function<std::string(const std::string& params)>;

class QueryDispatcher {
public:
    void registerHandler(const std::string& method, QueryHandler handler);
    std::string dispatch(const std::string& jsonRequest);

    // 엔진 컨텍스트 연결
    void setContext(EngineContext *ctx, Shell *shell, GuiEngineBridge *bridge);

private:
    std::unordered_map<std::string, QueryHandler> handlers_;
    EngineContext *ctx_ = nullptr;
    Shell *shell_ = nullptr;
    GuiEngineBridge *bridge_ = nullptr;
};

} // namespace canvasos
```

### 핸들러 등록 (Phase별)

```cpp
// QueryDispatcher.cpp 초기화 시
void QueryDispatcher::registerDefaultHandlers() {
    // ── Phase 1 ──
    registerHandler("getStatus", [this](auto& p) { return handleGetStatus(p); });

    // ── Phase 3 ──
    registerHandler("branchList",   [this](auto& p) { return handleBranchList(p); });
    registerHandler("snapshotList", [this](auto& p) { return handleSnapshotList(p); });
    registerHandler("whLog",        [this](auto& p) { return handleWhLog(p); });
    registerHandler("bhStatus",     [this](auto& p) { return handleBhStatus(p); });
    registerHandler("diffBranch",   [this](auto& p) { return handleDiffBranch(p); });
    registerHandler("diffTick",     [this](auto& p) { return handleDiffTick(p); });
    registerHandler("gateList",     [this](auto& p) { return handleGateList(p); });
    registerHandler("traceLog",     [this](auto& p) { return handleTraceLog(p); });
    registerHandler("processList",  [this](auto& p) { return handleProcessList(p); });
    registerHandler("sourceMap",    [this](auto& p) { return handleSourceMap(p); });
    registerHandler("cellRange",    [this](auto& p) { return handleCellRange(p); });

    // ── Phase 5+ 확장 ──
    // 새 핸들러 등록만 하면 JS 변경 없이 즉시 사용 가능
}
```

### 확장 패턴 예시

```cpp
// 새 기능 추가 시:
registerHandler("canvasfsLs", [this](const std::string& params) {
    // params에서 path 파싱
    // C의 canvasfs 함수 호출
    // JSON 결과 반환
    return R"({"ok":true,"data":[...]})";
});
// → JS에서 query('{"method":"canvasfsLs","params":{"path":"/"}}') 호출 가능
// → UI 패널 코드 수정 없음
```

---

## 9. OnLoad.cpp

```cpp
#include <fbjni/fbjni.h>
#include <ReactCommon/CxxTurboModuleUtils.h>
#include "CanvasOSEngineTurbo.h"

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *) {
    return facebook::jni::initialize(vm, [] {
        // TurboModule 팩토리 등록
        facebook::react::registerCxxModuleToGlobalModuleMap(
            "CanvasOSEngine",
            [](std::shared_ptr<facebook::react::CallInvoker> jsInvoker) {
                // JSI HostObject를 TurboModule로 래핑
                // (실제 구현은 installCanvasOSEngine에서 Runtime에 직접 설치)
                return nullptr;
            }
        );
    });
}
```

---

## 10. JS Export

### `src/index.ts`

```typescript
import NativeCanvasOSEngine from './NativeCanvasOSEngine';
export default NativeCanvasOSEngine;
export type { Spec as CanvasOSEngineSpec } from './NativeCanvasOSEngine';
```

---

## 11. 구현 Phase + 검증 체크리스트

### Phase 1: 빌드 + 기본 동작 (exec, tick, hash)

**작업**:
- [ ] `CMakeLists.txt` 작성
- [ ] `build.gradle` 작성
- [ ] `CanvasOSEnginePackage.java`, `CanvasOSEngineModule.java` 작성
- [ ] `CanvasOSEngineTurbo.h/.cpp` — `initialize`, `shutdown`, `tick`, `getHash`, `exec`, `getStatus` 구현
- [ ] `OnLoad.cpp` 작성
- [ ] `NativeCanvasOSEngine.ts` 작성
- [ ] `canvasos_launcher.c`에 `#ifndef CANVASOS_TURBO_MODULE` 추가 (main만 감싸기)

**검증** (Spec 2와 독립적으로 가능):
```
1. npx expo run:android → 빌드 성공, 크래시 없음
2. JS 콘솔에서: NativeCanvasOSEngine.initialize("/data/.../files") === true
3. NativeCanvasOSEngine.tick(10) → 11 반환 (초기 tick=1 + 10)
4. NativeCanvasOSEngine.getHash() → "00000000"이 아닌 8자리 hex
5. NativeCanvasOSEngine.exec("hash") → "Hash: XXXXXXXX" 형태 문자열
6. NativeCanvasOSEngine.exec("gate open 0") → "OK" 포함 문자열
7. NativeCanvasOSEngine.exec("info") → tick, hash, gates 정보 포함
8. NativeCanvasOSEngine.getStatus() → 유효한 JSON 파싱 가능
```

### Phase 2: 렌더링 + Cell 접근

**작업**:
- [ ] `doSetViewport`, `doSetVisMode`, `doRenderFrame`, `doGetFrameBuffer` 구현
- [ ] `doGetCell` 구현
- [ ] `doExportBMP` 구현
- [ ] GuiBuffer 메모리 관리 (뷰포트 리사이즈 시 realloc)

**검증**:
```
1. setViewport(480, 480, 64, 64, 4) → 크래시 없음
2. renderFrame() → 크래시 없음
3. getFrameBuffer() → {ptr: 비zero, width: 256, height: 256, byteLength: 262144}
4. exec("B=01 R=48 !") → 셀 쓰기 후 renderFrame() → ptr의 RGBA 값 변경 확인
5. getCell(512, 512) → {a,b,g,r} JSON 파싱 가능
6. exportBMP("/data/.../test.bmp") → true, 파일 존재 확인
7. 5개 visMode (0-4) 순회하며 setVisMode → renderFrame → 크래시 없음
```

### Phase 3: Query 시스템

**작업**:
- [ ] `QueryDispatcher.h/.cpp` 구현
- [ ] 모든 핸들러 등록 (branchList, whLog, diffBranch, gateList, traceLog, processList, snapshotList, sourceMap, cellRange)
- [ ] `doQuery()` 에서 dispatcher 호출

**검증**:
```
1. query('{"method":"gateList"}') → {"ok":true,"data":[...]} 파싱 가능
2. exec("branch create test") 후 query('{"method":"branchList"}') → 브랜치 포함
3. exec("trace on") → exec("tick 5") → query('{"method":"traceLog","params":{"count":5}}')
   → 5개 트레이스 이벤트 반환
4. query('{"method":"whLog","params":{"count":10}}') → WH 레코드 반환
5. query('{"method":"없는메서드"}') → {"ok":false,"error":"unknown method"} 반환
```

---

## 12. 에러 처리 규칙

1. C 함수가 -1 반환 시 → `exec()`는 `"ERROR: ..."` 문자열 반환
2. `query()`에서 JSON 파싱 실패 → `{"ok":false,"error":"invalid JSON"}`
3. 초기화 전 호출 → `exec()`는 `"ERROR: not initialized"` 반환
4. NULL 포인터 / OOB → C++ try-catch로 감싸되, C 엔진 내부 크래시는 방지 불가
5. **절대로 C 엔진의 에러를 삼키지 않는다** — 에러 메시지를 그대로 JS에 전달

---

## 13. 메모리 예산

| 항목 | 크기 |
|------|------|
| Cell 배열 | 8 MB |
| Gate + Active | 8 KB |
| WH 링버퍼 (캔버스 내 영역) | 0 (cells_ 내 포함) |
| GuiBuffer (256x256 기본) | 256 KB |
| Shell + Tervas + ProcTable 등 | ~64 KB |
| **합계** | **~8.3 MB** |

모바일에서 충분히 안전한 수준.
