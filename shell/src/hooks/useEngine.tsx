import React, { createContext, useContext, useEffect, useRef } from 'react';
import { AppState, Platform } from 'react-native';
import { MockEngine } from '../mock/MockCanvasOSEngine';
import { useEngineStore } from '../stores/engineStore';
import type { EngineStatus } from '../types/engine';

// Spec 1의 runtime.ts를 통해 TurboModule 존재 여부 확인
// Android 네이티브 빌드에서는 실제 C 엔진 사용, 그 외에는 Mock
function getEngineModule() {
  if (Platform.OS === 'android') {
    try {
      const { getCanvasOSEngineModule } = require('canvasos-engine/runtime');
      const native = getCanvasOSEngineModule();
      if (native) return native;
    } catch {}
  }
  return MockEngine;
}

type EngineType = typeof MockEngine;

const EngineContext = createContext<EngineType | null>(null);

export function EngineProvider({ children }: { children: React.ReactNode }) {
  const engineRef = useRef(getEngineModule());
  const store = useEngineStore();

  useEffect(() => {
    const engine = engineRef.current;
    const ok = engine.initialize('canvasos_data');
    store.setInitialized(ok);

    if (ok) {
      // 초기 상태 동기화
      const status: EngineStatus = JSON.parse(engine.getStatus());
      store.update(status);
    }

    // 1초마다 엔진 상태 폴링
    const interval = setInterval(() => {
      if (!store.initialized) return;
      try {
        const status: EngineStatus = JSON.parse(engine.getStatus());
        store.update(status);
      } catch {}
    }, 1000);

    // 백그라운드 전환 시 자동 저장
    const sub = AppState.addEventListener('change', (state) => {
      if (state === 'background') {
        engine.exec('cvp save session.cvp');
      }
    });

    return () => {
      clearInterval(interval);
      sub.remove();
      engine.shutdown();
      store.setInitialized(false);
    };
  }, []);

  return (
    <EngineContext.Provider value={engineRef.current}>
      {children}
    </EngineContext.Provider>
  );
}

export function useEngine(): EngineType {
  const engine = useContext(EngineContext);
  if (!engine) throw new Error('useEngine must be used within EngineProvider');
  return engine;
}
