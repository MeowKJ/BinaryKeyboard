<template>
  <div class="panel layer-panel">
    <div class="layer-panel-head">
      <h3 class="panel-title layer-title">
        <i class="pi pi-clone"></i>
        <span>层选择</span>
      </h3>
      <div class="layer-head-tools">
        <span v-if="!previewMode" class="layer-inline-readout" aria-label="当前层和编辑层">
          <span>当前 <strong class="current">{{ currentLayerNumber }}</strong></span>
          <span>编辑 <strong class="edit">{{ editLayerNumber }}</strong></span>
        </span>
        <button
          type="button"
          class="layer-help-btn studio-help-link"
          v-tooltip.left="layerSwitchHint"
          aria-label="层切换提示"
        >
          <i class="pi pi-question-circle"></i>
        </button>
      </div>
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

    <div
      v-if="layout"
      class="layer-keyboard-mini"
      :style="{
        gridTemplateColumns: `repeat(${layout.cols}, 1fr)`,
        gridTemplateRows: `repeat(${layout.rows}, 1fr)`,
      }"
    >
      <div
        v-for="key in layout.keys"
        :key="`${key.index}-${key.type ?? ''}`"
        class="layer-key-mini"
        :class="{
          'current-layer': !previewMode && isSelectableLayer(key) && deviceStore.deviceStatus?.currentLayer === getLayerIndex(key.index),
          'edit-layer': !previewMode && isSelectableLayer(key) && deviceStore.currentEditLayer === getLayerIndex(key.index),
          [`key-${key.size}`]: true,
          disabled: !isSelectableLayer(key),
          readonly: !!previewMode,
          'encoder-placeholder': isEncoderPlaceholder(key),
        }"
        :style="getKeyStyle(key)"
        :title="getKeyTitle(key)"
        @click="onKeyClick(key)"
      >
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
          <span class="layer-key-label">层 {{ getLayerIndex(key.index) + 1 }}</span>
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

const currentLayerNumber = computed(() => {
  return (deviceStore.deviceStatus?.currentLayer ?? deviceStore.keymap.currentLayer) + 1;
});

const editLayerNumber = computed(() => deviceStore.currentEditLayer + 1);

const layerSwitchHint = computed(() => {
  return `按住 ${layerSwitchModifierLabel.value} + 按键 N，可在键盘上切换到对应层。`;
});

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
    return '旋钮，不参与层选择';
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
.layer-panel-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 0.55rem;
  margin-bottom: 0.55rem;
}

.layer-title {
  margin: 0;
  min-width: 0;
}

.layer-title i {
  color: var(--c-accent);
}

.layer-head-tools {
  display: inline-flex;
  align-items: center;
  gap: 0.35rem;
  min-width: 0;
}

.layer-inline-readout {
  display: inline-flex;
  align-items: center;
  gap: 0.48rem;
  min-height: 1.55rem;
  padding: 0 0.52rem;
  border: 1px solid var(--c-border-light);
  border-radius: 999px;
  background: color-mix(in srgb, var(--c-bg-tertiary) 86%, transparent);
  color: var(--c-text-muted);
  font-size: 0.62rem;
  font-weight: 900;
  white-space: nowrap;
}

.layer-inline-readout strong {
  font-family: 'JetBrains Mono', 'SFMono-Regular', Consolas, monospace;
  font-size: 0.74rem;
}

.layer-inline-readout strong.current {
  color: #22c55e;
}

.layer-inline-readout strong.edit {
  color: #f59e0b;
}

.layer-help-btn {
  flex: 0 0 auto;
}

.layer-legend {
  display: flex;
  justify-content: center;
  gap: 0.85rem;
  padding: 0.42rem 0.55rem;
  margin-bottom: 0.6rem;
  border: 1px solid var(--c-border-light);
  border-radius: var(--radius-sm);
  background: color-mix(in srgb, var(--c-bg-tertiary) 82%, transparent);
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
  flex: 0 0 auto;
}

.current-dot {
  background: #22c55e;
  box-shadow: 0 0 8px rgba(34, 197, 94, 0.48);
}

.edit-dot {
  background: #f59e0b;
  box-shadow: 0 0 8px rgba(245, 158, 11, 0.48);
}

.legend-text {
  font-size: 0.68rem;
  color: var(--c-text-secondary);
  font-weight: 800;
}

.layer-keyboard-mini {
  display: grid;
  gap: 0.3rem;
  width: 100%;
  max-width: 204px;
  margin: 0 auto;
  padding: 0.55rem;
  border: 1px solid var(--c-border-light);
  border-radius: var(--radius-sm);
  background:
    linear-gradient(135deg, color-mix(in srgb, var(--c-bg-tertiary) 88%, var(--c-accent) 6%), var(--c-bg-tertiary)),
    repeating-linear-gradient(90deg, rgba(255, 255, 255, 0.028) 0 1px, transparent 1px 12px);
  box-shadow: inset 0 0 0 1px rgba(255, 255, 255, 0.025);
}

.layer-key-mini {
  position: relative;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  min-width: 30px;
  min-height: 30px;
  padding: 4px 2px 3px;
  border: 1.5px solid var(--c-border);
  border-radius: calc(var(--radius-sm) * 0.8);
  background: var(--c-bg-secondary);
  cursor: pointer;
  transition: transform 0.15s ease, border-color 0.15s ease, box-shadow 0.15s ease, background 0.15s ease;
  font-size: 0.75rem;
  overflow: hidden;
}

.layer-key-mini.key-1u {
  aspect-ratio: 1;
}

.layer-key-mini.key-2u-h {
  aspect-ratio: 2;
}

.layer-key-mini.key-2u-v {
  min-height: calc(2 * 30px + 0.3rem);
}

.layer-key-mini:hover:not(.disabled):not(.readonly) {
  border-color: var(--c-accent);
  background: var(--c-bg-hover);
  box-shadow: 0 2px 8px var(--c-key-shadow);
  transform: translateY(-1px);
}

.layer-key-mini.current-layer {
  border-color: #22c55e;
  background: rgba(34, 197, 94, 0.1);
  box-shadow: 0 0 0 1.5px rgba(34, 197, 94, 0.14), 0 0 12px rgba(34, 197, 94, 0.14);
}

.layer-key-mini.edit-layer {
  border-color: #f59e0b;
  background: rgba(245, 158, 11, 0.1);
  box-shadow: 0 0 0 1.5px rgba(245, 158, 11, 0.14), 0 0 12px rgba(245, 158, 11, 0.13);
}

.layer-key-mini.current-layer.edit-layer {
  border-color: transparent;
  background:
    linear-gradient(var(--c-bg-secondary), var(--c-bg-secondary)) padding-box,
    linear-gradient(135deg, #22c55e 0%, #f59e0b 100%) border-box;
  box-shadow:
    0 0 0 1.5px rgba(255, 255, 255, 0.03),
    0 0 14px rgba(34, 197, 94, 0.12),
    0 0 14px rgba(245, 158, 11, 0.12);
}

.layer-key-mini.disabled {
  opacity: 0.35;
  cursor: not-allowed;
  background: var(--c-bg-tertiary) !important;
  border-color: var(--c-border-light) !important;
  box-shadow: none !important;
}

.layer-key-mini.readonly {
  cursor: default;
}

.layer-key-mini.encoder-placeholder {
  border-radius: 50%;
  aspect-ratio: 1;
}

.layer-key-mini.encoder-placeholder.current-layer.edit-layer {
  border-radius: 50%;
}

.layer-key-number {
  font-weight: 800;
  font-size: 0.9rem;
  color: var(--c-accent);
  line-height: 1;
}

.layer-key-label {
  margin-top: 0.14rem;
  font-size: 0.54rem;
  color: var(--c-text-muted);
  font-weight: 800;
  line-height: 1;
}

.layer-key-mini.current-layer .layer-key-number,
.layer-key-mini.current-layer .layer-key-label {
  color: #22c55e;
}

.layer-key-mini.edit-layer .layer-key-number,
.layer-key-mini.edit-layer .layer-key-label {
  color: #f59e0b;
}

.layer-key-mini.current-layer.edit-layer .layer-key-number,
.layer-key-mini.current-layer.edit-layer .layer-key-label {
  background: linear-gradient(135deg, #22c55e 0%, #f59e0b 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
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

:global([data-theme="light"]) .layer-inline-readout,
:global([data-theme="light"]) .layer-legend,
:global([data-theme="light"]) .layer-keyboard-mini {
  background: #f8fafc;
  border-color: #e2e8f0;
}

:global([data-theme="light"]) .layer-key-mini {
  background: #ffffff;
  border-color: #cbd5e1;
}

:global([data-theme="light"]) .layer-key-mini.current-layer {
  border-color: #16a34a;
  background: rgba(34, 197, 94, 0.15);
}

:global([data-theme="light"]) .layer-key-mini.edit-layer {
  border-color: #d97706;
  background: rgba(245, 158, 11, 0.15);
}

:global([data-theme="light"]) .layer-key-mini.current-layer .layer-key-number,
:global([data-theme="light"]) .layer-key-mini.current-layer .layer-key-label {
  color: #16a34a;
}

:global([data-theme="light"]) .layer-key-mini.edit-layer .layer-key-number,
:global([data-theme="light"]) .layer-key-mini.edit-layer .layer-key-label {
  color: #d97706;
}

@media (max-width: 768px) {
  .layer-keyboard-mini {
    max-width: 164px;
    gap: 0.24rem;
    padding: 0.42rem;
  }

  .layer-key-mini {
    min-width: 27px;
    min-height: 27px;
  }

  .layer-key-number {
    font-size: 0.8rem;
  }

  .layer-key-label {
    font-size: 0.48rem;
  }
}
</style>
