import { useState, useCallback } from 'react';
import { useEngine } from './useEngine';

interface ShellEntry {
  input: string;
  output: string;
  timestamp: number;
}

export function useShell() {
  const engine = useEngine();
  const [history, setHistory] = useState<ShellEntry[]>([]);

  const execAndCapture = useCallback((command: string): string => {
    const output = engine.exec(command);
    setHistory((prev) => [
      ...prev,
      { input: command, output, timestamp: Date.now() },
    ]);
    return output;
  }, [engine]);

  const clear = useCallback(() => setHistory([]), []);

  return { execAndCapture, history, clear };
}
