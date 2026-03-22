import { useCallback } from 'react';
import { useEngine } from './useEngine';
import type { QueryResult } from '../types/engine';

export function useEngineQuery() {
  const engine = useEngine();

  const fetch = useCallback(<T = unknown>(method: string, params?: object): QueryResult<T> => {
    try {
      const req = JSON.stringify({ method, params });
      const res: QueryResult<T> = JSON.parse(engine.query(req));
      return res;
    } catch (e) {
      return { ok: false, error: String(e) };
    }
  }, [engine]);

  return { fetch };
}
