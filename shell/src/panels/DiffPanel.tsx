import React, { useState } from 'react';
import { View, Text, TextInput, ScrollView, TouchableOpacity, StyleSheet } from 'react-native';
import { useEngineQuery } from '../hooks/useQuery';
import { panelRegistry } from './registry';
import type { DiffCell } from '../types/engine';

function DiffPanelComponent() {
  const { fetch } = useEngineQuery();
  const [branchA, setBranchA] = useState('0');
  const [branchB, setBranchB] = useState('1');
  const [diffs, setDiffs] = useState<DiffCell[]>([]);
  const [compared, setCompared] = useState(false);

  const compare = () => {
    const res = fetch<DiffCell[]>('diffBranch', {
      a: parseInt(branchA) || 0,
      b: parseInt(branchB) || 0,
    });
    if (res.ok && res.data) {
      setDiffs(res.data);
    } else {
      setDiffs([]);
    }
    setCompared(true);
  };

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Diff 비교</Text>
      </View>

      <View style={styles.inputRow}>
        <Text style={styles.label}>A:</Text>
        <TextInput
          style={styles.input}
          value={branchA}
          onChangeText={setBranchA}
          keyboardType="numeric"
          placeholderTextColor="#687076"
        />
        <Text style={styles.label}>B:</Text>
        <TextInput
          style={styles.input}
          value={branchB}
          onChangeText={setBranchB}
          keyboardType="numeric"
          placeholderTextColor="#687076"
        />
        <TouchableOpacity onPress={compare} style={styles.btn}>
          <Text style={styles.btnText}>비교</Text>
        </TouchableOpacity>
      </View>

      {compared && (
        <View style={styles.summary}>
          <Text style={styles.summaryText}>변경된 셀: {diffs.length}개</Text>
        </View>
      )}

      <ScrollView style={styles.list}>
        {!compared && (
          <Text style={styles.hint}>두 브랜치 ID를 입력하고 비교하세요</Text>
        )}
        {compared && diffs.length === 0 && (
          <Text style={styles.hint}>차이가 없습니다</Text>
        )}
        {diffs.map((d, i) => (
          <View key={i} style={styles.diffRow}>
            <Text style={styles.coord}>({d.x}, {d.y})</Text>
            <Text style={styles.before}>
              B:0x{d.beforeB.toString(16).padStart(2, '0')} R:0x{d.beforeR.toString(16).padStart(2, '0')}
            </Text>
            <Text style={styles.arrow}>{' -> '}</Text>
            <Text style={styles.after}>
              B:0x{d.afterB.toString(16).padStart(2, '0')} R:0x{d.afterR.toString(16).padStart(2, '0')}
            </Text>
          </View>
        ))}
      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#151718' },
  header: { padding: 12, borderBottomWidth: 0.5, borderBottomColor: '#334155' },
  title: { color: '#ECEDEE', fontSize: 14, fontWeight: '700' },
  inputRow: { flexDirection: 'row', alignItems: 'center', padding: 12, gap: 6 },
  label: { color: '#9BA1A6', fontSize: 13, fontWeight: '600' },
  input: { width: 56, color: '#ECEDEE', backgroundColor: '#1e2022', borderRadius: 6, borderWidth: 0.5, borderColor: '#334155', padding: 6, fontFamily: 'monospace', fontSize: 13, textAlign: 'center' },
  btn: { backgroundColor: '#0a7ea4', paddingHorizontal: 12, paddingVertical: 6, borderRadius: 6, marginLeft: 4 },
  btnText: { color: '#fff', fontSize: 13, fontWeight: '600' },
  summary: { backgroundColor: '#1e2022', marginHorizontal: 12, padding: 8, borderRadius: 6, borderWidth: 0.5, borderColor: '#334155' },
  summaryText: { color: '#FBBF24', fontFamily: 'monospace', fontSize: 13, fontWeight: '600' },
  list: { flex: 1, padding: 8 },
  hint: { color: '#687076', fontSize: 13, textAlign: 'center', marginTop: 24 },
  diffRow: { flexDirection: 'row', alignItems: 'center', backgroundColor: '#1e2022', borderRadius: 6, padding: 8, marginBottom: 4, borderWidth: 0.5, borderColor: '#334155' },
  coord: { color: '#0a7ea4', fontFamily: 'monospace', fontSize: 12, fontWeight: '600', width: 80 },
  before: { color: '#F87171', fontFamily: 'monospace', fontSize: 11 },
  arrow: { color: '#9BA1A6', fontFamily: 'monospace', fontSize: 11 },
  after: { color: '#4ADE80', fontFamily: 'monospace', fontSize: 11 },
});

panelRegistry.register({
  id: 'diff',
  label: 'Diff',
  icon: 'diff',
  component: DiffPanelComponent,
  phase: 3,
});

export default DiffPanelComponent;
