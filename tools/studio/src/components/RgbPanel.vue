<template>
  <div class="panel rgb-panel">
    <h3 class="panel-title">
      <i class="pi pi-palette"></i>
      RGB 灯效
    </h3>
    <div class="rgb-config">
      <div class="rgb-item">
        <span class="rgb-label">RGB 开关（仅按键灯）</span>
        <label class="rgb-switch">
          <input type="checkbox" v-model="deviceStore.rgbConfig.enabled" />
          <span>{{ deviceStore.rgbConfig.enabled ? '开启' : '关闭' }}</span>
        </label>
      </div>
      <div class="rgb-item">
        <span class="rgb-label">模式</span>
        <select v-model="deviceStore.rgbConfig.mode" class="rgb-select">
          <option :value="0">关闭</option>
          <option :value="1">静态</option>
          <option :value="2">呼吸</option>
          <option :value="3">闪烁</option>
          <option :value="4">彩虹</option>
          <option :value="5">仅指示灯</option>
        </select>
      </div>
      <div v-if="showColorPicker" class="rgb-item rgb-color-row">
        <span class="rgb-label">颜色</span>
        <div class="rgb-color-controls">
          <ColorPicker
            v-model="colorHex"
            format="hex"
            inline
            class="rgb-color-picker"
          />
          <input
            v-model="colorHex"
            type="text"
            class="rgb-hex-input"
            placeholder="#ffffff"
            maxlength="7"
          />
        </div>
      </div>
      <div class="rgb-item">
        <span class="rgb-label">按键灯亮度 {{ Math.round(deviceStore.rgbConfig.brightness * 100 / 255) }}%</span>
        <input type="range" v-model.number="deviceStore.rgbConfig.brightness" min="0" max="255" class="rgb-slider" />
      </div>
      <div class="rgb-item">
        <span class="rgb-label">指示灯亮度 {{ Math.round(deviceStore.rgbConfig.indicatorBrightness * 100 / 255) }}%</span>
        <input type="range" v-model.number="deviceStore.rgbConfig.indicatorBrightness" :min="RGB_INDICATOR_MIN_BRIGHTNESS" max="255" class="rgb-slider" />
      </div>
      <Button label="保存 RGB" icon="pi pi-check" size="small" @click="saveRgb"
        class="rgb-save-btn btn-primary" />
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import { useDeviceStore } from '@/stores/deviceStore';
import { showToast } from '@/services/toastService';
import { rgbToHex, hexToRgb, RGB_INDICATOR_MIN_BRIGHTNESS } from '@/types/protocol';

const deviceStore = useDeviceStore();

const showColorPicker = computed(
  () =>
    deviceStore.supportsRgb &&
    (
      deviceStore.rgbConfig.mode === 1 ||
      deviceStore.rgbConfig.mode === 2 ||
      deviceStore.rgbConfig.mode === 3
    ),
);

const colorHex = computed({
  get: () =>
    rgbToHex(
      deviceStore.rgbConfig.colorR,
      deviceStore.rgbConfig.colorG,
      deviceStore.rgbConfig.colorB,
    ),
  set: (hex: string) => {
    const rgb = hexToRgb(hex);
    if (rgb) {
      deviceStore.rgbConfig.colorR = rgb.r;
      deviceStore.rgbConfig.colorG = rgb.g;
      deviceStore.rgbConfig.colorB = rgb.b;
    }
  },
});

async function saveRgb() {
  try {
    await deviceStore.saveRgbConfig();
    showToast('success', 'RGB 已保存', 'RGB 灯效配置已保存到设备');
  } catch (error) {
    showToast('error', '保存失败', error instanceof Error ? error.message : '未知错误');
  }
}
</script>

<style scoped>
.rgb-config {
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.rgb-item {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.rgb-label {
  font-size: 0.85rem;
  color: var(--text-secondary);
}

.rgb-switch {
  display: flex;
  align-items: center;
  gap: 8px;
  cursor: pointer;
  font-size: 0.85rem;
}

.rgb-select {
  padding: 6px 8px;
  border-radius: 6px;
  border: 1px solid var(--border-color);
  background: var(--surface-card);
  color: var(--text-primary);
  font-size: 0.85rem;
}

.rgb-slider {
  width: 100%;
  accent-color: var(--primary-color);
}

.rgb-color-row {
  gap: 6px;
}

.rgb-color-controls {
  display: flex;
  align-items: flex-start;
  gap: 10px;
}

.rgb-color-picker {
  flex-shrink: 0;
}

.rgb-hex-input {
  width: 80px;
  padding: 6px 8px;
  border-radius: 6px;
  border: 1px solid var(--border-color);
  background: var(--surface-card);
  color: var(--text-primary);
  font-family: 'JetBrains Mono', 'Fira Code', monospace;
  font-size: 0.8rem;
}

.rgb-save-btn {
  margin-top: 4px;
}
</style>
