<template>
  <div class="panel layer-panel">
    <h3 class="panel-title">
      <i class="pi pi-layer-group"></i>
      层选择
    </h3>
    <div class="layer-hint">
      <i class="pi pi-info-circle"></i>
      <span>按住 {{ layerSwitchModifierLabel }} + 按键N 在键盘上切换层</span>
    </div>
    <div v-if="!previewMode" class="layer-legend">
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
      <div v-for="key in layout.keys" :key="`${key.index}-${key.type ?? ''}`" class="layer-key-mini" :class="{
        'current-layer': !previewMode && isSelectableLayer(key) && deviceStore.deviceStatus?.currentLayer === getLayerIndex(key.index),
        'edit-layer': !previewMode && isSelectableLayer(key) && deviceStore.currentEditLayer === getLayerIndex(key.index),
        [`key-${key.size}`]: true,
        'disabled': !isSelectableLayer(key),
        'readonly': !!previewMode,
        'encoder-placeholder': isEncoderPlaceholder(key),
      }" :style="getKeyStyle(key)"
        @click="onKeyClick(key)"
        :title="getKeyTitle(key)">
        <template v-if="isEncoderPlaceholder(key)">
          <svg class="mini-encoder-svg" viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg">
            <line x1="50" y1="0" x2="50" y2="50" class="mini-encoder-divider" stroke-width="2" />
            <line x1="93.3" y1="75" x2="50" y2="50" class="mini-encoder-divider" stroke-width="2" />
            <line x1="6.7" y1="75" x2="50" y2="50" class="mini-encoder-divider" stroke-width="2" />
          </svg>
        </template>
        <template v-else-if="!isSelectableLayer(key)">
          <span class="layer-key-number">{{ getLayerIndex(key.index) + 1 }}</span>
          <span class="layer-key-label">不可用</span>
        </template>
        <template v-else>
          <span class="layer-key-number">{{ getLayerIndex(key.index) + 1 }}</span>
          <span class="layer-key-label">层{{ getLayerIndex(key.index) + 1 }}</span>
        </template>
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
import { DeviceProtocol, KeyboardType, KeyboardTypeInfo } from '@/types/protocol';

const props = defineProps<{
  keyboardType: number;
  previewMode?: boolean;
}>();

const deviceStore = useDeviceStore();

const layout = computed<LayoutDef | null>(() => {
  return getLayerLayoutByType(props.keyboardType);
});

const availableLayerCount = computed(() => {
  if (props.previewMode) {
    return KeyboardTypeInfo[props.keyboardType as KeyboardType]?.layers ?? 1;
  }
  return deviceStore.keymap.numLayers;
});

const layerSwitchModifierLabel = computed(() => {
  return deviceStore.deviceInfo?.protocol === DeviceProtocol.CH552 ? 'FUNC' : 'BOOT';
});

// 旋钮款按键索引→层索引的映射 (仅物理按键，旋钮不参与层选择)
const knobMapping: Record<number, number> = {
  0: 0,
  1: 1,
  2: 2,
  3: 3,
};

function getLayerIndex(keyIndex: number): number {
  if (props.keyboardType === 2) {
    return knobMapping[keyIndex] ?? 0;
  }
  return keyIndex;
}

function getKeyStyle(key: LayoutDef['keys'][number]) {
  return {
    gridRow: `${key.row + 1} / span ${key.size === '2u-v' ? 2 : 1}`,
    gridColumn: `${key.col + 1} / span ${key.size === '2u-h' ? 2 : 1}`,
  };
}

function isSelectableLayer(key: LayoutDef['keys'][number]): boolean {
  if (isEncoderPlaceholder(key)) return false;
  const idx = getLayerIndex(key.index);
  return idx >= 0 && idx < availableLayerCount.value;
}

function isEncoderPlaceholder(key: LayoutDef['keys'][number]): boolean {
  return key.type === 'encoder-press';
}

function onKeyClick(key: LayoutDef['keys'][number]): void {
  if (props.previewMode) return;
  if (!isSelectableLayer(key)) return;
  deviceStore.setEditLayer(getLayerIndex(key.index));
}

function getKeyTitle(key: LayoutDef['keys'][number]): string {
  if (isEncoderPlaceholder(key)) {
    return '旋钮（不参与层选择）';
  }
  const idx = getLayerIndex(key.index);
  if (props.previewMode) {
    return `预览模式：层 ${idx + 1} 不可点击`;
  }
  if (idx >= availableLayerCount.value) {
    return `层 ${idx + 1} - 当前固件不支持`;
  }
  return `层 ${idx + 1} - 点击编辑 | ${layerSwitchModifierLabel.value} + ${idx + 1} 切换`;
}
</script>

<style scoped>
.layer-hint {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 0.75rem;
  color: var(--c-text-secondary);
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
  background: var(--c-accent, #818cf8);
}

.legend-text {
  font-size: 0.75rem;
  color: var(--c-text-secondary);
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
  border: 2px solid var(--c-border);
  background: var(--c-bg-tertiary);
  cursor: pointer;
  transition: all 0.15s ease;
  min-height: 36px;
  font-size: 0.75rem;
}

.layer-key-mini:hover:not(.disabled):not(.readonly) {
  border-color: var(--c-accent);
  background: var(--c-bg-hover);
}

.layer-key-mini.current-layer {
  border-color: var(--green-400, #4ade80);
  background: rgba(74, 222, 128, 0.1);
}

.layer-key-mini.edit-layer {
  border-color: var(--c-accent, #818cf8);
  background: rgba(129, 140, 248, 0.15);
}

.layer-key-mini.disabled {
  opacity: 0.35;
  cursor: not-allowed;
}

.layer-key-mini.readonly {
  cursor: default;
}

.layer-key-mini.encoder-placeholder {
  border-radius: 50%;
  aspect-ratio: 1;
  position: relative;
  overflow: hidden;
}

.layer-key-number {
  font-weight: 700;
  font-size: 0.9rem;
  color: var(--c-text-primary);
  line-height: 1;
}

.layer-key-label {
  font-size: 0.65rem;
  color: var(--c-text-secondary);
  line-height: 1;
}

.mini-encoder-svg {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
}

.mini-encoder-divider {
  stroke: var(--c-border);
  opacity: 0.6;
}

.layer-keyboard-mini-placeholder {
  text-align: center;
  padding: 12px;
  color: var(--c-text-secondary);
  font-size: 0.85rem;
}
</style>
