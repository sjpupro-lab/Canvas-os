import { create } from 'zustand';

export interface Project {
  id: string;
  name: string;
  description: string;
  cvpPath: string;
  createdAt: number;
  updatedAt: number;
  status: 'idle' | 'running' | 'error';
}

interface ProjectState {
  projects: Project[];
  currentProjectId: string | null;
  addProject: (p: Project) => void;
  removeProject: (id: string) => void;
  setCurrentProject: (id: string | null) => void;
  updateProject: (id: string, updates: Partial<Project>) => void;
  setProjects: (projects: Project[]) => void;
}

export const useProjectStore = create<ProjectState>((set) => ({
  projects: [],
  currentProjectId: null,
  addProject: (p) => set((s) => ({ projects: [...s.projects, p] })),
  removeProject: (id) => set((s) => ({
    projects: s.projects.filter((p) => p.id !== id),
    currentProjectId: s.currentProjectId === id ? null : s.currentProjectId,
  })),
  setCurrentProject: (id) => set({ currentProjectId: id }),
  updateProject: (id, updates) => set((s) => ({
    projects: s.projects.map((p) => p.id === id ? { ...p, ...updates, updatedAt: Date.now() } : p),
  })),
  setProjects: (projects) => set({ projects }),
}));
