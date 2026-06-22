<script setup lang="ts">
import { computed, ref } from 'vue';
import { useDeviceStore } from '@/stores/deviceStore';
import { useReleaseStore } from '@/stores/releaseStore';
import { useTheme } from '@/composables/useTheme';
import { useFirmwareUpdateInfo } from '@/composables/useFirmwareUpdateInfo';
import { DeviceProtocol, KeyboardType, KeyboardTypeInfo } from '@/types/protocol';
import CatEmoji from '@/components/CatEmoji.vue';
import FirmwareUpdateDialog from '@/components/FirmwareUpdateDialog.vue';
import Button from 'primevue/button';

const deviceStore = useDeviceStore();
const releaseStore = useReleaseStore();
const { toggleMode, themeMode, canToggleMode } = useTheme();
const firmwareDialogVisible = ref(false);

const previewKeyboardType = defineModel<number>('previewType', { default: -1 });

const emit = defineEmits<{
  (e: 'refresh'): void;
  (e: 'disconnect'): void;
  (e: 'open-device-info'): void;
}>();

const currentKeyboardType = computed(() => {
  if (previewKeyboardType.value >= 0) return previewKeyboardType.value;
  return deviceStore.deviceInfo?.keyboardType ?? 0;
});

const deviceDisplayName = computed(() => {
  const device = deviceStore.device;
  if (!device) return '未连接';
  return device.productName;
});

const {
  latestFirmwareVersion,
  firmwareUpdateKind,
  firmwareUpdateAvailable,
  firmwareUpdateTooltip,
} = useFirmwareUpdateInfo({ disabled: computed(() => previewKeyboardType.value >= 0) });

const firmwareUpdateLabel = computed(() => (
  firmwareUpdateKind.value === 'dev-to-release' ? '刷正式版' : '更新固件'
));

function openFirmwareUpdate() {
  if (deviceStore.deviceInfo?.protocol === DeviceProtocol.CH552) {
    window.open(`https://github.com/${releaseStore.repository}/releases/latest`, '_blank', 'noopener');
    return;
  }
  if (deviceStore.capabilities.iap) {
    firmwareDialogVisible.value = true;
    return;
  }
  window.open(`https://github.com/${releaseStore.repository}/releases/latest`, '_blank', 'noopener');
}

</script>

<template>
  <header class="app-header">
    <div class="header-left">
      <span class="header-logo"><CatEmoji /></span>
      <span class="header-title">BinaryKeyboard</span>
    </div>

    <div class="header-center">
      <button
        type="button"
        class="device-badge"
        :class="{ 'preview-mode': previewKeyboardType >= 0 }"
        title="设备信息"
        @click="emit('open-device-info')"
      >
        <i :class="previewKeyboardType >= 0 ? 'pi pi-eye' : 'pi pi-check-circle connected-icon'"></i>
        <span v-if="previewKeyboardType >= 0">
          预览 - {{ KeyboardTypeInfo[currentKeyboardType as KeyboardType]?.name || '未知型号' }}
        </span>
        <span v-else>{{ deviceDisplayName }} · {{ deviceStore.keyboardTypeName }}</span>
        <span v-if="previewKeyboardType < 0" class="version-chip" :class="{ dev: deviceStore.isDevFirmware }">
          {{ deviceStore.firmwareVersionLabel }}
        </span>
      </button>
    </div>

    <div class="header-right">
      <button
        v-if="firmwareUpdateAvailable"
        type="button"
        v-tooltip.bottom="firmwareUpdateTooltip"
        class="firmware-download-btn"
        :class="{ dev: firmwareUpdateKind === 'dev-to-release' }"
        @click="openFirmwareUpdate"
      >
        <i class="pi pi-download"></i>
        <span class="firmware-label">{{ firmwareUpdateLabel }}</span>
        <code>v{{ latestFirmwareVersion }}</code>
      </button>
      <button v-if="canToggleMode" class="theme-toggle" @click="toggleMode" v-tooltip.bottom="'切换明暗模式'">
        <i :class="themeMode === 'dark' ? 'pi pi-sun' : 'pi pi-moon'"></i>
      </button>
      <Button
        icon="pi pi-sync"
        severity="secondary"
        text
        rounded
        v-tooltip.bottom="'刷新配置'"
        class="header-btn btn-icon-secondary"
        @click="emit('refresh')"
      />
      <Button
        icon="pi pi-power-off"
        severity="danger"
        text
        rounded
        v-tooltip.bottom="'断开连接'"
        class="header-btn btn-icon-danger"
        @click="emit('disconnect')"
      />
    </div>
    <FirmwareUpdateDialog
      v-model:visible="firmwareDialogVisible"
      :target-version="latestFirmwareVersion"
    />
  </header>
</template>

<style scoped>
.app-header {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  height: var(--header-height);
  padding: 0 1.5rem;
  display: flex;
  align-items: center;
  border-bottom: 1px solid var(--c-border);
  background: var(--c-bg-secondary);
  flex-shrink: 0;
  z-index: 50;
}

.header-left {
  display: flex;
  align-items: center;
  gap: 0.75rem;
}

.header-logo {
  font-size: 1.5rem;
}

.header-title {
  font-size: 1.1rem;
  font-weight: 800;
  color: var(--c-text-primary);
}

.header-center {
  position: absolute;
  left: 50%;
  top: 50%;
  display: flex;
  justify-content: center;
  transform: translate(-50%, -50%);
  z-index: 1;
}

.device-badge {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  max-width: min(46vw, 560px);
  padding: 0.35rem 0.95rem;
  border-radius: 999px;
  border: 1px solid var(--c-border);
  font: inherit;
  font-size: 0.85rem;
  font-weight: 700;
  color: var(--c-text-secondary);
  background: var(--c-bg-primary);
  cursor: pointer;
  transition: border-color 0.16s ease, background 0.16s ease, color 0.16s ease, transform 0.16s ease;
}

.device-badge span {
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.device-badge:hover,
.device-badge:focus-visible {
  border-color: var(--c-accent);
  color: var(--c-text-primary);
  background: var(--c-bg-hover);
  outline: none;
  transform: translateY(-1px);
}

.device-badge.preview-mode {
  border-color: var(--c-accent);
  color: var(--c-accent-light);
}

.connected-icon {
  color: var(--c-success);
}

.version-chip {
  flex-shrink: 0;
  display: inline-flex;
  align-items: center;
  min-height: 1.28rem;
  padding: 0.1rem 0.48rem;
  border-radius: 999px;
  border: 1px solid color-mix(in srgb, var(--c-accent) 56%, transparent);
  background: color-mix(in srgb, var(--c-accent) 16%, var(--c-bg-tertiary));
  color: color-mix(in srgb, var(--c-accent-light) 86%, var(--c-text-primary));
  font-family: 'JetBrains Mono', 'SFMono-Regular', Consolas, monospace;
  font-size: 0.76rem;
  font-weight: 950;
  line-height: 1;
  letter-spacing: 0.02em;
}

.version-chip.dev {
  border-color: color-mix(in srgb, var(--c-warning) 62%, transparent);
  background: color-mix(in srgb, var(--c-warning) 15%, var(--c-bg-tertiary));
  color: color-mix(in srgb, var(--c-warning) 88%, var(--c-text-primary));
}

.header-right {
  margin-left: auto;
  display: flex;
  align-items: center;
  gap: 0.5rem;
  z-index: 2;
}

.header-btn {
  width: 36px !important;
  height: 36px !important;
}

.firmware-download-btn {
  display: inline-flex;
  align-items: center;
  gap: 0.38rem;
  min-width: 8.1rem;
  height: 36px;
  padding: 0 0.78rem;
  border: 1px solid color-mix(in srgb, var(--c-success) 56%, var(--c-border));
  border-radius: 999px;
  background: var(--c-bg-tertiary);
  color: var(--c-success);
  font: inherit;
  font-size: 0.74rem;
  font-weight: 950;
  cursor: pointer;
  transition: transform 0.16s ease, border-color 0.16s ease, color 0.16s ease, background 0.16s ease;
}

.firmware-download-btn.dev {
  border-color: color-mix(in srgb, var(--c-warning) 64%, var(--c-border));
  color: var(--c-warning);
}

.firmware-download-btn:hover,
.firmware-download-btn:focus-visible {
  transform: translateY(-1px);
  border-color: color-mix(in srgb, var(--c-success) 78%, var(--c-border));
  background: var(--c-bg-hover);
  color: color-mix(in srgb, var(--c-success) 82%, var(--c-text-primary));
  outline: none;
}

.firmware-download-btn.dev:hover,
.firmware-download-btn.dev:focus-visible {
  border-color: color-mix(in srgb, var(--c-warning) 78%, var(--c-border));
  color: color-mix(in srgb, var(--c-warning) 82%, var(--c-text-primary));
}

.firmware-download-btn i {
  font-size: 0.82rem;
}

.firmware-download-btn code {
  font-family: 'JetBrains Mono', 'SFMono-Regular', Consolas, monospace;
  font-size: 0.66rem;
  font-weight: 950;
  color: currentColor;
  opacity: 0.88;
}

.firmware-label {
  line-height: 1;
}

.theme-toggle {
  width: 36px;
  height: 36px;
  border-radius: 50%;
  border: none;
  background: none;
  color: var(--c-text-muted);
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: color var(--transition-fast), background var(--transition-fast);
}

.theme-toggle:hover {
  color: var(--c-accent);
  background: var(--c-bg-hover);
}

@media (max-width: 1120px) {
  .firmware-download-btn {
    min-width: 6.6rem;
    padding: 0 0.62rem;
  }

  .firmware-download-btn code {
    display: none;
  }
}

@media (max-width: 860px) {
  .firmware-download-btn {
    min-width: 0;
    width: 36px;
    padding: 0;
    justify-content: center;
  }

  .firmware-download-btn .firmware-label {
    display: none;
  }
}

</style>
