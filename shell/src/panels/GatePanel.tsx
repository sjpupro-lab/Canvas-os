import React, { useState, useCallback } from 'react';
import { View, Text, ScrollView, TouchableOpacity, StyleSheet } from 'react-native';
import { useEngine } from '../hooks/useEngine';
import { useEngineQuery } from '../hooks/useQuery';
import { panelRegistry } from './registry';
import type { GateInfo } from '../types/engine';

function GatePanelComponent() {
  const engine = useEngine();
  const { fetch } = useEngineQuery();
  const [gates, setGates] = useState<GateInfo[]>([]);
  const [openCount, setOpenCount] = useState(0);

  const refresh = useCallback(() => {
    const res = fetch<GateInfo[]>('gateList');
    if (res.ok && res.data) {
      setGates(res.data);
      setOpenCount(res.data.filter((g) => g.isOpen).length);
    }
  }, [fetch]);

  const toggle = (gate: GateInfo) => {
    const cmd = gate.isOpen ? `gate close ${gate.id}` : `gate open ${gate.id}`;
    engine.exec(cmd);
    refresh();
  };

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>게이트 제어</Text>
        <Text style={styles.count}>열린 게이트: {openCount}</Text>
        <TouchableOpacity onPress={refresh} style={styles.btn}>
          <Text style={styles.btnText}>새로고침</Text>
        </TouchableOpacity>
      </View>

      <ScrollView style={styles.list}>
        {gates.length === 0 && (
          <Text style={styles.hint}>게이트 목록이 비어 있습니다{'\n'}새로고침을 눌러 조회하세요</Text>
        )}
        {gates.map((gate) => (
          <View key={gate.id} style={styles.row}>
            <View style={styles.gateInfo}>
              <Text style={styles.gateId}>#{gate.id}</Text>
              <Text style={styles.gateTile}>({gate.tileX}, {gate.tileY})</Text>
            </View>
            <TouchableOpacity
              onPress={() => toggle(gate)}
              style={[styles.toggleBtn, gate.isOpen ? styles.toggleOpen : styles.toggleClosed]}
            >
              <Text style={styles.toggleText}>{gate.isOpen ? 'OPEN' : 'CLOSED'}</Text>
            </TouchableOpacity>
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
  count: { color: '#9BA1A6', fontSize: 12, flex: 1 },
  btn: { backgroundColor: '#0a7ea4', paddingHorizontal: 10, paddingVertical: 6, borderRadius: 6 },
  btnText: { color: '#fff', fontSize: 12, fontWeight: '600' },
  list: { flex: 1, padding: 8 },
  hint: { color: '#687076', fontSize: 13, lineHeight: 20, textAlign: 'center', marginTop: 24 },
  row: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between', backgroundColor: '#1e2022', borderRadius: 6, padding: 10, marginBottom: 6, borderWidth: 0.5, borderColor: '#334155' },
  gateInfo: { flexDirection: 'row', alignItems: 'center', gap: 8 },
  gateId: { color: '#ECEDEE', fontFamily: 'monospace', fontSize: 13, fontWeight: '600' },
  gateTile: { color: '#9BA1A6', fontFamily: 'monospace', fontSize: 12 },
  toggleBtn: { paddingHorizontal: 12, paddingVertical: 4, borderRadius: 4 },
  toggleOpen: { backgroundColor: '#4ADE8033' },
  toggleClosed: { backgroundColor: '#F8717133' },
  toggleText: { fontFamily: 'monospace', fontSize: 11, fontWeight: '700', color: '#ECEDEE' },
});

panelRegistry.register({
  id: 'gate',
  label: '게이트',
  icon: 'gate',
  component: GatePanelComponent,
  phase: 3,
});

export default GatePanelComponent;
