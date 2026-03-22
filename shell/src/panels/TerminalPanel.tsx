import React, { useState, useRef } from 'react';
import { View, Text, TextInput, ScrollView, TouchableOpacity, StyleSheet } from 'react-native';
import { useShell } from '../hooks/useShell';
import { panelRegistry } from './registry';

function TerminalPanelComponent() {
  const { execAndCapture, history, clear } = useShell();
  const [input, setInput] = useState('');
  const scrollRef = useRef<ScrollView>(null);

  const handleSubmit = () => {
    if (!input.trim()) return;
    execAndCapture(input.trim());
    setInput('');
    setTimeout(() => scrollRef.current?.scrollToEnd({ animated: true }), 100);
  };

  return (
    <View style={styles.container}>
      <ScrollView ref={scrollRef} style={styles.output}>
        {history.map((entry, i) => (
          <View key={i} style={styles.entry}>
            <Text style={styles.prompt}>$ {entry.input}</Text>
            <Text style={styles.result}>{entry.output}</Text>
          </View>
        ))}
        {history.length === 0 && (
          <Text style={styles.hint}>
            CanvasOS Shell 준비됨{'\n'}
            명령어: info, hash, tick, gate, branch, snapshot, rewind{'\n'}
            한글코드: ㅎㅏ 48{'\n'}
            픽셀코드: B=01 R=48 !
          </Text>
        )}
      </ScrollView>

      <View style={styles.inputRow}>
        <Text style={styles.dollar}>$</Text>
        <TextInput
          style={styles.input}
          value={input}
          onChangeText={setInput}
          onSubmitEditing={handleSubmit}
          placeholder="명령어 입력..."
          placeholderTextColor="#687076"
          autoCapitalize="none"
          autoCorrect={false}
          returnKeyType="send"
        />
        <TouchableOpacity onPress={handleSubmit} style={styles.sendBtn}>
          <Text style={styles.sendText}>실행</Text>
        </TouchableOpacity>
      </View>

      <TouchableOpacity onPress={clear} style={styles.clearBtn}>
        <Text style={styles.clearText}>지우기</Text>
      </TouchableOpacity>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1117' },
  output: { flex: 1, padding: 8 },
  entry: { marginBottom: 4 },
  prompt: { color: '#4ADE80', fontFamily: 'monospace', fontSize: 13 },
  result: { color: '#ECEDEE', fontFamily: 'monospace', fontSize: 13, marginLeft: 8 },
  hint: { color: '#687076', fontFamily: 'monospace', fontSize: 12, lineHeight: 20 },
  inputRow: { flexDirection: 'row', alignItems: 'center', borderTopWidth: 0.5, borderTopColor: '#334155', padding: 8 },
  dollar: { color: '#4ADE80', fontFamily: 'monospace', fontSize: 14, marginRight: 4 },
  input: { flex: 1, color: '#ECEDEE', fontFamily: 'monospace', fontSize: 14, padding: 8, backgroundColor: '#161b22', borderRadius: 6, borderWidth: 0.5, borderColor: '#334155' },
  sendBtn: { marginLeft: 8, backgroundColor: '#0a7ea4', paddingHorizontal: 12, paddingVertical: 8, borderRadius: 6 },
  sendText: { color: '#fff', fontSize: 13, fontWeight: '600' },
  clearBtn: { padding: 6, alignItems: 'center' },
  clearText: { color: '#687076', fontSize: 12 },
});

panelRegistry.register({
  id: 'terminal',
  label: '터미널',
  icon: 'terminal',
  component: TerminalPanelComponent,
  phase: 2,
});

export default TerminalPanelComponent;
