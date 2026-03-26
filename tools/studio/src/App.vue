<template>
  <div class="app-root" :data-theme="themeMode" :data-theme-id="themeId">
    <Toast position="top-center" />

    <!-- PWA 新版本提示 -->
    <div v-if="pwaNeedRefresh" class="pwa-update-banner">
      <span>发现新版本，点击更新</span>
      <button class="pwa-update-btn" @click="onPwaUpdate">更新</button>
    </div>

    <!-- Electron 新版本提示 -->
    <div v-else-if="isElectron && releaseStore.studioUpdateAvailable" class="pwa-update-banner">
      <span>发现新版本 v{{ releaseStore.latestStudioVersion }}</span>
      <a class="pwa-update-btn" :href="`https://github.com/${releaseStore.repository}/releases/latest`" target="_blank" rel="noopener">下载</a>
    </div>

    <!-- 加载遮罩 -->
    <div v-if="deviceStore.isLoading && viewPhase === 'connected'" class="loading-overlay">
      <ProgressSpinner strokeWidth="4" />
      <span class="loading-text">正在通讯...</span>
    </div>

    <!-- 未连接状态 -->
    <WelcomeView
      v-if="viewPhase !== 'connected'"
      :connecting="viewPhase === 'connecting'"
      :welcome-returning="welcomeReturning"
      @connect="requestDevice"
    />

    <!-- 已连接状态 -->
    <ConnectedView
      v-if="viewPhase === 'connected'"
      :on-refresh="refreshAll"
      :on-disconnect="disconnect"
      :on-toggle-theme="toggleMode"
    />

    <!-- 确认对话框 -->
    <ConfirmDialog />

    <!-- 调试终端 -->
    <DebugTerminal />
  </div>
</template>

<script setup lang="ts">
import { onMounted, onUnmounted } from 'vue';
import { useToast } from 'primevue/usetoast';
import { useDeviceStore } from '@/stores/deviceStore';
import { useRegisterSW } from 'virtual:pwa-register/vue';
import { HidService } from '@/services/HidService';
import { initToastService, showToast } from '@/services/toastService';
import { useReleaseStore } from '@/stores/releaseStore';
import { useConnection } from '@/composables/useConnection';
import { useTheme } from '@/composables/useTheme';

import Toast from 'primevue/toast';
import ConfirmDialog from 'primevue/confirmdialog';
import ProgressSpinner from 'primevue/progressspinner';
import WelcomeView from '@/views/WelcomeView.vue';
import ConnectedView from '@/views/ConnectedView.vue';
import DebugTerminal from '@/components/DebugTerminal.vue';

// 初始化服务
const toast = useToast();
initToastService(toast);
const deviceStore = useDeviceStore();
const releaseStore = useReleaseStore();
const isElectron = navigator.userAgent.includes('Electron');

// 连接状态
const {
  viewPhase,
  welcomeReturning,
  requestDevice,
  autoConnect,
  disconnect,
  refreshAll,
  setupHidListeners,
  teardownHidListeners,
} = useConnection();

// 主题系统
const { themeId, themeMode, toggleMode, init: initTheme } = useTheme();

// PWA
const { needRefresh: pwaNeedRefresh, updateServiceWorker } = useRegisterSW({
  onRegistered(r: ServiceWorkerRegistration | undefined) {
    if (r) r.update();
  },
});

async function onPwaUpdate() {
  try {
    await updateServiceWorker();
  } catch (error) {
    showToast('error', '更新失败', error instanceof Error ? error.message : '无法应用新版本');
  }
}

// 生命周期
onMounted(async () => {
  initTheme();
  void releaseStore.loadLatestVersions();
  if (HidService.isSupported()) {
    setupHidListeners();
    await autoConnect();
  } else {
    showToast('warn', '浏览器不支持', '请使用 Chrome / Edge 等支持 WebHID 的浏览器');
  }
});

onUnmounted(() => {
  teardownHidListeners();
});
</script>

<style>
/* 全局样式变量兼容旧代码 */
:root {
  --app-bg: var(--c-bg-primary);
  --app-surface: var(--c-bg-secondary);
  --app-surface-hover: var(--c-bg-hover);
  --app-border: var(--c-border);
  --app-text: var(--c-text-primary);
  --app-text-muted: var(--c-text-muted);
  --app-accent: var(--c-accent);
  --app-accent-soft: var(--c-accent-soft);
  --app-success: var(--c-success);
  --app-warning: var(--c-warning);
  --app-danger: var(--c-danger);
  --app-radius: var(--radius-md);
}

.app-root {
  height: 100vh;
  overflow: hidden;
  background: var(--c-bg-primary);
  color: var(--c-text-primary);
  font-family: 'Nunito', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
}

.loading-overlay {
  position: fixed;
  inset: 0;
  background: rgba(0, 0, 0, 0.5);
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 1rem;
  z-index: 9999;
  backdrop-filter: blur(4px);
}

.loading-text {
  color: var(--c-text-primary);
  font-weight: 600;
  font-size: 1rem;
}

.pwa-update-banner {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  padding: 0.75rem 1.5rem;
  background: var(--c-accent);
  color: #fff;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 1rem;
  z-index: 10000;
  font-weight: 600;
}

.pwa-update-btn {
  padding: 0.35rem 1rem;
  border: 2px solid #fff;
  border-radius: var(--radius-sm);
  background: transparent;
  color: #fff;
  font-weight: 700;
  cursor: pointer;
  transition: all var(--transition-fast);
}

.pwa-update-btn:hover {
  background: #fff;
  color: var(--c-accent);
}

a.pwa-update-btn {
  text-decoration: none;
}
</style>
