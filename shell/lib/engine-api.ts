import { getCanvasOSEngineModule, hasCanvasOSEngineModule } from "canvasos-engine/runtime";

/**
 * Canvas-OS 엔진 연결 API
 * TurboModule이 있으면 우선 사용하고, 없으면 기존 REST 경로로 폴백한다.
 */

/**
 * 엔진 실행 결과
 */
export interface EngineExecutionResult {
  success: boolean;
  tick: number;
  canvasData: Uint8Array;
  output: string;
  error?: string;
  metrics?: {
    executionTime: number;
    cellsModified: number;
    instructionsExecuted: number;
  };
}

/**
 * 엔진 상태
 */
export interface EngineState {
  tick: number;
  pc: { x: number; y: number };
  canvasHash: string;
  status: "idle" | "running" | "paused" | "error";
  lastError?: string;
}

/**
 * 캔버스 스냅샷 (CVP 포맷)
 */
export interface CanvasSnapshot {
  id: string;
  name: string;
  timestamp: number;
  tick: number;
  canvasData: Uint8Array;
  metadata: Record<string, any>;
}

/**
 * 브랜치 정보
 */
export interface BranchInfo {
  id: string;
  name: string;
  parentId?: string;
  createdAt: number;
  snapshots: CanvasSnapshot[];
}

/**
 * v2 C 엔진 API 클래스
 * 이 클래스는 네이티브 엔진과의 통신을 담당합니다.
 * 
 * 사용 예:
 * ```typescript
 * const engine = new CanvasOSEngine();
 * const result = await engine.executeCode("한글코드 또는 픽셀코드");
 * ```
 */
export class CanvasOSEngine {
  private baseUrl: string = "http://localhost:3000/api/engine";
  private nativeModule = getCanvasOSEngineModule();
  private currentState: EngineState = {
    tick: 0,
    pc: { x: 512, y: 512 },
    canvasHash: "",
    status: "idle",
  };

  /**
   * 엔진 초기화
   */
  async initialize(): Promise<void> {
    if (this.nativeModule && this.nativeModule.initialize("/data/user/0/canvasos")) {
      const state = this.parseNativeStatus();
      this.currentState = state;
      return;
    }

    try {
      const response = await fetch(`${this.baseUrl}/init`, {
        method: "POST",
      });
      if (!response.ok) throw new Error("Failed to initialize engine");
      const data = await response.json();
      this.currentState = data.state;
    } catch (error) {
      console.error("Engine initialization failed:", error);
      throw error;
    }
  }

  /**
   * 한글코드 또는 픽셀코드 실행
   * @param code 한글코드 또는 픽셀코드 문자열
   * @param codeType 코드 타입 ("hangul" | "pixel")
   */
  async executeCode(code: string, codeType: "hangul" | "pixel" = "hangul"): Promise<EngineExecutionResult> {
    try {
      const response = await fetch(`${this.baseUrl}/execute`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          code,
          codeType,
          state: this.currentState,
        }),
      });

      if (!response.ok) {
        throw new Error(`Engine execution failed: ${response.statusText}`);
      }

      const result: EngineExecutionResult = await response.json();
      
      // 상태 업데이트
      if (result.success) {
        this.currentState.tick = result.tick;
        this.currentState.status = "idle";
      } else {
        this.currentState.status = "error";
        this.currentState.lastError = result.error;
      }

      return result;
    } catch (error) {
      console.error("Code execution failed:", error);
      throw error;
    }
  }

  /**
   * 현재 상태 조회
   */
  async getState(): Promise<EngineState> {
    if (this.nativeModule) {
      this.currentState = this.parseNativeStatus();
      return this.currentState;
    }

    try {
      const response = await fetch(`${this.baseUrl}/state`);
      if (!response.ok) throw new Error("Failed to get state");
      const data = await response.json();
      this.currentState = data.state;
      return this.currentState;
    } catch (error) {
      console.error("Failed to get state:", error);
      throw error;
    }
  }

  /**
   * 캔버스 데이터 조회
   */
  async getCanvasData(): Promise<Uint8Array> {
    try {
      const response = await fetch(`${this.baseUrl}/canvas`);
      if (!response.ok) throw new Error("Failed to get canvas");
      const blob = await response.blob();
      const buffer = await blob.arrayBuffer();
      return new Uint8Array(buffer);
    } catch (error) {
      console.error("Failed to get canvas:", error);
      throw error;
    }
  }

  /**
   * 캔버스 데이터 설정
   */
  async setCanvasData(canvasData: Uint8Array): Promise<void> {
    try {
      const response = await fetch(`${this.baseUrl}/canvas`, {
        method: "POST",
        headers: { "Content-Type": "application/octet-stream" },
        body: canvasData as any,
      });
      if (!response.ok) throw new Error("Failed to set canvas");
    } catch (error) {
      console.error("Failed to set canvas:", error);
      throw error;
    }
  }

  /**
   * 실행 일시 중지
   */
  async pause(): Promise<void> {
    try {
      const response = await fetch(`${this.baseUrl}/pause`, {
        method: "POST",
      });
      if (!response.ok) throw new Error("Failed to pause");
      this.currentState.status = "paused";
    } catch (error) {
      console.error("Failed to pause:", error);
      throw error;
    }
  }

  /**
   * 실행 재개
   */
  async resume(): Promise<void> {
    try {
      const response = await fetch(`${this.baseUrl}/resume`, {
        method: "POST",
      });
      if (!response.ok) throw new Error("Failed to resume");
      this.currentState.status = "running";
    } catch (error) {
      console.error("Failed to resume:", error);
      throw error;
    }
  }

  /**
   * 특정 tick으로 되돌리기 (Rewind)
   */
  async rewind(targetTick: number): Promise<EngineExecutionResult> {
    try {
      const response = await fetch(`${this.baseUrl}/rewind`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ targetTick }),
      });

      if (!response.ok) throw new Error("Failed to rewind");
      const result: EngineExecutionResult = await response.json();
      this.currentState.tick = result.tick;
      return result;
    } catch (error) {
      console.error("Failed to rewind:", error);
      throw error;
    }
  }

  /**
   * 스냅샷 생성
   */
  async createSnapshot(name: string, metadata?: Record<string, any>): Promise<CanvasSnapshot> {
    try {
      const response = await fetch(`${this.baseUrl}/snapshot`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          name,
          tick: this.currentState.tick,
          metadata,
        }),
      });

      if (!response.ok) throw new Error("Failed to create snapshot");
      return await response.json();
    } catch (error) {
      console.error("Failed to create snapshot:", error);
      throw error;
    }
  }

  /**
   * 스냅샷 로드
   */
  async loadSnapshot(snapshotId: string): Promise<EngineExecutionResult> {
    try {
      const response = await fetch(`${this.baseUrl}/snapshot/${snapshotId}`, {
        method: "GET",
      });

      if (!response.ok) throw new Error("Failed to load snapshot");
      const result: EngineExecutionResult = await response.json();
      this.currentState.tick = result.tick;
      return result;
    } catch (error) {
      console.error("Failed to load snapshot:", error);
      throw error;
    }
  }

  /**
   * 브랜치 생성
   */
  async createBranch(name: string, parentId?: string): Promise<BranchInfo> {
    try {
      const response = await fetch(`${this.baseUrl}/branch`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          name,
          parentId,
          tick: this.currentState.tick,
        }),
      });

      if (!response.ok) throw new Error("Failed to create branch");
      return await response.json();
    } catch (error) {
      console.error("Failed to create branch:", error);
      throw error;
    }
  }

  /**
   * 브랜치 목록 조회
   */
  async listBranches(): Promise<BranchInfo[]> {
    try {
      const response = await fetch(`${this.baseUrl}/branches`);
      if (!response.ok) throw new Error("Failed to list branches");
      return await response.json();
    } catch (error) {
      console.error("Failed to list branches:", error);
      throw error;
    }
  }

  /**
   * 브랜치 전환
   */
  async switchBranch(branchId: string): Promise<EngineExecutionResult> {
    try {
      const response = await fetch(`${this.baseUrl}/branch/${branchId}`, {
        method: "POST",
      });

      if (!response.ok) throw new Error("Failed to switch branch");
      const result: EngineExecutionResult = await response.json();
      this.currentState.tick = result.tick;
      return result;
    } catch (error) {
      console.error("Failed to switch branch:", error);
      throw error;
    }
  }

  /**
   * 두 상태 비교 (Diff)
   */
  async diff(tick1: number, tick2: number): Promise<{ modified: number; changes: any[] }> {
    try {
      const response = await fetch(`${this.baseUrl}/diff`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ tick1, tick2 }),
      });

      if (!response.ok) throw new Error("Failed to compute diff");
      return await response.json();
    } catch (error) {
      console.error("Failed to compute diff:", error);
      throw error;
    }
  }

  /**
   * 엔진 리셋
   */
  async reset(): Promise<void> {
    if (this.nativeModule) {
      this.nativeModule.shutdown();
      this.nativeModule.initialize("/data/user/0/canvasos");
      this.currentState = this.parseNativeStatus();
      return;
    }

    try {
      const response = await fetch(`${this.baseUrl}/reset`, {
        method: "POST",
      });
      if (!response.ok) throw new Error("Failed to reset");
      this.currentState = {
        tick: 0,
        pc: { x: 512, y: 512 },
        canvasHash: "",
        status: "idle",
      };
    } catch (error) {
      console.error("Failed to reset:", error);
      throw error;
    }
  }

  /**
   * 현재 상태 반환
   */
  getLocalState(): EngineState {
    return this.currentState;
  }

  /**
   * 기본 URL 설정 (테스트 또는 다른 서버 사용 시)
   */
  setBaseUrl(url: string): void {
    this.baseUrl = url;
  }

  hasNativeModule(): boolean {
    return hasCanvasOSEngineModule();
  }

  private parseNativeStatus(): EngineState {
    if (!this.nativeModule) {
      return this.currentState;
    }

    const raw = this.nativeModule.getStatus();
    const status = JSON.parse(raw) as {
      tick: number;
      hash: string;
      pcX: number;
      pcY: number;
    };

    return {
      tick: status.tick,
      pc: { x: status.pcX, y: status.pcY },
      canvasHash: status.hash,
      status: "idle",
    };
  }
}

/**
 * 싱글톤 엔진 인스턴스
 */
let engineInstance: CanvasOSEngine | null = null;

export function getEngine(): CanvasOSEngine {
  if (!engineInstance) {
    engineInstance = new CanvasOSEngine();
  }
  return engineInstance;
}

export function resetEngine(): void {
  engineInstance = null;
}
