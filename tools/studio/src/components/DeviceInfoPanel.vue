<template>
  <div class="panel device-panel">
    <h3 class="panel-title">
      <i class="pi pi-info-circle"></i>
      设备信息
    </h3>

    <div class="info-list">
      <div v-for="item in deviceStore.deviceInfoList" :key="item.key" class="info-item">
        <span class="info-label">{{ item.key }}</span>
        <span class="info-value-row">
          <span
            v-if="item.key === '固件版本'"
            class="ver-hue-dot"
            :style="{ background: hueColor(deviceHue) }"
            :title="`设备色相 ${deviceHue}°`"
          ></span>
          <span class="info-value">{{ item.value }}</span>
        </span>
      </div>
    </div>

    <div class="version-divider"></div>

    <div class="version-section">
      <div class="version-section-head">
        <strong>版本</strong>
        <span>{{ firmwareStateLabel }}</span>
      </div>
      <div class="version-badges-compact">
        <div class="ver-badge studio">
          <span class="ver-badge-icon"><CatEmoji /></span>
          <span class="ver-badge-label">Studio</span>
          <span class="ver-badge-ver">v{{ releaseStore.latestStudioVersion }}</span>
          <span class="ver-hue-dot" :style="{ background: hueColor(studioHue) }" :title="`Studio 色相 ${studioHue}°`"></span>
        </div>
        <div class="ver-badge ch552">
          <span class="ver-badge-tag">552</span>
          <span class="ver-badge-label">CH552G</span>
          <span class="ver-badge-ver">v{{ releaseStore.latestVersions.ch552 }}</span>
          <span class="ver-hue-dot" :style="{ background: hueColor(ch552Hue) }" :title="`CH552 色相 ${ch552Hue}°`"></span>
        </div>
        <div class="ver-badge ch592">
          <span class="ver-badge-tag">592</span>
          <span class="ver-badge-label">CH592F</span>
          <span class="ver-badge-ver">v{{ releaseStore.latestVersions.ch592 }}</span>
          <span class="ver-hue-dot" :style="{ background: hueColor(ch592Hue) }" :title="`CH592 色相 ${ch592Hue}°`"></span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import { useDeviceStore } from '@/stores/deviceStore';
import CatEmoji from '@/components/CatEmoji.vue';
import { useReleaseStore } from '@/stores/releaseStore';
import { versionToHue } from '@/composables/useTheme';
import { useFirmwareUpdateInfo } from '@/composables/useFirmwareUpdateInfo';

const deviceStore = useDeviceStore();
const releaseStore = useReleaseStore();

const studioHue = computed(() => versionToHue(releaseStore.latestStudioVersion));
const ch552Hue = computed(() => versionToHue(releaseStore.latestVersions.ch552));
const ch592Hue = computed(() => versionToHue(releaseStore.latestVersions.ch592));
const deviceHue = computed(() => versionToHue(deviceStore.firmwareVersion));

const { firmwareStateLabel } = useFirmwareUpdateInfo();

function hueColor(hue: number) {
  return `hsl(${hue}, 70%, 60%)`;
}
</script>

<style scoped>
.info-list {
  display: grid;
  grid-template-columns: 1fr;
  gap: 0.55rem;
}

.info-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 0.75rem;
  font-size: 0.85rem;
}

.info-label {
  color: var(--c-text-secondary);
}

.info-value {
  color: var(--c-text-primary);
  font-weight: 700;
  font-family: 'JetBrains Mono', 'Fira Code', monospace;
  font-size: 0.8rem;
}

.info-value-row {
  display: flex;
  align-items: center;
  gap: 6px;
  min-width: 0;
}

.version-divider {
  height: 1px;
  margin: 0.9rem 0;
  background: var(--c-border);
}

.version-section {
  display: flex;
  flex-direction: column;
  gap: 0.55rem;
}

.version-section-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 0.75rem;
}

.version-section-head strong {
  color: var(--c-text-primary);
  font-size: 0.86rem;
}

.version-section-head span {
  color: var(--c-text-secondary);
  font-size: 0.72rem;
  font-weight: 800;
}

.version-badges-compact {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 0.45rem;
}

.ver-badge {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 3px;
  min-width: 0;
  padding: 0.48rem 0.25rem;
  border-radius: var(--radius-sm);
  border: 1px solid var(--c-border-light);
  background: var(--c-bg-primary);
  transition: border-color var(--transition-fast), transform var(--transition-fast);
}

.ver-badge:hover {
  border-color: var(--c-border);
  transform: translateY(-1px);
}

.ver-badge-icon {
  font-size: 0.9rem;
  line-height: 1;
}

.ver-badge-tag {
  font-size: 0.5rem;
  font-weight: 900;
  font-family: 'JetBrains Mono', 'Fira Code', monospace;
  padding: 0.08rem 0.25rem;
  border-radius: 3px;
  letter-spacing: 0.04em;
  line-height: 1.2;
}

.ver-badge-label {
  color: var(--c-text-muted);
  font-size: 0.58rem;
  font-weight: 900;
}

.ver-badge.ch552 .ver-badge-tag {
  background: rgba(96, 165, 250, 0.15);
  color: #60a5fa;
  border: 1px solid rgba(96, 165, 250, 0.25);
}

.ver-badge.ch592 .ver-badge-tag {
  background: rgba(74, 222, 128, 0.15);
  color: #4ade80;
  border: 1px solid rgba(74, 222, 128, 0.25);
}

.ver-badge-ver {
  font-size: 0.68rem;
  font-weight: 800;
  font-family: 'JetBrains Mono', 'Fira Code', monospace;
  color: var(--c-text-primary);
  line-height: 1;
}

.ver-badge.studio .ver-badge-ver {
  color: var(--c-accent-light);
}

.ver-hue-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  margin-top: 2px;
  cursor: default;
}
</style>
