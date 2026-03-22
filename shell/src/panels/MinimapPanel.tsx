import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { useEngineStore } from '../stores/engineStore';
import { panelRegistry } from './registry';

function MinimapPanelComponent() {
  const initialized = useEngineStore((s) => s.initialized);
  const tick = useEngineStore((s) => s.tick);

  return (
    <View style={styles.container}>
      <View style={styles.placeholder}>
        <Text style={styles.placeholderText}>미니맵 (Skia 연동 후 활성화)</Text>
        <View style={styles.statusRow}>
          <View style={[styles.dot, initialized ? styles.dotOn : styles.dotOff]} />
          <Text style={styles.statusText}>
            엔진: {initialized ? '연결됨' : '미연결'} | tick: {tick}
          </Text>
        </View>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#151718', justifyContent: 'center', alignItems: 'center' },
  placeholder: { alignItems: 'center', padding: 24 },
  placeholderText: { color: '#9BA1A6', fontSize: 14, fontWeight: '600', marginBottom: 12 },
  statusRow: { flexDirection: 'row', alignItems: 'center', gap: 6 },
  dot: { width: 8, height: 8, borderRadius: 4 },
  dotOn: { backgroundColor: '#4ADE80' },
  dotOff: { backgroundColor: '#F87171' },
  statusText: { color: '#687076', fontFamily: 'monospace', fontSize: 12 },
});

panelRegistry.register({
  id: 'minimap',
  label: '미니맵',
  icon: 'minimap',
  component: MinimapPanelComponent,
  phase: 4,
});

export default MinimapPanelComponent;
