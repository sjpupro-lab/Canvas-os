import React from 'react';
import { View, Text, ScrollView, TouchableOpacity, StyleSheet } from 'react-native';
import { useEngineStore } from '../../src/stores/engineStore';

const FEATURES = [
  { title: '공간실행', desc: '2D 캔버스 위 VM', color: '#0a7ea4' },
  { title: '이중언어', desc: '한글코드 + PixelCode', color: '#4ADE80' },
  { title: '멀티버스', desc: '브랜치/게이트', color: '#FBBF24' },
  { title: '고성능엔진', desc: 'C Turbo Module', color: '#F87171' },
];

export default function HomeScreen() {
  const tick = useEngineStore((s) => s.tick);
  const hash = useEngineStore((s) => s.hash);
  const openGates = useEngineStore((s) => s.openGates);
  const branchId = useEngineStore((s) => s.branchId);

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.contentContainer}>
      {/* Hero */}
      <View style={styles.hero}>
        <Text style={styles.heroTitle}>Canvas-OS</Text>
        <Text style={styles.heroSub}>2D Spatial Operating System</Text>
      </View>

      {/* Quick Stats */}
      <View style={styles.statsRow}>
        <View style={styles.statCard}>
          <Text style={styles.statValue}>{tick}</Text>
          <Text style={styles.statLabel}>tick</Text>
        </View>
        <View style={styles.statCard}>
          <Text style={styles.statValue}>{hash ? hash.slice(0, 6) : '------'}</Text>
          <Text style={styles.statLabel}>hash</Text>
        </View>
        <View style={styles.statCard}>
          <Text style={styles.statValue}>{openGates}</Text>
          <Text style={styles.statLabel}>gates</Text>
        </View>
        <View style={styles.statCard}>
          <Text style={styles.statValue}>{branchId}</Text>
          <Text style={styles.statLabel}>branch</Text>
        </View>
      </View>

      {/* Core Features */}
      <Text style={styles.sectionTitle}>Core Features</Text>
      <View style={styles.featuresGrid}>
        {FEATURES.map((feat) => (
          <View key={feat.title} style={[styles.featureCard, { borderLeftColor: feat.color }]}>
            <Text style={styles.featureTitle}>{feat.title}</Text>
            <Text style={styles.featureDesc}>{feat.desc}</Text>
          </View>
        ))}
      </View>

      {/* CTA */}
      <TouchableOpacity style={styles.cta}>
        <Text style={styles.ctaText}>워크스페이스 열기</Text>
      </TouchableOpacity>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#151718' },
  contentContainer: { padding: 20, paddingTop: 48 },
  hero: { alignItems: 'center', marginBottom: 32 },
  heroTitle: { color: '#ECEDEE', fontSize: 32, fontWeight: '800', fontFamily: 'monospace' },
  heroSub: { color: '#9BA1A6', fontSize: 14, marginTop: 4 },
  statsRow: { flexDirection: 'row', gap: 8, marginBottom: 28 },
  statCard: { flex: 1, backgroundColor: '#1e2022', borderRadius: 8, padding: 12, alignItems: 'center', borderWidth: 0.5, borderColor: '#334155' },
  statValue: { color: '#ECEDEE', fontFamily: 'monospace', fontSize: 16, fontWeight: '700' },
  statLabel: { color: '#9BA1A6', fontSize: 10, fontWeight: '600', marginTop: 4 },
  sectionTitle: { color: '#ECEDEE', fontSize: 16, fontWeight: '700', marginBottom: 12 },
  featuresGrid: { gap: 8, marginBottom: 28 },
  featureCard: { backgroundColor: '#1e2022', borderRadius: 8, padding: 14, borderLeftWidth: 3, borderWidth: 0.5, borderColor: '#334155' },
  featureTitle: { color: '#ECEDEE', fontSize: 14, fontWeight: '700' },
  featureDesc: { color: '#9BA1A6', fontSize: 12, marginTop: 2 },
  cta: { backgroundColor: '#0a7ea4', borderRadius: 10, paddingVertical: 14, alignItems: 'center' },
  ctaText: { color: '#fff', fontSize: 16, fontWeight: '700' },
});
