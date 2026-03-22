# 명세서 2: UI Shell (React Native / Expo Layer)

> **담당**: Expo 프로젝트, 화면, 패널 시스템, 훅, 상태관리, Skia 렌더링
> **의존**: `NativeCanvasOSEngine.ts` (Spec 1이 제공하는 Turbo Module)
> **산출물**: 프로젝트 루트 전체 (modules/ 제외)
> **통합 인터페이스**: `NativeCanvasOSEngine.ts` (Spec 1과의 계약)

---

## 0. SJ CANVAS 태생적 장점 — UI 측 보존 원칙

| 장점 | UI에서 보존하는 방법 |
|------|---------------------|
| **Canvas-as-Memory** | 캔버스를 단순 이미지가 아닌 **인터랙티브 공간**으로 표현. 터치 → 셀 좌표 → 셀 정보 즉시 표시. "그리드가 곧 세계" 느낌 유지 |
| **Determinism** | UI에서 해시/틱을 항상 표시하여 "같은 입력 → 같은 결과" 확인 가능하게. 해시 값은 C에서만 받음 |
| **WH/BH 시간축** | TimelinePanel에서 시간을 **공간적으로** 시각화 (가로축=틱, WH=흰색점, BH=검은색 압축 구간). 타임워프를 드래그로 조작 |
| **Gate 시스템** | GatePanel + CanvasPanel 오버레이로 "공간적 접근 제어"를 시각적으로 표현. 열린 게이트 = 밝은 타일, 닫힌 = 어두운 타일 |
| **이중 언어** | TerminalPanel에서 한글코드/PixelCode 전환 버튼. 건반 키보드 레이아웃으로 "피아노 치듯 코딩" 경험 보존 |
| **Branch/Multiverse** | BranchPanel에서 브랜치를 **나무 구조**로 시각화. 현재 위치 강조. "평행 우주" 메타포 유지 |
| **BMP 기반 시각화** | DiffPanel에서 BMP export를 활용한 side-by-side 비교. 공간적 diff (변경 셀 위치 하이라이트) |

---

## 1. Spec 1과의 연결 지점

### Mock Module (Spec 1 완성 전 독립 개발용)

Spec 1이 완성되기 전에 UI를 개발하기 위해 **Mock Turbo Module**을 제공한다.
이것은 Spec 2의 `src/mock/` 에 위치하며, `__DEV__` 모드에서만 사용된다.

```typescript
// src/mock/MockCanvasOSEngine.ts
// NativeCanvasOSEngine.ts와 동일한 인터페이스, JS로 시뮬레이션
export const MockEngine = {
  initialize: (dataDir: string) => true,
  shutdown: () => {},
  tick: (n: number) => { mockTick += n; return mockTick; },
  getHash: () => "deadbeef",
  getStatus: () => JSON.stringify({
    tick: mockTick, hash: "deadbeef", branchId: 0,
    openGates: 0, visMode: 0, pcX: 512, pcY: 512
  }),
  setViewport: () => {},
  setVisMode: () => {},
  renderFrame: () => {},
  getFrameBuffer: () => JSON.stringify({
    ptr: 0, width: 256, height: 256, byteLength: 262144
  }),
  getCell: (x: number, y: number) => JSON.stringify({ a: 0, b: 0, g: 0, r: 0 }),
  exec: (cmd: string) => `[mock] ${cmd}\nOK`,
  exportBMP: () => true,
  query: (req: string) => {
    const { method } = JSON.parse(req);
    return JSON.stringify({ ok: true, data: [], _mock: true });
  },
};
```

### 엔진 접근 통합 포인트

```typescript
// src/hooks/useEngine.ts
import { Platform } from 'react-native';

function getEngine() {
  if (__DEV__ && Platform.OS === 'web') {
    return require('../mock/MockCanvasOSEngine').MockEngine;
  }
  return require('../../modules/canvasos-engine/src').default;
}
```

**통합 테스트 시**: Mock을 제거하고 실제 Turbo Module로 전환.
`getEngine()` 한 곳만 변경하면 전체 앱이 실제 엔진에 연결됨.

---

## 2. 프로젝트 구조

```
canvasos-hybrid/
├── app/
│   ├── (tabs)/
│   │   ├── _layout.tsx           # [2.3] 3탭 (홈, 워크스페이스, 설정)
│   │   ├── index.tsx             # [2.4] 홈 대시보드
│   │   ├── workspace.tsx         # [2.5] 워크스페이스 (패널 시스템)
│   │   └── settings.tsx          # [2.6] 설정
│   ├── _layout.tsx               # [2.7] Root (EngineProvider)
│   └── +not-found.tsx
│
├── modules/canvasos-engine/      # ★ Spec 1이 구현 (여기서는 Mock만 사용)
│   └── src/
│       └── NativeCanvasOSEngine.ts  # 계약 파일 (양쪽 공유)
│
├── native/build/                 # v2 C 엔진 (복사만, Spec 1이 빌드)
│
├── src/
│   ├── panels/                   # [2.8] 패널 시스템 ★확장 핵심★
│   │   ├── registry.ts           # [2.8.1] 패널 레지스트리
│   │   ├── CanvasPanel.tsx       # [2.8.2] 캔버스 Skia 렌더링
│   │   ├── TerminalPanel.tsx     # [2.8.3] Shell + PixelCode
│   │   ├── CellInspectorPanel.tsx # [2.8.4] 셀 상세 정보
│   │   ├── GatePanel.tsx         # [2.8.5] 게이트 제어
│   │   ├── BranchPanel.tsx       # [2.8.6] 브랜치 트리
│   │   ├── TimelinePanel.tsx     # [2.8.7] WH/BH 타임라인
│   │   ├── DiffPanel.tsx         # [2.8.8] 상태 비교 (BMP)
│   │   ├── TracePanel.tsx        # [2.8.9] VM 실행 트레이스
│   │   ├── MinimapPanel.tsx      # [2.8.10] 축소 미니맵
│   │   └── ProcessPanel.tsx      # [2.8.11] 프로세스 관리
│   │
│   ├── components/               # [2.9] 재사용 UI 빌딩블록
│   │   ├── SkiaCanvas.tsx        # [2.9.1] Skia 기반 캔버스 뷰
│   │   ├── PanelContainer.tsx    # [2.9.2] 패널 프레임
│   │   ├── WorkspaceLayout.tsx   # [2.9.3] 패널 배치 관리자
│   │   ├── StatusBar.tsx         # [2.9.4] 엔진 상태바
│   │   ├── VisModePicker.tsx     # [2.9.5] 시각화 모드 선택
│   │   └── KeyboardLayout.tsx    # [2.9.6] 건반 모드 키보드
│   │
│   ├── hooks/                    # [2.10] 엔진 접근 훅
│   │   ├── useEngine.ts          # [2.10.1] 엔진 싱글톤
│   │   ├── useCanvas.ts          # [2.10.2] 뷰포트 + 렌더링
│   │   ├── useShell.ts           # [2.10.3] 명령 실행 + 히스토리
│   │   ├── useBranch.ts          # [2.10.4] 브랜치 CRUD
│   │   ├── useTimeline.ts        # [2.10.5] 스냅샷 + 타임워프
│   │   ├── useGate.ts            # [2.10.6] 게이트 상태
│   │   └── useQuery.ts           # [2.10.7] query() 래퍼
│   │
│   ├── stores/                   # [2.11] Zustand 상태관리
│   │   ├── engineStore.ts        # [2.11.1] 엔진 상태 미러
│   │   ├── uiStore.ts            # [2.11.2] UI 레이아웃 상태
│   │   └── projectStore.ts       # [2.11.3] 프로젝트 메타
│   │
│   ├── mock/                     # [2.12] Mock 엔진 (개발용)
│   │   └── MockCanvasOSEngine.ts
│   │
│   └── types/
│       └── engine.ts             # [2.13] C 구조체 매칭 타입
│
├── components/                   # v3.2에서 유지하는 공용 컴포넌트
│   ├── screen-container.tsx      # 유지
│   ├── themed-view.tsx           # 유지
│   ├── haptic-tab.tsx            # 유지
│   └── ui/
│       ├── collapsible.tsx       # 유지
│       └── icon-symbol.tsx       # 유지
│
├── constants/
│   └── theme.ts                  # v3.2 디자인 토큰 유지
├── hooks/
│   ├── use-colors.ts             # v3.2 유지
│   └── use-color-scheme.ts       # v3.2 유지
├── lib/
│   ├── theme-provider.tsx        # v3.2 유지
│   ├── utils.ts                  # v3.2 유지
│   └── project-context.tsx       # 수정: canvasData 제거, cvpPath만 저장
│
├── assets/                       # v3.2 이미지 유지
├── global.css                    # v3.2 유지
├── tailwind.config.js            # v3.2 유지
├── babel.config.js               # v3.2 + worklets 플러그인 추가
├── metro.config.js               # v3.2 유지
├── tsconfig.json                 # v3.2 + src/ 경로 추가
├── app.config.ts                 # 수정: 패키지명, 플러그인 조정
└── package.json                  # 수정: 불필요 의존성 제거, skia 추가
```

---

## 3. 탭 레이아웃 (`_layout.tsx`)

### 3탭 구조

```
┌───────┬────────────┬────────┐
│  홈   │ 워크스페이스 │  설정  │
│  🏠   │    🎨      │  ⚙️   │
└───────┴────────────┴────────┘
```

| 탭 | 화면 | 목적 |
|----|------|------|
| **홈** | 대시보드 + 프로젝트 목록 + CTA | 진입점, 상태 요약 |
| **워크스페이스** | 캔버스 + 하위 패널 (자유 조합) | 핵심 작업 공간 |
| **설정** | CVP 관리, 테마, Det 모드, 앱 정보 | 설정/관리 |

탭 3개로 시작하며, 탭 추가 필요 없이 **패널 시스템**으로 기능 확장.

### 구현 코드 패턴 (v3.2 _layout.tsx 기반)

```tsx
// app/(tabs)/_layout.tsx
import { Tabs } from 'expo-router';
import { useColors } from '@/hooks/use-colors';
import { HapticTab } from '@/components/haptic-tab';
import { IconSymbol } from '@/components/ui/icon-symbol';

export default function TabLayout() {
  const colors = useColors();
  return (
    <Tabs screenOptions={{
      headerShown: false,
      tabBarButton: HapticTab,
      tabBarActiveTintColor: colors.primary,
      tabBarInactiveTintColor: colors.muted,
      tabBarStyle: {
        backgroundColor: colors.background,
        borderTopColor: colors.border,
        borderTopWidth: 0.5,
      },
    }}>
      <Tabs.Screen name="index" options={{
        title: '홈',
        tabBarIcon: ({ color }) => <IconSymbol name="house.fill" color={color} />,
      }} />
      <Tabs.Screen name="workspace" options={{
        title: '워크스페이스',
        tabBarIcon: ({ color }) => <IconSymbol name="square.grid.2x2.fill" color={color} />,
      }} />
      <Tabs.Screen name="settings" options={{
        title: '설정',
        tabBarIcon: ({ color }) => <IconSymbol name="gearshape.fill" color={color} />,
      }} />
    </Tabs>
  );
}
```

---

## 4. 홈 화면 (`index.tsx`)

v3.2의 홈 화면 디자인 유지 + **라이브 엔진 상태** 추가.

### 구성 요소

```
┌─────────────────────────────┐
│  Canvas-OS     [엔진 상태]  │  ← 헤더 + StatusBar
├─────────────────────────────┤
│  ┌───────────────────────┐  │
│  │     미니맵 (축소)      │  │  ← MinimapPanel 재사용 (128x128)
│  │   [ABGR] 1024x1024    │  │
│  └───────────────────────┘  │
├─────────────────────────────┤
│  Quick Stats:               │
│  ┌──────┐ ┌──────┐ ┌────┐  │
│  │tick  │ │gates │ │br  │  │  ← 엔진 실시간 데이터
│  │1,234 │ │12/256│ │ 0  │  │
│  └──────┘ └──────┘ └────┘  │
├─────────────────────────────┤
│  Core Features (v3.2 유지)  │
│  공간실행 이중언어 멀티버스   │
├─────────────────────────────┤
│  [새 프로젝트] [불러오기]    │  ← CTA 버튼
│  최근 프로젝트 목록          │
└─────────────────────────────┘
```

**엔진 데이터 접근**:
```typescript
const engine = useEngine();
const status = engine.getStatus();  // {tick, hash, openGates, branchId}
// 1초마다 폴링하여 Quick Stats 업데이트
```

---

## 5. 워크스페이스 (`workspace.tsx`)

### 핵심: 패널 시스템

```
┌──────────────────────────────────┐
│ StatusBar: tick:1234 | hash:a3f2 │
├──────────────────────────────────┤
│                                  │
│  CanvasPanel (Skia)              │  ← 상단 60% (조절 가능)
│  [핀치줌] [팬] [터치→셀정보]     │
│  VisMode: [ABGR][E][O][L][A]    │
│                                  │
├──────────────────────────────────┤
│ [터미널][셀][게이트][브랜치]       │  ← 패널 탭 바
│ [타임라인][트레이스][Diff][미니맵] │
├──────────────────────────────────┤
│  TerminalPanel (활성 패널)       │  ← 하단 40%
│  $ gate open 5                   │
│  > OK                            │
│  $ _                             │
└──────────────────────────────────┘
```

### 구현

```tsx
// app/(tabs)/workspace.tsx
import { WorkspaceLayout } from '@/src/components/WorkspaceLayout';
import { StatusBar } from '@/src/components/StatusBar';
import { CanvasPanel } from '@/src/panels/CanvasPanel';
import { useUiStore } from '@/src/stores/uiStore';
import { panelRegistry } from '@/src/panels/registry';

export default function WorkspaceScreen() {
  const { activePanelId } = useUiStore();
  const ActivePanel = panelRegistry.get(activePanelId)?.component;

  return (
    <ScreenContainer edges={['top']}>
      <StatusBar />
      <WorkspaceLayout
        mainPanel={<CanvasPanel />}
        subPanel={ActivePanel ? <ActivePanel /> : null}
        panelTabs={panelRegistry.getAll()}
      />
    </ScreenContainer>
  );
}
```

---

## 6. 설정 화면 (`settings.tsx`)

```
┌─────────────────────────────┐
│  설정                        │
├─────────────────────────────┤
│  세션 관리                   │
│  [저장] [불러오기] [리셋]    │  ← exec("cvp save/load"), exec("reset")
├─────────────────────────────┤
│  엔진 설정                   │
│  결정론 모드  [ON/OFF]       │  ← exec("det on/off")
│  자동 저장    [ON/OFF]       │
├─────────────────────────────┤
│  테마                        │
│  [라이트] [다크] [시스템]    │  ← uiStore.theme
├─────────────────────────────┤
│  앱 정보                     │
│  CanvasOS v4.0 Hybrid        │
│  Engine: v2.1 (C)            │
│  UI: v3.2 (React Native)    │
└─────────────────────────────┘
```

---

## 7. Root Layout (`app/_layout.tsx`)

v3.2에서 tRPC/Auth/Manus 보일러플레이트 제거하고 EngineProvider 추가.

```tsx
// app/_layout.tsx
import { Stack } from 'expo-router';
import { GestureHandlerRootView } from 'react-native-gesture-handler';
import { SafeAreaProvider } from 'react-native-safe-area-context';
import { ThemeProvider } from '@/lib/theme-provider';
import { EngineProvider } from '@/src/hooks/useEngine';

export default function RootLayout() {
  return (
    <GestureHandlerRootView style={{ flex: 1 }}>
      <SafeAreaProvider>
        <ThemeProvider>
          <EngineProvider>
            <Stack screenOptions={{ headerShown: false }}>
              <Stack.Screen name="(tabs)" />
            </Stack>
          </EngineProvider>
        </ThemeProvider>
      </SafeAreaProvider>
    </GestureHandlerRootView>
  );
}
```

---

## 8. 패널 시스템 상세

### 8.1 패널 레지스트리 (`registry.ts`)

```typescript
// src/panels/registry.ts
import { ComponentType } from 'react';

export interface PanelDef {
  id: string;
  label: string;
  icon: string;          // IconSymbol name
  component: ComponentType;
  phase: number;         // 구현 Phase (정렬/필터용)
}

const panels: Map<string, PanelDef> = new Map();

export const panelRegistry = {
  register(def: PanelDef) {
    panels.set(def.id, def);
  },
  get(id: string) {
    return panels.get(id);
  },
  getAll(): PanelDef[] {
    return Array.from(panels.values()).sort((a, b) => a.phase - b.phase);
  },
};

// ── 등록 (각 패널 파일에서 import 시 자동 등록) ──
// 새 패널 추가 = 이 파일에 import 1줄 + 패널 파일 1개
import './CanvasPanel';
import './TerminalPanel';
import './CellInspectorPanel';
import './GatePanel';
import './BranchPanel';
import './TimelinePanel';
import './DiffPanel';
import './TracePanel';
import './MinimapPanel';
import './ProcessPanel';
```

### 확장 방법

```typescript
// 예: 새 "CanvasFS 브라우저" 패널 추가 시

// 1. src/panels/CanvasFSPanel.tsx 생성
// 2. registry.ts 하단에 import './CanvasFSPanel' 추가
// 3. 끝. 워크스페이스에 자동으로 탭 등장.
```

### 8.2 CanvasPanel (핵심 — Skia 렌더링)

```typescript
// src/panels/CanvasPanel.tsx
import { Canvas, Image, useImage } from '@shopify/react-native-skia';
import { Gesture, GestureDetector } from 'react-native-gesture-handler';
import { useCanvas } from '@/src/hooks/useCanvas';
import { VisModePicker } from '@/src/components/VisModePicker';
import { panelRegistry } from './registry';

function CanvasPanelComponent() {
  const {
    frameImage,     // Skia Image (C 픽셀 버퍼에서 생성)
    viewport,       // {x, y, w, h, cellPx}
    visMode,        // 0-4
    setVisMode,
    onPinch,        // 핀치줌 핸들러
    onPan,          // 팬 핸들러
    onTap,          // 탭 핸들러 (셀 선택)
    selectedCell,   // {x, y} | null
  } = useCanvas();

  const gesture = Gesture.Simultaneous(
    Gesture.Pinch().onUpdate(onPinch),
    Gesture.Pan().onUpdate(onPan),
    Gesture.Tap().onEnd(onTap),
  );

  return (
    <View style={{ flex: 1 }}>
      <VisModePicker current={visMode} onChange={setVisMode} />
      <GestureDetector gesture={gesture}>
        <Canvas style={{ flex: 1 }}>
          {frameImage && <Image image={frameImage} fit="contain" />}
          {/* 게이트 오버레이, 그리드, 커서 등은 추가 Skia Path로 */}
        </Canvas>
      </GestureDetector>
      {selectedCell && (
        <Text style={styles.cellInfo}>
          ({selectedCell.x}, {selectedCell.y})
        </Text>
      )}
    </View>
  );
}

panelRegistry.register({
  id: 'canvas',
  label: '캔버스',
  icon: 'square.grid.2x2.fill',
  component: CanvasPanelComponent,
  phase: 2,
});
```

### 8.3 TerminalPanel

```
구성:
┌──────────────────────────────┐
│ [텍스트모드] [건반모드]        │  ← 모드 전환
├──────────────────────────────┤
│ $ gate open 5                │
│ > OK                         │  ← ScrollView (히스토리)
│ $ B=01 R=48 !                │
│ > Cell written at (512,512)  │
│ $ ㅎㅏ 48                     │
│ > 한글코드 실행됨             │
├──────────────────────────────┤
│ $ [입력창]            [실행]  │  ← TextInput + 버튼
└──────────────────────────────┘

건반모드:
┌──────────────────────────────┐
│ [ㄱ][ㄲ][ㄴ][ㄷ][ㄸ][ㄹ]... │  ← 자음 행
│ [ㅏ][ㅑ][ㅓ][ㅕ][ㅗ][ㅛ]... │  ← 모음 행
│ [0][1][2]...[F]              │  ← 16진수 파라미터
│ [!][tk][←][→][↑][↓]         │  ← 커밋 + 이동
└──────────────────────────────┘
```

**엔진 연동**:
```typescript
// useShell 훅
const { execAndCapture, history } = useShell();

async function handleSubmit(command: string) {
  const output = execAndCapture(command);
  // output = exec(command) 결과
  // history에 {input: command, output} 추가
}
```

**건반 모드 핵심**: 자음+모음 조합 → `exec("ㅎㅏ 48")` → C의 pixelcode.c가 파싱.
JS에서 파싱하지 않음. 키보드는 입력 UI만 제공.

### 8.4 CellInspectorPanel

```
선택된 셀 (512, 513):
┌────────────────────────┐
│ A: 0x00000000 (주소)   │
│ B: 0x01 (SET)          │  ← opcode 이름 매핑 (JS 상수 테이블)
│ G: 128 (에너지)         │
│ R: 0x48 ('H')          │  ← ASCII 변환
├────────────────────────┤
│ 타일: (32, 32)         │
│ 게이트: #2080 [열림]   │
│ 레인: 0                │
└────────────────────────┘
```

**엔진 연동**: `getCell(x, y)` → JSON 파싱 → 화면 표시

### 8.5 GatePanel

```
게이트 제어:
┌────────────────────────────┐
│ 열린 게이트: 12 / 4096     │
│                            │
│ [0: 열림 ●] [1: 닫힘 ○]   │
│ [2: 닫힘 ○] [3: 열림 ●]   │  ← 터치로 토글
│ ...                        │
│                            │
│ [모두 열기] [모두 닫기]     │
└────────────────────────────┘
```

**엔진 연동**:
- 목록: `query('{"method":"gateList"}')` → 게이트 상태 리스트
- 토글: `exec("gate open N")` 또는 `exec("gate close N")`

### 8.6 BranchPanel

```
브랜치 트리:
┌────────────────────────────┐
│  ● main (br:0) ← 현재     │
│  ├─ ○ experiment (br:1)   │
│  │  └─ ○ hotfix (br:3)    │
│  └─ ○ feature (br:2)      │
│                            │
│ [생성] [전환] [머지] [삭제] │
└────────────────────────────┘
```

**엔진 연동**:
- 목록: `query('{"method":"branchList"}')`
- 생성: `exec("branch create name")`
- 전환: `exec("branch switch N")`
- 머지: `exec("merge A B")`

### 8.7 TimelinePanel

```
타임라인 (가로 스크롤):
┌──────────────────────────────────────┐
│  tick: 0    500    1000   1500  현재 │
│  ──●──────●──────────●────────●──▶  │
│    ↑snap0  ↑snap1    ↑snap2   ↑now  │
│                                      │
│  WH: ████░░░░████████░░██████████   │  ← 흰색=이벤트, 회색=idle
│  BH: ░░░░████░░░░░░░░████░░░░░░░   │  ← 검정=압축됨
│                                      │
│  [◁ -100] [되감기] [▷ +100]         │
└──────────────────────────────────────┘
```

**엔진 연동**:
- 스냅샷: `query('{"method":"snapshotList"}')`
- WH 로그: `query('{"method":"whLog","params":{"count":100}}')`
- 되감기: `exec("rewind 500")`
- 스냅샷 생성: `exec("snapshot checkpoint_1")`

### 8.8 DiffPanel (BMP 연동)

```
Diff 비교:
┌──────────────┐ ┌──────────────┐
│  Branch A    │ │  Branch B    │
│  (BMP 이미지) │ │  (BMP 이미지) │
│              │ │              │
│  tick: 150   │ │  tick: 200   │
└──────────────┘ └──────────────┘
변경: 42 셀  [빨간 점으로 표시]

[Branch A ▼] [Branch B ▼] [비교 실행]
```

**BMP 연동 플로우**:
```typescript
async function runDiff(branchA: number, branchB: number) {
  // 1. Branch A 렌더링 → BMP
  exec(`branch switch ${branchA}`);
  renderFrame();
  exportBMP(pathA);

  // 2. Branch B 렌더링 → BMP
  exec(`branch switch ${branchB}`);
  renderFrame();
  exportBMP(pathB);

  // 3. Diff 데이터 가져오기
  const diff = JSON.parse(query(JSON.stringify({
    method: 'diffBranch',
    params: { a: branchA, b: branchB }
  })));

  // 4. BMP 이미지 + 변경 셀 오버레이 렌더링
  return { imageA: pathA, imageB: pathB, changes: diff.data };
}
```

### 8.9-8.11 TracePanel, MinimapPanel, ProcessPanel

패턴 동일:
1. `query()` 로 데이터 조회
2. React 컴포넌트로 시각화
3. `exec()` 로 조작
4. `panelRegistry.register()` 로 등록

---

## 9. 핵심 컴포넌트 상세

### 9.1 SkiaCanvas (범용 Skia 래퍼)

```typescript
// src/components/SkiaCanvas.tsx
// CanvasPanel과 MinimapPanel 모두 이 컴포넌트를 사용

interface SkiaCanvasProps {
  engine: CanvasOSEngineSpec;
  width: number;        // 렌더링 영역 너비 (픽셀)
  height: number;
  onCellTap?: (x: number, y: number) => void;
  showGrid?: boolean;
  showGateOverlay?: boolean;
}

// 렌더링 루프:
// useFrameCallback(() => {
//   engine.renderFrame();
//   const fb = JSON.parse(engine.getFrameBuffer());
//   // fb.ptr → Skia.Image.MakeImage
// });
```

### 9.2 PanelContainer (패널 프레임)

```typescript
// src/components/PanelContainer.tsx
interface PanelContainerProps {
  title: string;
  icon: string;
  children: ReactNode;
  collapsible?: boolean;
  actions?: Array<{ label: string; onPress: () => void }>;
}

// 모든 패널은 이 프레임 안에 렌더링됨:
// ┌─ [아이콘] 타이틀 ─── [접기] [최대화] ─┐
// │                                      │
// │  패널 내용 (children)                 │
// │                                      │
// └──────────────────────────────────────┘
```

### 9.3 WorkspaceLayout (패널 배치 관리)

```typescript
// src/components/WorkspaceLayout.tsx
interface WorkspaceLayoutProps {
  mainPanel: ReactNode;           // CanvasPanel (항상 표시)
  subPanel: ReactNode | null;     // 활성 하위 패널
  panelTabs: PanelDef[];          // 패널 목록 (탭 바 표시용)
}

// 세로 모드:
//   [Main 60%]
//   [Panel Tab Bar]
//   [Sub Panel 40%]
//
// 가로 모드 (미래 확장):
//   [Main 60% | Sub 40%]
//
// 분할 비율은 드래그로 조절 가능 (Reanimated gesture)
```

### 9.4 StatusBar

```typescript
// src/components/StatusBar.tsx
// 엔진 상태를 한 줄로 표시

// tick:1234 | hash:a3f2e1b0 | br:0 | gates:12/4096 | ABGR
// ↑ 1초마다 폴링 (useEngineStats)

function StatusBar() {
  const { tick, hash, branchId, openGates, visMode } = useEngineStats();
  return (
    <View style={styles.bar}>
      <Text>tick:{tick}</Text>
      <Text>hash:{hash.slice(0,8)}</Text>
      <Text>br:{branchId}</Text>
      <Text>gates:{openGates}/4096</Text>
      <Text>{['ABGR','Energy','Opcode','Lane','Activity'][visMode]}</Text>
    </View>
  );
}
```

### 9.6 KeyboardLayout (건반 모드)

v2의 `TerminalActivity.kt` 건반 레이아웃 재현:

```
행 1 (자음 — opcode 카테고리):
 ㄱ ㄲ ㄴ ㄷ ㄸ ㄹ ㅁ ㅂ ㅃ ㅅ ㅆ ㅇ ㅈ ㅉ ㅊ ㅋ ㅌ ㅍ ㅎ

행 2 (모음 — 수식어):
 ㅏ ㅑ ㅓ ㅕ ㅗ ㅛ ㅜ ㅠ ㅡ ㅢ ㅣ

행 3 (16진수):
 0 1 2 3 4 5 6 7 8 9 A B C D E F

행 4 (제어):
 ! (커밋)  tk (tick)  ← → ↑ ↓ (커서이동)  ? (셀조회)
```

터치 시: 조합된 문자열을 `exec()` 에 전달.
예: [ㅎ] → [ㅏ] → [4] → [8] → [!] = `exec("ㅎㅏ 48 !")`

---

## 10. 훅 상세

### 10.1 useEngine (싱글톤 + 라이프사이클)

```typescript
// src/hooks/useEngine.ts
import { createContext, useContext, useEffect, useRef } from 'react';
import { AppState } from 'react-native';
import NativeCanvasOSEngine from '../../modules/canvasos-engine/src';
import { useEngineStore } from '../stores/engineStore';

const EngineContext = createContext<typeof NativeCanvasOSEngine | null>(null);

export function EngineProvider({ children }) {
  const engineRef = useRef(NativeCanvasOSEngine);

  useEffect(() => {
    // 앱 시작 시 엔진 초기화
    const dataDir = FileSystem.documentDirectory + 'canvasos';
    engineRef.current.initialize(dataDir);

    // 백그라운드 전환 시 자동 저장
    const sub = AppState.addEventListener('change', (state) => {
      if (state === 'background') {
        engineRef.current.exec('cvp save session.cvp');
      }
    });

    return () => {
      sub.remove();
      engineRef.current.shutdown();
    };
  }, []);

  return (
    <EngineContext.Provider value={engineRef.current}>
      {children}
    </EngineContext.Provider>
  );
}

export function useEngine() {
  const engine = useContext(EngineContext);
  if (!engine) throw new Error('useEngine must be used within EngineProvider');
  return engine;
}
```

### 10.2 useCanvas (뷰포트 + 렌더링)

```typescript
// src/hooks/useCanvas.ts
export function useCanvas() {
  const engine = useEngine();
  const [viewport, setViewport] = useState({ x: 480, y: 480, w: 64, h: 64, cellPx: 4 });
  const [visMode, setVisMode] = useState(0);
  const [frameImage, setFrameImage] = useState(null);

  // 프레임 업데이트 (requestAnimationFrame 또는 Reanimated useFrameCallback)
  const updateFrame = useCallback(() => {
    engine.renderFrame();
    const fb = JSON.parse(engine.getFrameBuffer());
    // fb.ptr → Skia SkImage 생성
    // setFrameImage(skImage);
  }, [engine]);

  // 핀치줌 → 뷰포트 cellPx 변경
  const onPinch = useCallback((e) => {
    const newCellPx = Math.max(1, Math.min(16, viewport.cellPx * e.scale));
    engine.setViewport(viewport.x, viewport.y, viewport.w, viewport.h, newCellPx);
    setViewport(v => ({ ...v, cellPx: newCellPx }));
  }, [viewport, engine]);

  // 팬 → 뷰포트 x,y 이동
  const onPan = useCallback((e) => {
    const newX = viewport.x - Math.round(e.translationX / viewport.cellPx);
    const newY = viewport.y - Math.round(e.translationY / viewport.cellPx);
    engine.setViewport(newX, newY, viewport.w, viewport.h, viewport.cellPx);
    setViewport(v => ({ ...v, x: newX, y: newY }));
  }, [viewport, engine]);

  // 탭 → 셀 좌표 계산
  const onTap = useCallback((e) => {
    const cellX = viewport.x + Math.floor(e.x / viewport.cellPx);
    const cellY = viewport.y + Math.floor(e.y / viewport.cellPx);
    return { x: cellX, y: cellY };
  }, [viewport]);

  return { frameImage, viewport, visMode, setVisMode, onPinch, onPan, onTap, updateFrame };
}
```

### 10.3 useShell (명령 실행 + 히스토리)

```typescript
export function useShell() {
  const engine = useEngine();
  const [history, setHistory] = useState<Array<{input: string; output: string}>>([]);

  const execAndCapture = useCallback((command: string) => {
    const output = engine.exec(command);
    setHistory(prev => [...prev, { input: command, output }]);
    return output;
  }, [engine]);

  const clear = useCallback(() => setHistory([]), []);

  return { execAndCapture, history, clear };
}
```

### 10.7 useQuery (query() 래퍼)

```typescript
export function useQuery<T>(method: string, params?: object) {
  const engine = useEngine();

  const fetch = useCallback(() => {
    const req = JSON.stringify({ method, params });
    const res = JSON.parse(engine.query(req));
    if (!res.ok) throw new Error(res.error);
    return res.data as T;
  }, [engine, method, params]);

  return { fetch };
}
```

---

## 11. Zustand 스토어

### 11.1 engineStore

```typescript
// src/stores/engineStore.ts
import { create } from 'zustand';

interface EngineState {
  tick: number;
  hash: string;
  branchId: number;
  openGates: number;
  visMode: number;
  pcX: number;
  pcY: number;
  update: (status: Partial<EngineState>) => void;
}

export const useEngineStore = create<EngineState>((set) => ({
  tick: 0, hash: '', branchId: 0, openGates: 0,
  visMode: 0, pcX: 512, pcY: 512,
  update: (status) => set(status),
}));
```

### 11.2 uiStore

```typescript
// src/stores/uiStore.ts
import { create } from 'zustand';

interface UiState {
  activePanelId: string;         // 현재 활성 하위 패널
  canvasSplitRatio: number;      // 캔버스/패널 분할 비율 (0.4-0.8)
  theme: 'light' | 'dark' | 'system';
  setActivePanel: (id: string) => void;
  setSplitRatio: (ratio: number) => void;
  setTheme: (theme: 'light' | 'dark' | 'system') => void;
}

export const useUiStore = create<UiState>((set) => ({
  activePanelId: 'terminal',     // 기본: 터미널 패널
  canvasSplitRatio: 0.6,
  theme: 'system',
  setActivePanel: (id) => set({ activePanelId: id }),
  setSplitRatio: (ratio) => set({ canvasSplitRatio: ratio }),
  setTheme: (theme) => set({ theme }),
}));
```

### 11.3 projectStore

```typescript
// src/stores/projectStore.ts
// v3.2의 project-context.tsx에서 canvasData 제거, CVP 경로만 저장

interface Project {
  id: string;
  name: string;
  description: string;
  cvpPath: string;              // CVP 파일 경로 (canvasData 대신)
  createdAt: number;
  updatedAt: number;
  status: 'idle' | 'running' | 'error';
}
```

---

## 12. package.json 변경사항

### 추가 의존성
```json
{
  "@shopify/react-native-skia": "^1.x",
  "zustand": "^4.x",
  "expo-file-system": "~18.x",
  "expo-dev-client": "~5.x"
}
```

### 제거 의존성
```json
{
  "@trpc/client": "삭제",
  "@trpc/react-query": "삭제",
  "@trpc/server": "삭제",
  "drizzle-orm": "삭제",
  "drizzle-kit": "삭제",
  "mysql2": "삭제",
  "jose": "삭제",
  "express": "삭제",
  "tsx": "삭제",
  "esbuild": "삭제",
  "superjson": "삭제"
}
```

### React Query 유지 (캐싱용)
```json
{
  "@tanstack/react-query": "^5.x"
}
```

---

## 13. 구현 Phase + 검증 체크리스트

### Phase 1: 프로젝트 셋업 + 스켈레톤 (Mock 엔진으로)

**작업**:
- [ ] `npx create-expo-app canvasos-hybrid --template tabs`
- [ ] v3.2에서 유지 파일 복사 (theme, components, constants)
- [ ] v3.2에서 삭제 파일 제거 (server, drizzle, auth, trpc)
- [ ] `NativeCanvasOSEngine.ts` 계약 파일 배치
- [ ] `MockCanvasOSEngine.ts` 작성
- [ ] `useEngine.ts` + `EngineProvider` 작성 (Mock 연결)
- [ ] 3탭 레이아웃 (`_layout.tsx`) 작성
- [ ] `StatusBar` 컴포넌트 (Mock 데이터)
- [ ] `panelRegistry.ts` 스켈레톤
- [ ] Zustand 스토어 3개 (engineStore, uiStore, projectStore)

**검증** (Mock 기반, 네이티브 빌드 불필요):
```
1. npx expo start --web → 3탭 네비게이션 동작
2. 홈 화면에 tick/hash 표시 (Mock 값)
3. 워크스페이스에서 패널 탭 전환 동작
4. 설정 화면 렌더링
```

### Phase 2: 캔버스 + 터미널 패널

**작업**:
- [ ] `@shopify/react-native-skia` 설치
- [ ] `SkiaCanvas.tsx` 구현 (Mock: 그라디언트 테스트 이미지)
- [ ] `CanvasPanel.tsx` + 핀치줌/팬 제스처
- [ ] `VisModePicker.tsx`
- [ ] `TerminalPanel.tsx` + `useShell.ts`
- [ ] `KeyboardLayout.tsx` (건반 모드)
- [ ] `WorkspaceLayout.tsx` (캔버스+패널 분할)
- [ ] `useCanvas.ts` 훅

**검증** (Mock):
```
1. 워크스페이스에서 테스트 이미지 렌더링
2. 핀치줌/팬 제스처 동작 (뷰포트 상태 변경)
3. 터미널에서 명령 입력 → Mock 응답 표시
4. 건반 모드 전환 → 자음+모음 터치 → 조합 문자열 생성
5. 패널 탭 전환 (터미널 ↔ 캔버스) 동작
```

### Phase 3: 나머지 패널들

**작업**:
- [ ] `CellInspectorPanel.tsx` + `useQuery('getCell')`
- [ ] `GatePanel.tsx` + `useGate.ts`
- [ ] `BranchPanel.tsx` + `useBranch.ts`
- [ ] `TimelinePanel.tsx` + `useTimeline.ts`
- [ ] `DiffPanel.tsx` (BMP 이미지 비교 뷰)
- [ ] `TracePanel.tsx`
- [ ] `MinimapPanel.tsx`
- [ ] `ProcessPanel.tsx`
- [ ] 홈 화면 강화 (미니맵, Quick Stats)
- [ ] 설정 화면 완성

**검증** (Mock):
```
1. 모든 11개 패널이 워크스페이스에서 탭으로 접근 가능
2. 각 패널이 Mock 데이터를 올바르게 렌더링
3. DiffPanel이 두 이미지를 side-by-side로 표시
4. BranchPanel이 트리 구조를 표시
5. TimelinePanel이 가로 스크롤 타임라인 표시
```

---

## 14. 통합 테스트 (Spec 1 + Spec 2 합류 후)

> **이 섹션은 Spec 1과 Spec 2 구현이 모두 완료된 후 실행**

### 14.1 연결 준비

```
1. Spec 1의 modules/canvasos-engine/ 을 프로젝트에 배치
2. native/build/ 에 v2 C 소스 복사
3. MockCanvasOSEngine 대신 실제 Turbo Module로 전환:
   useEngine.ts에서 Mock import 제거, 실제 NativeCanvasOSEngine 사용
4. npx expo run:android → 빌드 성공 확인
```

### 14.2 연결 동작 테스트 (순서대로)

```
테스트 1: 엔진 부팅
  - 앱 시작 → EngineProvider.initialize() 호출
  - 홈 화면에 tick=1, hash=실제값 표시 확인
  - StatusBar에 실시간 데이터 표시 확인

테스트 2: Shell 명령 실행
  - 워크스페이스 → 터미널 패널
  - "info" 입력 → 엔진 정보 출력 확인
  - "hash" 입력 → 해시값 출력 확인
  - "gate open 0" → "OK" 응답 확인
  - "gate list" → 게이트 0 열림 확인

테스트 3: 캔버스 렌더링
  - 워크스페이스 → 캔버스 상단 영역
  - Skia에 실제 C 픽셀 버퍼 렌더링 확인 (빈 캔버스 = 검은색)
  - 터미널에서 "B=01 R=48 !" → 캔버스에 픽셀 변화 확인
  - VisMode 전환 (5개) → 각 모드 색상 차이 확인
  - 핀치줌 → 뷰포트 확대 확인
  - 팬 → 뷰포트 이동 확인

테스트 4: 셀 인스펙션
  - 캔버스 터치 → CellInspectorPanel에 A/B/G/R 표시
  - (512,512) 주변에 데이터 있는 셀 확인

테스트 5: 한글코드 + 건반
  - 터미널에서 "ㅎㅏ 48" 입력 → 실행 확인
  - 건반 모드 전환 → [ㅎ][ㅏ][4][8][!] 순서 터치 → 같은 결과

테스트 6: 게이트 제어
  - GatePanel에서 게이트 0 토글 → 캔버스 오버레이 변경 확인
  - "gate list" 결과와 GatePanel 표시 일치 확인

테스트 7: 브랜치 + 타임워프
  - "branch create test" → BranchPanel 트리에 새 브랜치 표시
  - "tick 100" → StatusBar tick 변경 확인
  - "snapshot cp1" → TimelinePanel에 스냅샷 마커 표시
  - "rewind 50" → tick이 50으로 변경 확인
  - "branch switch 0" → 원래 브랜치로 복귀 확인

테스트 8: Diff + BMP
  - DiffPanel에서 Branch A, B 선택 → "비교 실행"
  - 두 BMP 이미지가 side-by-side로 표시
  - 변경된 셀 위치에 빨간 점 오버레이

테스트 9: CVP 저장/로드
  - 설정 → "저장" → exec("cvp save test.cvp")
  - 앱 종료 → 재시작 → 설정 → "불러오기"
  - tick/hash가 저장 시점과 동일한지 확인

테스트 10: 백그라운드 전환
  - 홈 버튼으로 앱 백그라운드
  - AppState 이벤트 → "cvp save session.cvp" 자동 실행 확인
  - 앱 복귀 → 상태 유지 확인

테스트 11: 성능
  - 캔버스 FPS 측정 → ≥ 30 FPS
  - exec() 레이턴시 측정 → < 5ms
  - 앱 메모리 → < 100MB (엔진 ~8.3MB + UI ~30MB + 시스템)
```

### 14.3 회귀 테스트

```
- v2 CLI에서 생성한 CVP 파일을 하이브리드 앱에서 로드 가능한지 확인
- 하이브리드 앱에서 저장한 CVP를 v2 CLI에서 로드 가능한지 확인
- "det on" 상태에서 동일한 명령 시퀀스 → 동일한 해시 값
```

---

## 15. v3.2 디자인 토큰 (그대로 사용)

```typescript
// constants/theme.ts (v3.2에서 복사)
Light: {
  primary: '#0a7ea4',
  background: '#ffffff',
  surface: '#f5f5f5',
  foreground: '#11181C',
  muted: '#687076',
  border: '#E5E7EB',
  success: '#22C55E',
  warning: '#F59E0B',
  error: '#EF4444',
}
Dark: {
  primary: '#0a7ea4',
  background: '#151718',
  surface: '#1e2022',
  foreground: '#ECEDEE',
  muted: '#9BA1A6',
  border: '#334155',
  success: '#4ADE80',
  warning: '#FBBF24',
  error: '#F87171',
}
```

패널별 추가 색상:
```typescript
// 엔진 시각화 전용 색상 (캔버스 안에서만 사용)
const engineColors = {
  gateOpen: '#4ADE80',       // 열린 게이트 오버레이
  gateClosed: '#334155',     // 닫힌 게이트 오버레이
  cursor: '#FBBF24',         // 현재 커서 위치
  diffHighlight: '#EF4444',  // Diff 변경 셀
  whEvent: '#ECEDEE',        // WH 이벤트 점
  bhCompressed: '#687076',   // BH 압축 구간
  branchCurrent: '#0a7ea4',  // 현재 브랜치
  branchOther: '#687076',    // 다른 브랜치
};
```
