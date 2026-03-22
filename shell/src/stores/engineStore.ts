import { create } from 'zustand';
import type { EngineStatus } from '../types/engine';

interface EngineState extends EngineStatus {
  initialized: boolean;
  update: (status: Partial<EngineStatus>) => void;
  setInitialized: (v: boolean) => void;
}

export const useEngineStore = create<EngineState>((set) => ({
  tick: 0,
  hash: '',
  branchId: 0,
  openGates: 0,
  visMode: 0,
  pcX: 512,
  pcY: 512,
  initialized: false,
  update: (status) => set(status),
  setInitialized: (v) => set({ initialized: v }),
}));
