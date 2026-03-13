<template>
  <div class="panel layer-panel">
    <h3 class="panel-title">
      <i class="pi pi-layer-group"></i>
      层选择
    </h3>
    <div class="layer-hint">
      <i class="pi pi-info-circle"></i>
      <span>按住 FN + 按键N 在键盘上切换层</span>
    </div>
    <div class="layer-legend">
      <div class="legend-item">
        <span class="legend-dot current-dot"></span>
        <span class="legend-text">当前层</span>
      </div>
      <div class="legend-item">
        <span class="legend-dot edit-dot"></span>
        <span class="legend-text">编辑层</span>
      </div>
    </div>
    <!-- 缩小版键盘布局 -->
    <div v-if="layout" class="layer-keyboard-mini" :style="{
      gridTemplateColumns: `repeat(${layout.cols}, 1fr)`,
      gridTemplateRows: `repeat(${layout.rows}, 1fr)`,
    }">
      <div v-for="key in layout.keys" :key="key.index" class="layer-key-mini" :class="{
        'current-layer': deviceStore.deviceStatus?.currentLayer === getLayerIndex(key.index),
        'edit-layer': deviceStore.currentEditLayer === getLayerIndex(key.index),
        [`key-${key.size}`]: true,
        'key-encoder-press': key.type === 'encoder-press',
        'disabled': isDisabled(key)
      }" :style="getKeyStyle(key)"
        @click="onKeyClick(key)"
        :title="getKeyTitle(key)">
        <span class="layer-key-number" v-if="!isDisabled(key)">{{ getLayerIndex(key.index) + 1 }}</span>
        <span class="layer-key-label" v-if="key.type !== 'encoder-press'">层{{ getLayerIndex(key.index) + 1 }}</span>
        <span class="layer-key-label encoder-label" v-else>🎚️</span>
      </div>
    </div>
    <div v-else class="layer-keyboard-mini-placeholder">
      <span>未连接设备</span>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import { useDeviceStore } from '@/stores/deviceStore';
import { getLayerLayoutByType, type LayoutDef } from '@/config/layouts';

const props = defineProps<{
  keyboardType: number;
}>();

const deviceStore = useDeviceStore();

const layout = computed<LayoutDef | null>(() => {
  return getLayerLayoutByType(props.keyboardType);
});

// 旋钮款按键索引→层索引的映射
const knobMapping: Record<number, number> = {
  6: 0, // 旋钮按下 -> 层 0
  0: 1, // 按键0 -> 层 1
  2: 2, // 按键2 -> 层 2
  1: 3, // 按键1 -> 层 3
  3: 4, // 按键3 -> 层 4
};

function getLayerIndex(keyIndex: number): number {
  if (props.keyboardType === 2) {
    return knobMapping[keyIndex] ?? 0;
  }
  return keyIndex;
}

function getKeyStyle(key: any) {
  return {
    gridRow: `${key.row + 1} / span ${key.size === '2u-v' ? 2 : 1}`,
    gridColumn: `${key.col + 1} / span ${key.size === '2u-h' ? 2 : 1}`,
  };
}

function isDisabled(key: any): boolean {
  return props.keyboardType === 2 && key.type === 'encoder-press';
}

function onKeyClick(key: any): void {
  if (isDisabled(key)) return;
  deviceStore.setEditLayer(getLayerIndex(key.index));
}

function getKeyTitle(key: any): string {
  if (isDisabled(key)) {
    return '旋钮位置无RGB，不可用作层切换';
  }
  const idx = getLayerIndex(key.index);
  return `层 ${idx + 1} - 点击编辑 | FN + ${idx + 1} 切换`;
}
</script>

<style scoped>
.layer-hint {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 0.75rem;
  color: var(--text-secondary);
  padding: 4px 0;
}

.layer-legend {
  display: flex;
  gap: 12px;
  margin-bottom: 8px;
}

.legend-item {
  display: flex;
  align-items: center;
  gap: 4px;
}

.legend-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
}

.current-dot {
  background: var(--green-400, #4ade80);
}

.edit-dot {
  background: var(--primary-color, #818cf8);
}

.legend-text {
  font-size: 0.75rem;
  color: var(--text-secondary);
}

.layer-keyboard-mini {
  display: grid;
  gap: 4px;
}

.layer-key-mini {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 4px 2px;
  border-radius: 6px;
  border: 2px solid var(--border-color);
  background: var(--surface-card);
  cursor: pointer;
  transition: all 0.15s ease;
  min-height: 36px;
  font-size: 0.75rem;
}

.layer-key-mini:hover:not(.disabled) {
  border-color: var(--primary-color);
  background: var(--surface-hover, rgba(129, 140, 248, 0.08));
}

.layer-key-mini.current-layer {
  border-color: var(--green-400, #4ade80);
  background: rgba(74, 222, 128, 0.1);
}

.layer-key-mini.edit-layer {
  border-color: var(--primary-color, #818cf8);
  background: rgba(129, 140, 248, 0.15);
}

.layer-key-mini.disabled {
  opacity: 0.3;
  cursor: not-allowed;
}

.layer-key-number {
  font-weight: 700;
  font-size: 0.9rem;
  color: var(--text-primary);
  line-height: 1;
}

.layer-key-label {
  font-size: 0.65rem;
  color: var(--text-secondary);
  line-height: 1;
}

.encoder-label {
  font-size: 0.8rem;
}

.layer-keyboard-mini-placeholder {
  text-align: center;
  padding: 12px;
  color: var(--text-secondary);
  font-size: 0.85rem;
}
</style>
