import { ComponentType } from 'react';

export interface PanelDef {
  id: string;
  label: string;
  icon: string;
  component: ComponentType;
  phase: number;
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
