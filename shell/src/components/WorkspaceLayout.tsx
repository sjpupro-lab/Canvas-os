import React from 'react';
import { View, Text, ScrollView, TouchableOpacity, StyleSheet } from 'react-native';
import { useUiStore } from '../stores/uiStore';
import { panelRegistry } from '../panels/registry';

interface WorkspaceLayoutProps {
  mainPanel: React.ReactNode;
  subPanel: React.ReactNode;
}

export default function WorkspaceLayout({ mainPanel, subPanel }: WorkspaceLayoutProps) {
  const canvasSplitRatio = useUiStore((s) => s.canvasSplitRatio);
  const activePanelId = useUiStore((s) => s.activePanelId);
  const setActivePanel = useUiStore((s) => s.setActivePanel);
  const allPanels = panelRegistry.getAll();

  return (
    <View style={styles.container}>
      {/* Upper area: main panel (canvas) */}
      <View style={[styles.main, { flex: canvasSplitRatio }]}>
        {mainPanel}
      </View>

      {/* Panel tab bar */}
      <ScrollView
        horizontal
        showsHorizontalScrollIndicator={false}
        style={styles.tabBar}
        contentContainerStyle={styles.tabBarContent}
      >
        {allPanels.map((panel) => {
          const isActive = panel.id === activePanelId;
          return (
            <TouchableOpacity
              key={panel.id}
              onPress={() => setActivePanel(panel.id)}
              style={[styles.tab, isActive && styles.tabActive]}
            >
              <Text style={[styles.tabText, isActive && styles.tabTextActive]}>
                {panel.label}
              </Text>
            </TouchableOpacity>
          );
        })}
      </ScrollView>

      {/* Lower area: sub panel */}
      <View style={[styles.sub, { flex: 1 - canvasSplitRatio }]}>
        {subPanel}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#151718' },
  main: { backgroundColor: '#151718' },
  tabBar: {
    maxHeight: 36,
    backgroundColor: '#0d1117',
    borderTopWidth: 0.5,
    borderTopColor: '#334155',
    borderBottomWidth: 0.5,
    borderBottomColor: '#334155',
  },
  tabBarContent: { alignItems: 'center', paddingHorizontal: 4 },
  tab: {
    paddingHorizontal: 12,
    paddingVertical: 8,
    marginHorizontal: 2,
    borderRadius: 4,
  },
  tabActive: { backgroundColor: '#0a7ea4' },
  tabText: { color: '#9BA1A6', fontSize: 12, fontWeight: '600' },
  tabTextActive: { color: '#fff' },
  sub: { backgroundColor: '#151718' },
});
