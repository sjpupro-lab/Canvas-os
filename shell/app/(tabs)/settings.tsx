import React, { useState } from 'react';
import { View, Text, ScrollView, TouchableOpacity, Switch, StyleSheet } from 'react-native';
import { useEngine } from '../../src/hooks/useEngine';
import { useUiStore } from '../../src/stores/uiStore';

export default function SettingsScreen() {
  const engine = useEngine();
  const theme = useUiStore((s) => s.theme);
  const setTheme = useUiStore((s) => s.setTheme);
  const [detMode, setDetMode] = useState(false);
  const [lastResult, setLastResult] = useState('');

  const execCmd = (cmd: string) => {
    const result = engine.exec(cmd);
    setLastResult(result);
  };

  const toggleDet = (value: boolean) => {
    setDetMode(value);
    execCmd(`det ${value ? 'on' : 'off'}`);
  };

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      <Text style={styles.screenTitle}>설정</Text>

      {/* Session Management */}
      <Text style={styles.sectionTitle}>세션 관리</Text>
      <View style={styles.card}>
        <TouchableOpacity onPress={() => execCmd('cvp save session.cvp')} style={styles.rowBtn}>
          <Text style={styles.rowBtnText}>세션 저장</Text>
        </TouchableOpacity>
        <TouchableOpacity onPress={() => execCmd('cvp load session.cvp')} style={styles.rowBtn}>
          <Text style={styles.rowBtnText}>세션 불러오기</Text>
        </TouchableOpacity>
        <TouchableOpacity onPress={() => execCmd('reset')} style={[styles.rowBtn, styles.dangerBtn]}>
          <Text style={styles.rowBtnText}>엔진 리셋</Text>
        </TouchableOpacity>
      </View>

      {lastResult !== '' && (
        <View style={styles.resultBox}>
          <Text style={styles.resultText}>{lastResult}</Text>
        </View>
      )}

      {/* Engine Settings */}
      <Text style={styles.sectionTitle}>엔진 설정</Text>
      <View style={styles.card}>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>결정론 모드 (Determinism)</Text>
          <Switch
            value={detMode}
            onValueChange={toggleDet}
            trackColor={{ false: '#334155', true: '#0a7ea4' }}
            thumbColor={detMode ? '#4ADE80' : '#9BA1A6'}
          />
        </View>
      </View>

      {/* Theme */}
      <Text style={styles.sectionTitle}>테마</Text>
      <View style={styles.card}>
        {(['light', 'dark', 'system'] as const).map((t) => {
          const isActive = theme === t;
          const label = t === 'light' ? '라이트' : t === 'dark' ? '다크' : '시스템';
          return (
            <TouchableOpacity
              key={t}
              onPress={() => setTheme(t)}
              style={[styles.themeOption, isActive && styles.themeActive]}
            >
              <Text style={[styles.themeText, isActive && styles.themeTextActive]}>{label}</Text>
            </TouchableOpacity>
          );
        })}
      </View>

      {/* App Info */}
      <Text style={styles.sectionTitle}>앱 정보</Text>
      <View style={styles.card}>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>앱 이름</Text>
          <Text style={styles.infoValue}>CanvasOS Hybrid</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>버전</Text>
          <Text style={styles.infoValue}>1.0.0</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>엔진</Text>
          <Text style={styles.infoValue}>Mock (Dev)</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>프레임워크</Text>
          <Text style={styles.infoValue}>Expo + React Native</Text>
        </View>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#151718' },
  content: { padding: 20, paddingTop: 48 },
  screenTitle: { color: '#ECEDEE', fontSize: 24, fontWeight: '800', marginBottom: 24 },
  sectionTitle: { color: '#9BA1A6', fontSize: 12, fontWeight: '700', textTransform: 'uppercase', letterSpacing: 1, marginBottom: 8, marginTop: 20 },
  card: { backgroundColor: '#1e2022', borderRadius: 10, borderWidth: 0.5, borderColor: '#334155', overflow: 'hidden' },
  rowBtn: { paddingVertical: 14, paddingHorizontal: 16, borderBottomWidth: 0.5, borderBottomColor: '#334155' },
  rowBtnText: { color: '#0a7ea4', fontSize: 14, fontWeight: '600' },
  dangerBtn: { borderBottomWidth: 0 },
  resultBox: { backgroundColor: '#0d1117', borderRadius: 6, padding: 10, marginTop: 8, borderWidth: 0.5, borderColor: '#334155' },
  resultText: { color: '#4ADE80', fontFamily: 'monospace', fontSize: 12 },
  settingRow: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between', padding: 14 },
  settingLabel: { color: '#ECEDEE', fontSize: 14 },
  themeOption: { paddingVertical: 12, paddingHorizontal: 16, borderBottomWidth: 0.5, borderBottomColor: '#334155' },
  themeActive: { backgroundColor: '#0a7ea433' },
  themeText: { color: '#9BA1A6', fontSize: 14 },
  themeTextActive: { color: '#0a7ea4', fontWeight: '600' },
  infoRow: { flexDirection: 'row', justifyContent: 'space-between', padding: 14, borderBottomWidth: 0.5, borderBottomColor: '#334155' },
  infoLabel: { color: '#9BA1A6', fontSize: 13 },
  infoValue: { color: '#ECEDEE', fontSize: 13, fontFamily: 'monospace' },
});
