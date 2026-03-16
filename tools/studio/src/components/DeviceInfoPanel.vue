<template>
  <div class="panel device-panel">
    <h3 class="panel-title">
      <i class="pi pi-info-circle"></i>
      设备信息
    </h3>
    <div class="info-list">
      <div v-for="item in deviceStore.deviceInfoList" :key="item.key" class="info-item">
        <span class="info-label">{{ item.key }}</span>
        <span class="info-value">{{ item.value }}</span>
      </div>
    </div>
    <div class="version-divider"></div>
    <div class="version-badges-compact">
      <div class="ver-badge studio">
        <span class="ver-badge-icon"><CatEmoji /></span>
        <span class="ver-badge-ver">v{{ releaseStore.studioVersion }}</span>
      </div>
      <div class="ver-badge ch552">
        <span class="ver-badge-tag">552</span>
        <span class="ver-badge-ver">v{{ releaseStore.latestVersions.ch552 }}</span>
      </div>
      <div class="ver-badge ch592">
        <span class="ver-badge-tag">592</span>
        <span class="ver-badge-ver">v{{ releaseStore.latestVersions.ch592 }}</span>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { useDeviceStore } from '@/stores/deviceStore';
import CatEmoji from '@/components/CatEmoji.vue';
import { useReleaseStore } from '@/stores/releaseStore';

const deviceStore = useDeviceStore();
const releaseStore = useReleaseStore();
</script>

<style scoped>
.device-panel {
  /* 继承 .panel 基础样式 */
}

.info-list {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.info-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-size: 0.85rem;
}

.info-label {
  color: var(--c-text-secondary);
}

.info-value {
  color: var(--c-text-primary);
  font-weight: 500;
  font-family: 'JetBrains Mono', 'Fira Code', monospace;
  font-size: 0.8rem;
}

.version-divider {
  height: 1px;
  margin: 0.85rem 0;
  background: var(--c-border);
}

.version-badges-compact {
  display: flex;
  gap: 0.4rem;
}

.ver-badge {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 3px;
  padding: 0.45rem 0.3rem;
  border-radius: var(--radius-sm);
  border: 1px solid var(--c-border-light);
  background: var(--c-bg-primary);
  transition: border-color var(--transition-fast);
}

.ver-badge:hover {
  border-color: var(--c-border);
}

.ver-badge-icon {
  font-size: 0.9rem;
  line-height: 1;
}

.ver-badge-tag {
  font-size: 0.55rem;
  font-weight: 800;
  font-family: 'JetBrains Mono', 'Fira Code', monospace;
  padding: 0.1rem 0.3rem;
  border-radius: 3px;
  letter-spacing: 0.04em;
  line-height: 1.2;
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
  font-size: 0.72rem;
  font-weight: 700;
  font-family: 'JetBrains Mono', 'Fira Code', monospace;
  color: var(--c-text-primary);
  line-height: 1;
}

.ver-badge.studio .ver-badge-ver {
  color: var(--c-accent-light);
}
</style>
