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
  </div>
</template>

<script setup lang="ts">
import { useDeviceStore } from '@/stores/deviceStore';

const deviceStore = useDeviceStore();
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
  color: var(--text-secondary);
}

.info-value {
  color: var(--text-primary);
  font-weight: 500;
  font-family: 'JetBrains Mono', 'Fira Code', monospace;
  font-size: 0.8rem;
}
</style>
