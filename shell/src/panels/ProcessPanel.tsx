import React, { useState, useCallback } from 'react';
import { View, Text, TextInput, ScrollView, TouchableOpacity, StyleSheet } from 'react-native';
import { useEngine } from '../hooks/useEngine';
import { useEngineQuery } from '../hooks/useQuery';
import { panelRegistry } from './registry';
import type { ProcessInfo } from '../types/engine';

function ProcessPanelComponent() {
  const engine = useEngine();
  const { fetch } = useEngineQuery();
  const [processes, setProcesses] = useState<ProcessInfo[]>([]);
  const [killPid, setKillPid] = useState('');

  const refresh = useCallback(() => {
    const res = fetch<ProcessInfo[]>('processList');
    if (res.ok && res.data) {
      setProcesses(res.data);
    }
  }, [fetch]);

  const spawn = () => {
    engine.exec('spawn');
    refresh();
  };

  const kill = (pid?: number) => {
    const target = pid !== undefined ? pid : parseInt(killPid);
    if (isNaN(target)) return;
    engine.exec(`kill ${target}`);
    setKillPid('');
    refresh();
  };

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>프로세스 관리</Text>
        <TouchableOpacity onPress={refresh} style={styles.btn}>
          <Text style={styles.btnText}>새로고침</Text>
        </TouchableOpacity>
        <TouchableOpacity onPress={spawn} style={[styles.btn, styles.spawnBtn]}>
          <Text style={styles.btnText}>Spawn</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.killRow}>
        <Text style={styles.label}>PID:</Text>
        <TextInput
          style={styles.pidInput}
          value={killPid}
          onChangeText={setKillPid}
          keyboardType="numeric"
          placeholder="0"
          placeholderTextColor="#687076"
        />
        <TouchableOpacity onPress={() => kill()} style={[styles.btn, styles.killBtn]}>
          <Text style={styles.btnText}>Kill</Text>
        </TouchableOpacity>
      </View>

      {/* Table header */}
      <View style={styles.tableHeader}>
        <Text style={[styles.th, styles.colPid]}>PID</Text>
        <Text style={[styles.th, styles.colState]}>상태</Text>
        <Text style={[styles.th, styles.colEnergy]}>에너지</Text>
        <Text style={[styles.th, styles.colPC]}>PC</Text>
        <Text style={[styles.th, styles.colAction]}>작업</Text>
      </View>

      <ScrollView style={styles.list}>
        {processes.length === 0 && (
          <Text style={styles.hint}>프로세스가 없습니다{'\n'}새로고침을 눌러 조회하세요</Text>
        )}
        {processes.map((proc) => (
          <View key={proc.pid} style={styles.tableRow}>
            <Text style={[styles.td, styles.colPid]}>{proc.pid}</Text>
            <Text style={[styles.td, styles.colState, proc.state === 'running' ? styles.running : styles.stopped]}>
              {proc.state}
            </Text>
            <Text style={[styles.td, styles.colEnergy]}>{proc.energy}</Text>
            <Text style={[styles.td, styles.colPC]}>({proc.pcX},{proc.pcY})</Text>
            <View style={styles.colAction}>
              <TouchableOpacity onPress={() => kill(proc.pid)} style={styles.killSmall}>
                <Text style={styles.killSmallText}>kill</Text>
              </TouchableOpacity>
            </View>
          </View>
        ))}
      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#151718' },
  header: { flexDirection: 'row', alignItems: 'center', padding: 12, gap: 8, borderBottomWidth: 0.5, borderBottomColor: '#334155' },
  title: { color: '#ECEDEE', fontSize: 14, fontWeight: '700', flex: 1 },
  btn: { backgroundColor: '#0a7ea4', paddingHorizontal: 10, paddingVertical: 6, borderRadius: 6 },
  spawnBtn: { backgroundColor: '#4ADE80' },
  killBtn: { backgroundColor: '#F87171' },
  btnText: { color: '#fff', fontSize: 12, fontWeight: '600' },
  killRow: { flexDirection: 'row', alignItems: 'center', paddingHorizontal: 12, paddingVertical: 8, gap: 6, borderBottomWidth: 0.5, borderBottomColor: '#334155' },
  label: { color: '#9BA1A6', fontSize: 12, fontWeight: '600' },
  pidInput: { width: 56, color: '#ECEDEE', backgroundColor: '#1e2022', borderRadius: 6, borderWidth: 0.5, borderColor: '#334155', padding: 6, fontFamily: 'monospace', fontSize: 13, textAlign: 'center' },
  tableHeader: { flexDirection: 'row', paddingHorizontal: 12, paddingVertical: 6, backgroundColor: '#1e2022', borderBottomWidth: 0.5, borderBottomColor: '#334155' },
  th: { color: '#9BA1A6', fontFamily: 'monospace', fontSize: 11, fontWeight: '700' },
  list: { flex: 1 },
  hint: { color: '#687076', fontSize: 13, textAlign: 'center', marginTop: 24 },
  tableRow: { flexDirection: 'row', alignItems: 'center', paddingHorizontal: 12, paddingVertical: 8, borderBottomWidth: 0.5, borderBottomColor: '#1e2022' },
  td: { color: '#ECEDEE', fontFamily: 'monospace', fontSize: 12 },
  colPid: { width: 40 },
  colState: { width: 64 },
  colEnergy: { width: 56 },
  colPC: { flex: 1 },
  colAction: { width: 40, alignItems: 'center' as const },
  running: { color: '#4ADE80' },
  stopped: { color: '#F87171' },
  killSmall: { backgroundColor: '#F8717133', paddingHorizontal: 6, paddingVertical: 2, borderRadius: 3 },
  killSmallText: { color: '#F87171', fontFamily: 'monospace', fontSize: 10, fontWeight: '700' },
});

panelRegistry.register({
  id: 'process',
  label: '프로세스',
  icon: 'process',
  component: ProcessPanelComponent,
  phase: 4,
});

export default ProcessPanelComponent;
