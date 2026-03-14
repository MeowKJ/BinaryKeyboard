<template>
  <div class="panel rgb-panel">
    <h3 class="panel-title">
      <i class="pi pi-palette"></i>
      RGB 灯效
    </h3>
    <div class="rgb-config">
      <div class="rgb-item">
        <span class="rgb-label">RGB 开关</span>
        <label class="rgb-switch">
          <input type="checkbox" v-model="deviceStore.rgbConfig.enabled" />
          <span>{{ deviceStore.rgbConfig.enabled ? '开启' : '关闭' }}</span>
        </label>
      </div>

      <div class="rgb-item">
        <span class="rgb-label">模式</span>
        <select v-model="deviceStore.rgbConfig.mode" class="rgb-select">
          <option v-for="mode in modeOptions" :key="mode.value" :value="mode.value">
            {{ mode.label }}
          </option>
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

      <div v-if="showIndicatorBrightness" class="rgb-item">
        <span class="rgb-label">指示灯亮度 {{ Math.round(deviceStore.rgbConfig.indicatorBrightness * 100 / 255) }}%</span>
        <input type="range" v-model.number="deviceStore.rgbConfig.indicatorBrightness" :min="RGB_INDICATOR_MIN_BRIGHTNESS" max="255" class="rgb-slider" />
      </div>

      <div class="rgb-item">
        <span class="rgb-label">按下效果</span>
        <select v-model.number="deviceStore.rgbConfig.pressEffect" class="rgb-select">
          <option v-for="opt in pressEffectOptions" :key="opt.value" :value="opt.value">
            {{ opt.label }}
          </option>
        </select>
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
import {
  DeviceProtocol,
  PressEffect,
  RgbMode,
  hexToRgb,
  rgbToHex,
  RGB_INDICATOR_MIN_BRIGHTNESS,
} from '@/types/protocol';

const deviceStore = useDeviceStore();

const modeOptions = computed(() => {
  if (deviceStore.deviceInfo?.protocol === DeviceProtocol.CH552) {
    return [
      { value: RgbMode.OFF, label: '关闭' },
      { value: RgbMode.STATIC, label: '静态' },
      { value: RgbMode.BREATHING, label: '呼吸' },
      { value: RgbMode.BLINK, label: '闪烁' },
      { value: RgbMode.RAINBOW, label: '彩虹' },
    ];
  }

  return [
    { value: RgbMode.OFF, label: '关闭' },
    { value: RgbMode.STATIC, label: '静态' },
    { value: RgbMode.BREATHING, label: '呼吸' },
    { value: RgbMode.BLINK, label: '闪烁' },
    { value: RgbMode.RAINBOW, label: '彩虹' },
    { value: RgbMode.INDICATOR, label: '仅指示灯' },
  ];
});

const showColorPicker = computed(
  () =>
    deviceStore.supportsRgb &&
    (
      deviceStore.rgbConfig.mode === RgbMode.STATIC ||
      deviceStore.rgbConfig.mode === RgbMode.BREATHING ||
      deviceStore.rgbConfig.mode === RgbMode.BLINK
    ),
);

const showIndicatorBrightness = computed(
  () => deviceStore.deviceInfo?.protocol === DeviceProtocol.CH592,
);

const pressEffectOptions = [
  { value: PressEffect.NONE, label: '无' },
  { value: PressEffect.PRESS_LIGHT_FADE, label: '按下亮起渐灭' },
  { value: PressEffect.PRESS_DARK_FADE, label: '按下熄灭渐亮' },
];

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
  color: var(--c-text-secondary);
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
  border: 1px solid var(--c-border);
  background: var(--c-bg-tertiary);
  color: var(--c-text-primary);
  font-size: 0.85rem;
}

.rgb-slider {
  width: 100%;
  accent-color: var(--c-accent);
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
  border: 1px solid var(--c-border);
  background: var(--c-bg-tertiary);
  color: var(--c-text-primary);
  font-family: 'JetBrains Mono', 'Fira Code', monospace;
  font-size: 0.8rem;
}

.rgb-subsection {
  display: flex;
  flex-direction: column;
  gap: 8px;
  padding-top: 4px;
}

.rgb-checkbox {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 0.85rem;
  cursor: pointer;
  color: var(--c-text-primary);
}

.rgb-save-btn {
  margin-top: 4px;
}
</style>
