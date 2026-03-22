import type { TurboModule } from "react-native";
import { TurboModuleRegistry } from "react-native";

export interface Spec extends TurboModule {
  initialize(dataDir: string): boolean;
  shutdown(): void;
  tick(n: number): number;
  getHash(): string;
  getStatus(): string;
  setViewport(x: number, y: number, w: number, h: number, cellPx: number): void;
  setVisMode(mode: number): void;
  renderFrame(): void;
  getFrameBuffer(): string;
  getCell(x: number, y: number): string;
  exec(command: string): string;
  exportBMP(filePath: string): boolean;
  query(request: string): string;
}

export default TurboModuleRegistry.get<Spec>("CanvasOSEngine");
