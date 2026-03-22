import React, { useState, useCallback } from 'react';
import { View, Text, ScrollView, TouchableOpacity, StyleSheet } from 'react-native';
import { useEngine } from '../hooks/useEngine';
import { useEngineQuery } from '../hooks/useQuery';
import { panelRegistry } from './registry';
import type { BranchInfo } from '../types/engine';

function BranchPanelComponent() {
  const engine = useEngine();
  const { fetch } = useEngineQuery();
  const [branches, setBranches] = useState<BranchInfo[]>([]);
  const [activeBranch, setActiveBranch] = useState(0);

  const refresh = useCallback(() => {
    const res = fetch<BranchInfo[]>('branchList');
    if (res.ok && res.data) {
      setBranches(res.data);
    }
  }, [fetch]);

  const create = () => {
    engine.exec('branch create');
    refresh();
  };

  const switchBranch = (id: number) => {
    engine.exec(`branch switch ${id}`);
    setActiveBranch(id);
  };

  const merge = (id: number) => {
    engine.exec(`branch merge ${id}`);
    refresh();
  };

  const getDepth = (branch: BranchInfo): number => {
    if (branch.parentId < 0) return 0;
    const parent = branches.find((b) => b.id === branch.parentId);
    return parent ? getDepth(parent) + 1 : 0;
  };

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>브랜치 트리</Text>
        <TouchableOpacity onPress={refresh} style={styles.btn}>
          <Text style={styles.btnText}>새로고침</Text>
        </TouchableOpacity>
        <TouchableOpacity onPress={create} style={[styles.btn, styles.createBtn]}>
          <Text style={styles.btnText}>생성</Text>
        </TouchableOpacity>
      </View>

      <ScrollView style={styles.list}>
        {branches.length === 0 && (
          <Text style={styles.hint}>브랜치 목록이 비어 있습니다{'\n'}새로고침을 눌러 조회하세요</Text>
        )}
        {branches.map((branch) => {
          const depth = getDepth(branch);
          const isActive = branch.id === activeBranch;
          return (
            <View key={branch.id} style={[styles.row, { marginLeft: depth * 16 }]}>
              {depth > 0 && <Text style={styles.arrow}>{'└─ '}</Text>}
              <View style={styles.branchInfo}>
                <Text style={[styles.branchId, isActive && styles.activeText]}>
                  br:{branch.id}
                </Text>
                <Text style={styles.branchMeta}>
                  lane:{branch.laneId} plane:0x{branch.planeMask.toString(16)}
                </Text>
              </View>
              <View style={styles.actions}>
                <TouchableOpacity onPress={() => switchBranch(branch.id)} style={styles.actionBtn}>
                  <Text style={styles.actionText}>전환</Text>
                </TouchableOpacity>
                {branch.id !== 0 && (
                  <TouchableOpacity onPress={() => merge(branch.id)} style={styles.actionBtn}>
                    <Text style={styles.actionText}>병합</Text>
                  </TouchableOpacity>
                )}
              </View>
            </View>
          );
        })}
      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#151718' },
  header: { flexDirection: 'row', alignItems: 'center', padding: 12, gap: 8, borderBottomWidth: 0.5, borderBottomColor: '#334155' },
  title: { color: '#ECEDEE', fontSize: 14, fontWeight: '700', flex: 1 },
  btn: { backgroundColor: '#0a7ea4', paddingHorizontal: 10, paddingVertical: 6, borderRadius: 6 },
  createBtn: { backgroundColor: '#4ADE80' },
  btnText: { color: '#fff', fontSize: 12, fontWeight: '600' },
  list: { flex: 1, padding: 8 },
  hint: { color: '#687076', fontSize: 13, lineHeight: 20, textAlign: 'center', marginTop: 24 },
  row: { flexDirection: 'row', alignItems: 'center', backgroundColor: '#1e2022', borderRadius: 6, padding: 10, marginBottom: 6, borderWidth: 0.5, borderColor: '#334155' },
  arrow: { color: '#9BA1A6', fontFamily: 'monospace', fontSize: 12 },
  branchInfo: { flex: 1 },
  branchId: { color: '#ECEDEE', fontFamily: 'monospace', fontSize: 13, fontWeight: '600' },
  activeText: { color: '#4ADE80' },
  branchMeta: { color: '#9BA1A6', fontFamily: 'monospace', fontSize: 11, marginTop: 2 },
  actions: { flexDirection: 'row', gap: 6 },
  actionBtn: { backgroundColor: '#0a7ea4', paddingHorizontal: 8, paddingVertical: 4, borderRadius: 4 },
  actionText: { color: '#fff', fontSize: 11, fontWeight: '600' },
});

panelRegistry.register({
  id: 'branch',
  label: '브랜치',
  icon: 'branch',
  component: BranchPanelComponent,
  phase: 3,
});

export default BranchPanelComponent;
