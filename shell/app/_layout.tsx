import React from 'react';
import { View, StyleSheet } from 'react-native';
import { EngineProvider } from '../src/hooks/useEngine';

import TabLayout from './(tabs)/_layout';

export default function RootLayout() {
  return (
    <View style={styles.root}>
      <EngineProvider>
        <TabLayout />
      </EngineProvider>
    </View>
  );
}

const styles = StyleSheet.create({
  root: { flex: 1, backgroundColor: '#151718' },
});
