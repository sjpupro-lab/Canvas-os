import React, { useState } from 'react';
import { View, Text, TouchableOpacity, StyleSheet } from 'react-native';

interface PanelContainerProps {
  label: string;
  children: React.ReactNode;
}

export default function PanelContainer({ label, children }: PanelContainerProps) {
  const [collapsed, setCollapsed] = useState(false);

  return (
    <View style={styles.container}>
      <TouchableOpacity onPress={() => setCollapsed(!collapsed)} style={styles.titleBar}>
        <Text style={styles.toggle}>{collapsed ? '+' : '-'}</Text>
        <Text style={styles.label}>{label}</Text>
      </TouchableOpacity>
      {!collapsed && <View style={styles.content}>{children}</View>}
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#151718',
    borderWidth: 0.5,
    borderColor: '#334155',
    borderRadius: 8,
    overflow: 'hidden',
  },
  titleBar: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#1e2022',
    paddingHorizontal: 10,
    paddingVertical: 6,
    borderBottomWidth: 0.5,
    borderBottomColor: '#334155',
  },
  toggle: {
    color: '#9BA1A6',
    fontFamily: 'monospace',
    fontSize: 14,
    fontWeight: '700',
    width: 18,
  },
  label: {
    color: '#ECEDEE',
    fontSize: 13,
    fontWeight: '600',
  },
  content: {
    flex: 1,
  },
});
