// C 구조체 매칭 타입 — Spec 1의 JSON 반환값과 1:1 대응

export interface EngineStatus {
  tick: number;
  hash: string;
  branchId: number;
  openGates: number;
  visMode: number;
  pcX: number;
  pcY: number;
}

export interface FrameBufferInfo {
  ptr: number;
  width: number;
  height: number;
  byteLength: number;
}

export interface CellData {
  a: number;
  b: number;
  g: number;
  r: number;
}

export interface BranchInfo {
  id: number;
  parentId: number;
  planeMask: number;
  xMin: number; xMax: number;
  yMin: number; yMax: number;
  laneId: number;
  gatePolicy: number;
}

export interface WhRecord {
  tick: number;
  opcode: number;
  targetAddr: number;
  targetKind: number;
  flags: number;
  param0: number;
}

export interface DiffCell {
  x: number; y: number;
  beforeB: number; beforeR: number;
  afterB: number; afterR: number;
}

export interface GateInfo {
  id: number;
  isOpen: boolean;
  tileX: number;
  tileY: number;
}

export interface TraceEvent {
  tick: number;
  pcX: number; pcY: number;
  opcode: number;
  regR: number;
}

export interface ProcessInfo {
  pid: number;
  state: string;
  energy: number;
  pcX: number; pcY: number;
}

export interface SnapshotInfo {
  id: number;
  tick: number;
  branchId: number;
  hash: number;
}

export interface QueryResult<T = unknown> {
  ok: boolean;
  data?: T;
  error?: string;
}

// 시각화 모드 상수
export const VIS_MODES = ['ABGR', 'Energy', 'Opcode', 'Lane', 'Activity'] as const;
export type VisMode = typeof VIS_MODES[number];

// 캔버스 상수
export const CANVAS_W = 1024;
export const CANVAS_H = 1024;
export const ORIGIN_X = 512;
export const ORIGIN_Y = 512;

// Opcode 이름 매핑 (디버그 표시용)
export const OPCODE_NAMES: Record<number, string> = {
  0x00: 'NOP', 0x01: 'SET', 0x02: 'COPY', 0x03: 'LOAD', 0x04: 'STORE',
  0x05: 'ADD', 0x06: 'SUB', 0x10: 'JMP', 0x11: 'JZ', 0x12: 'JNZ',
  0x13: 'CALL', 0x14: 'RET', 0x20: 'PRINT', 0x21: 'SEND', 0x22: 'RECV',
  0x30: 'SPAWN', 0x31: 'EXIT', 0x3F: 'HALT',
  0x40: 'GATE_ON', 0x41: 'GATE_OFF',
  0x50: 'DRAW', 0x51: 'LINE', 0x52: 'RECT',
  0x60: 'SYSCALL', 0x70: 'BREAKPOINT',
};
