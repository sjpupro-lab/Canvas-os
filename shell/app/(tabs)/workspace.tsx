import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

// Import all panels to trigger registration
import '../../src/panels/index';

import StatusBar from '../../src/components/StatusBar';
import WorkspaceLayout from '../../src/components/WorkspaceLayout';
import { panelRegistry } from '../../src/panels/registry';
import { useUiStore } from '../../src/stores/uiStore';

function CanvasPlaceholder() {
  return (
    <View style={styles.canvasPlaceholder}>
      <Text style={styles.canvasText}>캔버스 (Skia 연동 후 활성화)</Text>
    </View>
  );
}

export default function WorkspaceScreen() {
  const activePanelId = useUiStore((s) => s.activePanelId);
  const panelDef = panelRegistry.get(activePanelId);
  const ActivePanelComponent = panelDef?.component;

  return (
    <View style={styles.container}>
      <StatusBar />
      <WorkspaceLayout
        mainPanel={<CanvasPlaceholder />}
        subPanel={
          ActivePanelComponent ? (
            <ActivePanelComponent />
          ) : (
            <View style={styles.noPanelContainer}>
              <Text style={styles.noPanelText}>패널을 선택하세요</Text>
            </View>
          )
        }
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#151718' },
  canvasPlaceholder: {
    flex: 1,
    backgroundColor: '#0d1117',
    justifyContent: 'center',
    alignItems: 'center',
    borderWidth: 0.5,
    borderColor: '#334155',
    margin: 4,
    borderRadius: 8,
  },
  canvasText: { color: '#9BA1A6', fontSize: 14, fontWeight: '600' },
  noPanelContainer: { flex: 1, justifyContent: 'center', alignItems: 'center' },
  noPanelText: { color: '#687076', fontSize: 13 },
});
