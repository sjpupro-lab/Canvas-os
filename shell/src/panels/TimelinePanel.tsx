import React, { useState, useCallback } from 'react';
import { View, Text, ScrollView, TouchableOpacity, StyleSheet } from 'react-native';
import { useEngine } from '../hooks/useEngine';
import { useEngineQuery } from '../hooks/useQuery';
import { useEngineStore } from '../stores/engineStore';
import { panelRegistry } from './registry';
import type { SnapshotInfo } from '../types/engine';

function TimelinePanelComponent() {
  const engine = useEngine();
  const { fetch } = useEngineQuery();
  const tick = useEngineStore((s) => s.tick);
  const [snapshots, setSnapshots] = useState<SnapshotInfo[]>([]);

  const refresh = useCallback(() => {
    const res = fetch<SnapshotInfo[]>('snapshotList');
    if (res.ok && res.data) {
      setSnapshots(res.data);
    }
  }, [fetch]);

  const rewind = (delta: number) => {
    const target = Math.max(0, tick + delta);
    engine.exec(`rewind ${target}`);
  };

  const advanceTick = (n: number) => {
    engine.tick(n);
  };

  const takeSnapshot = () => {
    engine.exec('snapshot');
    refresh();
  };

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>타임라인</Text>
        <Text style={styles.tickLabel}>tick: {tick}</Text>
      </View>

      <View style={styles.controls}>
        <TouchableOpacity onPress={() => rewind(-100)} style={styles.ctrlBtn}>
          <Text style={styles.ctrlText}>-100</Text>
        </TouchableOpacity>
        <TouchableOpacity onPress={() => rewind(-10)} style={styles.ctrlBtn}>
          <Text style={styles.ctrlText}>-10</Text>
        </TouchableOpacity>
        <TouchableOpacity onPress={() => advanceTick(10)} style={styles.ctrlBtn}>
          <Text style={styles.ctrlText}>+10</Text>
        </TouchableOpacity>
        <TouchableOpacity onPress={() => advanceTick(100)} style={styles.ctrlBtn}>
          <Text style={styles.ctrlText}>+100</Text>
        </TouchableOpacity>
        <TouchableOpacity onPress={takeSnapshot} style={[styles.ctrlBtn, styles.snapBtn]}>
          <Text style={styles.ctrlText}>snapshot</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.sectionHeader}>
        <Text style={styles.sectionTitle}>스냅샷 목록</Text>
        <TouchableOpacity onPress={refresh} style={styles.refreshBtn}>
          <Text style={styles.refreshText}>새로고침</Text>
        </TouchableOpacity>
      </View>

      <ScrollView style={styles.list}>
        {snapshots.length === 0 && (
          <Text style={styles.hint}>스냅샷이 없습니다</Text>
        )}
        {snapshots.map((snap) => (
          <TouchableOpacity
            key={snap.id}
            style={styles.snapRow}
            onPress={() => engine.exec(`rewind ${snap.tick}`)}
          >
            <Text style={styles.snapId}>#{snap.id}</Text>
            <Text style={styles.snapTick}>tick: {snap.tick}</Text>
            <Text style={styles.snapBranch}>br: {snap.branchId}</Text>
          </TouchableOpacity>
        ))}
      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#151718' },
  header: { flexDirection: 'row', alignItems: 'center', padding: 12, borderBottomWidth: 0.5, borderBottomColor: '#334155' },
  title: { color: '#ECEDEE', fontSize: 14, fontWeight: '700', flex: 1 },
  tickLabel: { color: '#FBBF24', fontFamily: 'monospace', fontSize: 14, fontWeight: '700' },
  controls: { flexDirection: 'row', padding: 8, gap: 6, flexWrap: 'wrap' },
  ctrlBtn: { backgroundColor: '#1e2022', paddingHorizontal: 12, paddingVertical: 8, borderRadius: 6, borderWidth: 0.5, borderColor: '#334155' },
  snapBtn: { backgroundColor: '#0a7ea4' },
  ctrlText: { color: '#ECEDEE', fontFamily: 'monospace', fontSize: 12, fontWeight: '600' },
  sectionHeader: { flexDirection: 'row', alignItems: 'center', paddingHorizontal: 12, paddingVertical: 6, borderBottomWidth: 0.5, borderBottomColor: '#334155' },
  sectionTitle: { color: '#9BA1A6', fontSize: 12, fontWeight: '600', flex: 1 },
  refreshBtn: { padding: 4 },
  refreshText: { color: '#0a7ea4', fontSize: 11 },
  list: { flex: 1, padding: 8 },
  hint: { color: '#687076', fontSize: 13, textAlign: 'center', marginTop: 16 },
  snapRow: { flexDirection: 'row', alignItems: 'center', gap: 12, backgroundColor: '#1e2022', borderRadius: 6, padding: 10, marginBottom: 6, borderWidth: 0.5, borderColor: '#334155' },
  snapId: { color: '#0a7ea4', fontFamily: 'monospace', fontSize: 12, fontWeight: '700' },
  snapTick: { color: '#ECEDEE', fontFamily: 'monospace', fontSize: 12, flex: 1 },
  snapBranch: { color: '#9BA1A6', fontFamily: 'monospace', fontSize: 11 },
});

panelRegistry.register({
  id: 'timeline',
  label: '타임라인',
  icon: 'timeline',
  component: TimelinePanelComponent,
  phase: 3,
});

export default TimelinePanelComponent;
