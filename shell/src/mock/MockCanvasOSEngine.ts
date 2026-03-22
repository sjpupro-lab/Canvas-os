// Mock Turbo Module — Spec 1 완성 전 UI 독립 개발용
// NativeCanvasOSEngine.ts와 동일한 인터페이스

let mockTick = 0;
let mockVisMode = 0;
let mockBranchId = 0;
let mockOpenGates = 0;
const mockHistory: string[] = [];

const mockBranches = [
  { id: 0, parentId: -1, planeMask: 0xFF, xMin: 0, xMax: 1023, yMin: 0, yMax: 1023, laneId: 0, gatePolicy: 0 },
];

const mockSnapshots: Array<{ id: number; tick: number; branchId: number; hash: number }> = [];

export const MockEngine = {
  initialize(_dataDir: string): boolean {
    mockTick = 1;
    return true;
  },

  shutdown(): void {
    mockTick = 0;
  },

  tick(n: number): number {
    mockTick += n;
    return mockTick;
  },

  getHash(): string {
    return ((mockTick * 2654435761) >>> 0).toString(16).padStart(8, '0');
  },

  getStatus(): string {
    return JSON.stringify({
      tick: mockTick,
      hash: this.getHash(),
      branchId: mockBranchId,
      openGates: mockOpenGates,
      visMode: mockVisMode,
      pcX: 512,
      pcY: 512,
    });
  },

  setViewport(_x: number, _y: number, _w: number, _h: number, _cellPx: number): void {},

  setVisMode(mode: number): void {
    mockVisMode = mode;
  },

  renderFrame(): void {},

  getFrameBuffer(): string {
    return JSON.stringify({ ptr: 0, width: 256, height: 256, byteLength: 262144 });
  },

  getCell(x: number, y: number): string {
    // 중앙 근처는 데이터 있는 것처럼 시뮬레이션
    const hasData = Math.abs(x - 512) < 16 && Math.abs(y - 512) < 16;
    return JSON.stringify({
      a: hasData ? 0xFF000000 : 0,
      b: hasData ? 0x01 : 0,
      g: hasData ? 128 : 0,
      r: hasData ? 0x48 : 0,
    });
  },

  exec(command: string): string {
    mockHistory.push(command);
    const parts = command.trim().split(/\s+/);
    const cmd = parts[0];

    switch (cmd) {
      case 'info':
        return `CanvasOS v2.1 (Mock)\ntick: ${mockTick}\nhash: ${this.getHash()}\ngates: ${mockOpenGates}/4096`;
      case 'hash':
        return `Hash: ${this.getHash()}`;
      case 'tick': {
        const n = parseInt(parts[1] || '1');
        mockTick += n;
        return `OK: tick=${mockTick}`;
      }
      case 'gate':
        if (parts[1] === 'open') { mockOpenGates++; return 'OK'; }
        if (parts[1] === 'close') { mockOpenGates = Math.max(0, mockOpenGates - 1); return 'OK'; }
        if (parts[1] === 'list') return `Open gates: ${mockOpenGates}`;
        return 'Usage: gate open|close|list [id]';
      case 'branch':
        if (parts[1] === 'create') {
          const newId = mockBranches.length;
          mockBranches.push({ id: newId, parentId: mockBranchId, planeMask: 0xFF, xMin: 0, xMax: 1023, yMin: 0, yMax: 1023, laneId: newId, gatePolicy: 0 });
          return `Branch ${newId} created`;
        }
        if (parts[1] === 'switch') { mockBranchId = parseInt(parts[2] || '0'); return `Switched to branch ${mockBranchId}`; }
        if (parts[1] === 'list') return mockBranches.map(b => `br:${b.id} parent:${b.parentId}`).join('\n');
        return 'Usage: branch create|switch|list';
      case 'snapshot': {
        const snap = { id: mockSnapshots.length, tick: mockTick, branchId: mockBranchId, hash: 0 };
        mockSnapshots.push(snap);
        return `Snapshot ${snap.id} at tick ${mockTick}`;
      }
      case 'rewind': {
        const target = parseInt(parts[1] || '0');
        mockTick = target;
        return `Rewound to tick ${target}`;
      }
      case 'cvp':
        return `[mock] CVP ${parts[1] || 'save'}: OK`;
      case 'det':
        return `Determinism mode: ${parts[1] || 'off'}`;
      case 'trace':
        return `Trace: ${parts[1] || 'off'}`;
      case 'reset':
        mockTick = 1; mockBranchId = 0; mockOpenGates = 0;
        return 'Engine reset';
      default:
        // 한글코드/PixelCode도 mock 처리
        if (command.includes('=') || /[ㄱ-ㅎ]/.test(command)) {
          mockTick++;
          return `[mock] Code executed, tick=${mockTick}`;
        }
        return `Unknown command: ${cmd}`;
    }
  },

  exportBMP(_filePath: string): boolean {
    return true;
  },

  query(request: string): string {
    try {
      const { method, params } = JSON.parse(request);
      switch (method) {
        case 'getStatus':
          return JSON.stringify({ ok: true, data: JSON.parse(this.getStatus()) });
        case 'branchList':
          return JSON.stringify({ ok: true, data: mockBranches });
        case 'snapshotList':
          return JSON.stringify({ ok: true, data: mockSnapshots });
        case 'gateList':
          return JSON.stringify({ ok: true, data: [] });
        case 'whLog':
          return JSON.stringify({ ok: true, data: [] });
        case 'bhStatus':
          return JSON.stringify({ ok: true, data: { active: false } });
        case 'traceLog':
          return JSON.stringify({ ok: true, data: [] });
        case 'processList':
          return JSON.stringify({ ok: true, data: [{ pid: 0, state: 'running', energy: 255, pcX: 512, pcY: 512 }] });
        case 'diffBranch':
        case 'diffTick':
          return JSON.stringify({ ok: true, data: [] });
        case 'sourceMap':
          return JSON.stringify({ ok: true, data: [] });
        case 'cellRange': {
          const { x0, y0, x1, y1 } = params || { x0: 510, y0: 510, x1: 514, y1: 514 };
          const cells = [];
          for (let y = y0; y <= y1; y++)
            for (let x = x0; x <= x1; x++)
              cells.push({ x, y, a: 0, b: 0, g: 0, r: 0 });
          return JSON.stringify({ ok: true, data: cells });
        }
        default:
          return JSON.stringify({ ok: false, error: `unknown method: ${method}` });
      }
    } catch {
      return JSON.stringify({ ok: false, error: 'invalid JSON' });
    }
  },
};
