import React, { useState } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet } from 'react-native';
import { useEngine } from '../hooks/useEngine';
import { panelRegistry } from './registry';
import type { CellData } from '../types/engine';
import { OPCODE_NAMES } from '../types/engine';

function CellInspectorPanelComponent() {
  const engine = useEngine();
  const [x, setX] = useState('512');
  const [y, setY] = useState('512');
  const [cell, setCell] = useState<CellData | null>(null);

  const inspect = () => {
    const cx = parseInt(x) || 0;
    const cy = parseInt(y) || 0;
    const data: CellData = JSON.parse(engine.getCell(cx, cy));
    setCell(data);
  };

  return (
    <View style={styles.container}>
      <View style={styles.inputRow}>
        <Text style={styles.label}>X:</Text>
        <TextInput style={styles.coordInput} value={x} onChangeText={setX} keyboardType="numeric" />
        <Text style={styles.label}>Y:</Text>
        <TextInput style={styles.coordInput} value={y} onChangeText={setY} keyboardType="numeric" />
        <TouchableOpacity onPress={inspect} style={styles.btn}>
          <Text style={styles.btnText}>조회</Text>
        </TouchableOpacity>
      </View>

      {cell && (
        <View style={styles.cellInfo}>
          <Text style={styles.field}>A: 0x{(cell.a >>> 0).toString(16).padStart(8, '0')}</Text>
          <Text style={styles.field}>B: 0x{cell.b.toString(16).padStart(2, '0')} ({OPCODE_NAMES[cell.b] || 'UNKNOWN'})</Text>
          <Text style={styles.field}>G: {cell.g} (에너지/상태)</Text>
          <Text style={styles.field}>R: 0x{cell.r.toString(16).padStart(2, '0')} {cell.r >= 32 && cell.r < 127 ? `('${String.fromCharCode(cell.r)}')` : ''}</Text>
          <View style={styles.divider} />
          <Text style={styles.meta}>타일: ({Math.floor(parseInt(x) / 16)}, {Math.floor(parseInt(y) / 16)})</Text>
          <Text style={styles.meta}>게이트 ID: {Math.floor(parseInt(y) / 16) * 64 + Math.floor(parseInt(x) / 16)}</Text>
        </View>
      )}

      {!cell && (
        <Text style={styles.hint}>셀 좌표를 입력하고 조회하세요{'\n'}캔버스 터치로도 선택 가능</Text>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 12, backgroundColor: '#151718' },
  inputRow: { flexDirection: 'row', alignItems: 'center', gap: 6, marginBottom: 12 },
  label: { color: '#9BA1A6', fontSize: 13, fontWeight: '600' },
  coordInput: { width: 64, color: '#ECEDEE', backgroundColor: '#1e2022', borderRadius: 6, borderWidth: 0.5, borderColor: '#334155', padding: 6, fontFamily: 'monospace', fontSize: 13, textAlign: 'center' },
  btn: { backgroundColor: '#0a7ea4', paddingHorizontal: 12, paddingVertical: 6, borderRadius: 6, marginLeft: 4 },
  btnText: { color: '#fff', fontSize: 13, fontWeight: '600' },
  cellInfo: { backgroundColor: '#1e2022', borderRadius: 8, padding: 12, borderWidth: 0.5, borderColor: '#334155' },
  field: { color: '#ECEDEE', fontFamily: 'monospace', fontSize: 14, lineHeight: 22 },
  divider: { height: 0.5, backgroundColor: '#334155', marginVertical: 8 },
  meta: { color: '#9BA1A6', fontFamily: 'monospace', fontSize: 12, lineHeight: 18 },
  hint: { color: '#687076', fontSize: 13, lineHeight: 20, textAlign: 'center', marginTop: 24 },
});

panelRegistry.register({
  id: 'cell',
  label: '셀',
  icon: 'info.circle',
  component: CellInspectorPanelComponent,
  phase: 3,
});

export default CellInspectorPanelComponent;
