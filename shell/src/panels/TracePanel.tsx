import React, { useState, useCallback } from 'react';
import { View, Text, ScrollView, TouchableOpacity, StyleSheet } from 'react-native';
import { useEngine } from '../hooks/useEngine';
import { useEngineQuery } from '../hooks/useQuery';
import { panelRegistry } from './registry';
import { OPCODE_NAMES } from '../types/engine';
import type { TraceEvent } from '../types/engine';

function TracePanelComponent() {
  const engine = useEngine();
  const { fetch } = useEngineQuery();
  const [tracing, setTracing] = useState(false);
  const [events, setEvents] = useState<TraceEvent[]>([]);

  const toggleTrace = () => {
    const cmd = tracing ? 'trace off' : 'trace on';
    engine.exec(cmd);
    setTracing(!tracing);
  };

  const refresh = useCallback(() => {
    const res = fetch<TraceEvent[]>('traceLog');
    if (res.ok && res.data) {
      setEvents(res.data);
    }
  }, [fetch]);

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>VM 트레이스</Text>
        <View style={[styles.indicator, tracing ? styles.indicatorOn : styles.indicatorOff]} />
        <Text style={styles.status}>{tracing ? 'ON' : 'OFF'}</Text>
        <TouchableOpacity onPress={toggleTrace} style={[styles.btn, tracing ? styles.btnOff : styles.btnOn]}>
          <Text style={styles.btnText}>{tracing ? '중지' : '시작'}</Text>
        </TouchableOpacity>
        <TouchableOpacity onPress={refresh} style={styles.btn}>
          <Text style={styles.btnText}>새로고침</Text>
        </TouchableOpacity>
      </View>

      <ScrollView style={styles.list}>
        {events.length === 0 && (
          <Text style={styles.hint}>
            {tracing ? '트레이스 이벤트 대기 중...' : '트레이스를 시작하면 VM 실행 로그가 표시됩니다'}
          </Text>
        )}
        {events.map((evt, i) => (
          <View key={i} style={styles.eventRow}>
            <Text style={styles.eventTick}>t:{evt.tick}</Text>
            <Text style={styles.eventPC}>({evt.pcX},{evt.pcY})</Text>
            <Text style={styles.eventOp}>{OPCODE_NAMES[evt.opcode] || `0x${evt.opcode.toString(16)}`}</Text>
            <Text style={styles.eventReg}>R={evt.regR}</Text>
          </View>
        ))}
      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#151718' },
  header: { flexDirection: 'row', alignItems: 'center', padding: 12, gap: 8, borderBottomWidth: 0.5, borderBottomColor: '#334155' },
  title: { color: '#ECEDEE', fontSize: 14, fontWeight: '700' },
  indicator: { width: 8, height: 8, borderRadius: 4 },
  indicatorOn: { backgroundColor: '#4ADE80' },
  indicatorOff: { backgroundColor: '#F87171' },
  status: { color: '#9BA1A6', fontSize: 12, fontWeight: '600', flex: 1 },
  btn: { backgroundColor: '#0a7ea4', paddingHorizontal: 10, paddingVertical: 6, borderRadius: 6 },
  btnOn: { backgroundColor: '#4ADE80' },
  btnOff: { backgroundColor: '#F87171' },
  btnText: { color: '#fff', fontSize: 12, fontWeight: '600' },
  list: { flex: 1, padding: 8 },
  hint: { color: '#687076', fontSize: 13, lineHeight: 20, textAlign: 'center', marginTop: 24 },
  eventRow: { flexDirection: 'row', alignItems: 'center', gap: 8, paddingVertical: 4, paddingHorizontal: 8, borderBottomWidth: 0.5, borderBottomColor: '#1e2022' },
  eventTick: { color: '#FBBF24', fontFamily: 'monospace', fontSize: 11, width: 48 },
  eventPC: { color: '#9BA1A6', fontFamily: 'monospace', fontSize: 11, width: 72 },
  eventOp: { color: '#0a7ea4', fontFamily: 'monospace', fontSize: 11, fontWeight: '700', flex: 1 },
  eventReg: { color: '#ECEDEE', fontFamily: 'monospace', fontSize: 11 },
});

panelRegistry.register({
  id: 'trace',
  label: '트레이스',
  icon: 'trace',
  component: TracePanelComponent,
  phase: 4,
});

export default TracePanelComponent;
