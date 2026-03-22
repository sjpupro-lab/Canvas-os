import { create } from 'zustand';

interface UiState {
  activePanelId: string;
  canvasSplitRatio: number;
  theme: 'light' | 'dark' | 'system';
  setActivePanel: (id: string) => void;
  setSplitRatio: (ratio: number) => void;
  setTheme: (theme: 'light' | 'dark' | 'system') => void;
}

export const useUiStore = create<UiState>((set) => ({
  activePanelId: 'terminal',
  canvasSplitRatio: 0.6,
  theme: 'system',
  setActivePanel: (id) => set({ activePanelId: id }),
  setSplitRatio: (ratio) => set({ canvasSplitRatio: Math.max(0.3, Math.min(0.8, ratio)) }),
  setTheme: (theme) => set({ theme }),
}));
