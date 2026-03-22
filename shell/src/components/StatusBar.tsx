import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { useEngineStore } from '../stores/engineStore';
import { VIS_MODES } from '../types/engine';

export default function StatusBar() {
  const tick = useEngineStore((s) => s.tick);
  const hash = useEngineStore((s) => s.hash);
  const branchId = useEngineStore((s) => s.branchId);
  const openGates = useEngineStore((s) => s.openGates);
  const visMode = useEngineStore((s) => s.visMode);

  return (
    <View style={styles.container}>
      <Text style={styles.item}>tick:{tick}</Text>
      <Text style={styles.sep}>|</Text>
      <Text style={styles.item}>{hash ? hash.slice(0, 8) : '--------'}</Text>
      <Text style={styles.sep}>|</Text>
      <Text style={styles.item}>br:{branchId}</Text>
      <Text style={styles.sep}>|</Text>
      <Text style={styles.item}>gates:{openGates}</Text>
      <Text style={styles.sep}>|</Text>
      <Text style={styles.item}>{VIS_MODES[visMode] || 'ABGR'}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    height: 32,
    backgroundColor: '#0d1117',
    flexDirection: 'row',
    alignItems: 'center',
    paddingHorizontal: 12,
    borderBottomWidth: 0.5,
    borderBottomColor: '#334155',
  },
  item: {
    color: '#ECEDEE',
    fontFamily: 'monospace',
    fontSize: 11,
  },
  sep: {
    color: '#334155',
    fontFamily: 'monospace',
    fontSize: 11,
    marginHorizontal: 6,
  },
});
