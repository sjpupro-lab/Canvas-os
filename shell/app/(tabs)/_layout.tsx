import React, { useState } from 'react';
import { View, Text, TouchableOpacity, StyleSheet } from 'react-native';

import HomeScreen from './index';
import WorkspaceScreen from './workspace';
import SettingsScreen from './settings';

const TABS = [
  { key: 'home', label: 'Home', icon: '\u2302' },
  { key: 'workspace', label: 'Workspace', icon: '\u2588' },
  { key: 'settings', label: 'Settings', icon: '\u2699' },
] as const;

type TabKey = typeof TABS[number]['key'];

export default function TabLayout() {
  const [activeTab, setActiveTab] = useState<TabKey>('home');

  const renderScreen = () => {
    switch (activeTab) {
      case 'home': return <HomeScreen />;
      case 'workspace': return <WorkspaceScreen />;
      case 'settings': return <SettingsScreen />;
    }
  };

  return (
    <View style={styles.container}>
      <View style={styles.content}>
        {renderScreen()}
      </View>
      <View style={styles.tabBar}>
        {TABS.map((tab) => {
          const isActive = activeTab === tab.key;
          return (
            <TouchableOpacity
              key={tab.key}
              onPress={() => setActiveTab(tab.key)}
              style={styles.tab}
            >
              <Text style={[styles.tabIcon, isActive && styles.tabIconActive]}>{tab.icon}</Text>
              <Text style={[styles.tabLabel, isActive && styles.tabLabelActive]}>{tab.label}</Text>
            </TouchableOpacity>
          );
        })}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#151718' },
  content: { flex: 1 },
  tabBar: {
    flexDirection: 'row',
    backgroundColor: '#0d1117',
    borderTopWidth: 0.5,
    borderTopColor: '#334155',
    paddingBottom: 16,
    paddingTop: 6,
  },
  tab: { flex: 1, alignItems: 'center', justifyContent: 'center' },
  tabIcon: { color: '#9BA1A6', fontSize: 20, marginBottom: 2 },
  tabIconActive: { color: '#0a7ea4' },
  tabLabel: { color: '#9BA1A6', fontSize: 10, fontWeight: '600' },
  tabLabelActive: { color: '#0a7ea4' },
});
