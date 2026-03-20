<script setup lang="ts">
import { ref, computed } from 'vue';
import { useDeviceStore } from '@/stores/deviceStore';
import { useTheme } from '@/composables/useTheme';
import { KeyboardType, KeyboardTypeInfo } from '@/types/protocol';
import { showToast } from '@/services/toastService';
import CatEmoji from '@/components/CatEmoji.vue';
import Button from 'primevue/button';

const deviceStore = useDeviceStore();
const { toggleMode, themeMode } = useTheme();

const previewKeyboardType = defineModel<number>('previewType', { default: -1 });

const emit = defineEmits<{
  (e: 'refresh'): void;
  (e: 'disconnect'): void;
}>();

const currentKeyboardType = computed(() => {
  if (previewKeyboardType.value >= 0) return previewKeyboardType.value;
  return deviceStore.deviceInfo?.keyboardType ?? 0;
});

function onPreviewTypeChange() {
  if (previewKeyboardType.value >= 0) {
    deviceStore.setEditLayer(0);
    showToast('info', '预览模式', `正在预览 ${KeyboardTypeInfo[previewKeyboardType.value as KeyboardType]?.name || '未知型号'}`);
  } else {
    if (deviceStore.deviceInfo) {
      deviceStore.setEditLayer(deviceStore.keymap.currentLayer);
    }
  }
}
</script>

<template>
  <header class="app-header">
    <div class="header-left">
      <span class="header-logo"><CatEmoji /></span>
      <span class="header-title">BinaryKeyboard</span>
    </div>

    <div class="header-center">
      <div class="device-badge" :class="{ 'preview-mode': previewKeyboardType >= 0 }">
        <i :class="previewKeyboardType >= 0 ? 'pi pi-eye' : 'pi pi-check-circle connected-icon'"></i>
        <span v-if="previewKeyboardType >= 0">
          预览模式 - {{ KeyboardTypeInfo[currentKeyboardType as KeyboardType]?.name || '未知型号' }}
        </span>
        <span v-else>
          {{ deviceStore.device?.productName }} - {{ deviceStore.keyboardTypeName }}
        </span>
      </div>
    </div>

    <div class="header-right">
      <div class="preview-mode-selector">
        <select v-model="previewKeyboardType" @change="onPreviewTypeChange" class="preview-select"
          v-tooltip.bottom="'预览不同键盘型号的布局'">
          <option :value="-1">实际设备</option>
          <option :value="0">预览：基础款</option>
          <option :value="1">预览：五键款</option>
          <option :value="2">预览：旋钮款</option>
        </select>
      </div>
      <button class="theme-toggle" @click="toggleMode" v-tooltip.bottom="'切换主题'">
        <i :class="themeMode === 'dark' ? 'pi pi-sun' : 'pi pi-moon'"></i>
      </button>
      <Button icon="pi pi-sync" severity="secondary" text rounded v-tooltip.bottom="'刷新配置'" @click="emit('refresh')"
        class="header-btn btn-icon-secondary" />
      <Button icon="pi pi-power-off" severity="danger" text rounded v-tooltip.bottom="'断开连接'" @click="emit('disconnect')"
        class="header-btn btn-icon-danger" />
    </div>
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
  letter-spacing: -0.02em;
}

.header-center {
  flex: 1;
  display: flex;
  justify-content: center;
}

.device-badge {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  padding: 0.35rem 1rem;
  border-radius: 999px;
  border: 1px solid var(--c-border);
  font-size: 0.85rem;
  font-weight: 600;
  color: var(--c-text-secondary);
  background: var(--c-bg-primary);
}

.device-badge.preview-mode {
  border-color: var(--c-accent);
  color: var(--c-accent);
}

.connected-icon {
  color: var(--c-success);
}

.header-right {
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.header-btn {
  width: 36px !important;
  height: 36px !important;
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
  transition: color var(--transition-fast);
}

.theme-toggle:hover {
  color: var(--c-accent);
}

.preview-mode-selector {
  display: flex;
  align-items: center;
}

.preview-select {
  padding: 0.35rem 0.75rem;
  font-size: 0.8rem;
  font-weight: 600;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  background: var(--c-bg-tertiary);
  color: var(--c-text-secondary);
  cursor: pointer;
  transition: all var(--transition-fast);
  outline: none;
}

.preview-select:hover {
  border-color: var(--c-accent);
}

.preview-select:focus {
  border-color: var(--c-accent);
  box-shadow: 0 0 0 2px var(--c-accent-soft);
}

[data-theme="light"] .preview-select {
  background: #ffffff;
  border-color: #cbd5e1;
}
</style>
