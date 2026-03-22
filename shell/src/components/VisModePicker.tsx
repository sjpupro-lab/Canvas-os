import React from 'react';
import { View, Text, TouchableOpacity, StyleSheet } from 'react-native';
import { VIS_MODES } from '../types/engine';

interface VisModePickerProps {
  current: number;
  onChange: (mode: number) => void;
}

export default function VisModePicker({ current, onChange }: VisModePickerProps) {
  return (
    <View style={styles.container}>
      {VIS_MODES.map((label, index) => {
        const isActive = index === current;
        return (
          <TouchableOpacity
            key={label}
            onPress={() => onChange(index)}
            style={[styles.btn, isActive && styles.btnActive]}
          >
            <Text style={[styles.text, isActive && styles.textActive]}>{label}</Text>
          </TouchableOpacity>
        );
      })}
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    gap: 4,
    padding: 4,
    backgroundColor: '#1e2022',
    borderRadius: 8,
  },
  btn: {
    flex: 1,
    paddingVertical: 6,
    alignItems: 'center',
    borderRadius: 6,
  },
  btnActive: {
    backgroundColor: '#0a7ea4',
  },
  text: {
    color: '#9BA1A6',
    fontSize: 11,
    fontWeight: '600',
    fontFamily: 'monospace',
  },
  textActive: {
    color: '#fff',
  },
});
