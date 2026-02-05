<template>
  <div class="app-root" :data-theme="currentTheme">
    <Toast position="top-center" />

    <!-- 加载遮罩 -->
    <div v-if="deviceStore.isLoading" class="loading-overlay">
      <ProgressSpinner strokeWidth="4" />
      <span class="loading-text">正在通讯...</span>
    </div>

    <!-- 未连接状态 - 欢迎页 -->
    <div v-if="!deviceStore.isConnected" class="welcome-screen">
      <!-- 主题切换按钮 -->
      <button class="theme-toggle welcome-theme-toggle" @click="toggleTheme">
        <i :class="currentTheme === 'dark' ? 'pi pi-sun' : 'pi pi-moon'"></i>
      </button>

      <div class="welcome-content">
        <div class="logo-section">
          <div class="logo-icon">🐱</div>
          <h1 class="app-title">MeowKeyboard</h1>
          <p class="app-subtitle">可爱键盘改键工具</p>
        </div>

        <div class="connect-section">
          <div class="connect-card">
            <div class="keyboard-preview">
              <div class="preview-keys">
                <div class="preview-key"></div>
                <div class="preview-key"></div>
                <div class="preview-key tall"></div>
                <div class="preview-key"></div>
                <div class="preview-key"></div>
              </div>
            </div>

            <Button 
              label="连接键盘" 
              icon="pi pi-usb" 
              size="large"
              @click="requestDevice" 
              class="connect-button"
            />

            <p class="connect-hint">
              <i class="pi pi-info-circle"></i>
              请确保键盘已通过 USB 连接
            </p>
          </div>
        </div>

        <div class="features-section">
          <div class="feature-item">
            <i class="pi pi-th-large"></i>
            <span>多层映射</span>
          </div>
          <div class="feature-item">
            <i class="pi pi-palette"></i>
            <span>RGB 灯效</span>
          </div>
          <div class="feature-item">
            <i class="pi pi-code"></i>
            <span>宏录制</span>
          </div>
          <div class="feature-item">
            <i class="pi pi-bolt"></i>
            <span>USB / 蓝牙</span>
          </div>
        </div>
      </div>
    </div>

    <!-- 已连接状态 - 主界面 -->
    <div v-else class="main-layout">
      <!-- 顶部导航 -->
      <header class="app-header">
        <div class="header-left">
          <span class="header-logo">🐱</span>
          <span class="header-title">MeowKeyboard</span>
        </div>

        <div class="header-center">
          <div class="device-badge">
            <i class="pi pi-check-circle connected-icon"></i>
            <span>{{ deviceStore.device?.productName }} - {{ deviceStore.keyboardTypeName }}</span>
          </div>
        </div>

        <div class="header-right">
          <button class="theme-toggle" @click="toggleTheme" v-tooltip.bottom="'切换主题'">
            <i :class="currentTheme === 'dark' ? 'pi pi-sun' : 'pi pi-moon'"></i>
          </button>
          <Button 
            icon="pi pi-sync" 
            severity="secondary" 
            text 
            rounded
            v-tooltip.bottom="'刷新配置'"
            @click="refreshAll"
          />
          <Button 
            icon="pi pi-power-off" 
            severity="danger" 
            text 
            rounded
            v-tooltip.bottom="'断开连接'"
            @click="disconnect"
          />
        </div>
      </header>

      <!-- 主内容区 -->
      <main class="app-main">
        <!-- 左侧面板 -->
        <aside class="sidebar">
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

          <div class="panel layer-panel">
            <h3 class="panel-title">
              <i class="pi pi-layer-group"></i>
              层选择
            </h3>
            <div class="layer-buttons">
              <Button 
                v-for="i in deviceStore.keymap.numLayers" 
                :key="i - 1"
                :label="`层 ${i}`"
                :severity="deviceStore.currentEditLayer === i - 1 ? 'primary' : 'secondary'"
                :outlined="deviceStore.currentEditLayer !== i - 1"
                size="small"
                @click="deviceStore.setEditLayer(i - 1)"
              />
            </div>
          </div>

          <div class="panel actions-panel">
            <h3 class="panel-title">
              <i class="pi pi-cog"></i>
              操作
            </h3>
            <div class="action-buttons">
              <Button 
                label="保存配置" 
                icon="pi pi-save" 
                :disabled="!deviceStore.hasChanges"
                @click="saveConfig"
                class="action-btn"
              />
              <Button 
                label="放弃更改" 
                icon="pi pi-undo" 
                severity="secondary"
                :disabled="!deviceStore.hasChanges"
                @click="discardChanges"
                class="action-btn"
              />
              <Divider />
              <Button 
                label="恢复出厂" 
                icon="pi pi-refresh" 
                severity="danger"
                outlined
                @click="confirmReset"
                class="action-btn"
              />
            </div>
          </div>
        </aside>

        <!-- 中央键盘区 -->
        <section class="keyboard-section">
          <div class="keyboard-container">
            <KeyboardLayout 
              :keyboard-type="deviceStore.deviceInfo?.keyboardType ?? 0"
              :keys="deviceStore.currentLayerKeys"
              :selected-index="selectedKeyIndex"
              @select="onKeySelect"
            />
          </div>

          <!-- 变更状态提示 -->
          <div v-if="deviceStore.hasChanges" class="changes-indicator">
            <i class="pi pi-exclamation-circle"></i>
            <span>有未保存的更改</span>
          </div>
        </section>
      </main>

      <!-- 键位编辑器弹窗 -->
      <ActionEditor
        v-model:visible="editorVisible"
        :key-index="selectedKeyIndex"
        :action="selectedAction"
        @save="onActionSave"
      />
    </div>

    <!-- 确认对话框 -->
    <ConfirmDialog />
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue';
import { useToast } from 'primevue/usetoast';
import { useConfirm } from 'primevue/useconfirm';
import { useDeviceStore } from '@/stores/deviceStore';
import { hidService } from '@/services/HidService';
import type { KeyAction } from '@/types/protocol';
import { createEmptyAction } from '@/types/protocol';
import { applyTheme, getSavedTheme, saveTheme, getSystemTheme, type ThemeMode } from '@/config/theme';
import KeyboardLayout from '@/components/KeyboardLayout.vue';
import ActionEditor from '@/components/ActionEditor.vue';

const toast = useToast();
const confirm = useConfirm();
const deviceStore = useDeviceStore();

// 主题
const currentTheme = ref<ThemeMode>('dark');

// 编辑器状态
const editorVisible = ref(false);
const selectedKeyIndex = ref(-1);

const selectedAction = computed<KeyAction>(() => {
  if (selectedKeyIndex.value < 0) return createEmptyAction();
  return deviceStore.getKeyAction(selectedKeyIndex.value) || createEmptyAction();
});

// ----------------------------------------
// 主题切换
// ----------------------------------------

function initTheme() {
  const saved = getSavedTheme();
  currentTheme.value = saved || getSystemTheme();
  applyTheme(currentTheme.value);
}

function toggleTheme() {
  currentTheme.value = currentTheme.value === 'dark' ? 'light' : 'dark';
  applyTheme(currentTheme.value);
  saveTheme(currentTheme.value);
}

// ----------------------------------------
// 设备连接
// ----------------------------------------

async function requestDevice() {
  try {
    const device = await hidService.requestDevice();
    if (device) {
      const success = await deviceStore.connectDevice(device);
      if (success) {
        showToast('success', '连接成功', `已连接到 ${device.productName}`);
      } else {
        showToast('error', '连接失败', deviceStore.errorMessage || '无法连接设备');
      }
    }
  } catch (error) {
    showToast('error', '连接失败', error instanceof Error ? error.message : '未知错误');
  }
}

async function autoConnect() {
  const device = await hidService.getAuthorizedDevice();
  if (device) {
    const success = await deviceStore.connectDevice(device);
    if (success) {
      showToast('success', '自动连接', `已连接到 ${device.productName}`);
    }
  }
}

async function disconnect() {
  await deviceStore.disconnectDevice();
  showToast('info', '已断开', '设备连接已关闭');
}

async function refreshAll() {
  try {
    await deviceStore.refreshKeymap();
    await deviceStore.refreshRgbConfig();
    await deviceStore.refreshFnKeyConfig();
    showToast('success', '刷新成功', '配置已从设备重新加载');
  } catch (error) {
    showToast('error', '刷新失败', error instanceof Error ? error.message : '未知错误');
  }
}

// ----------------------------------------
// 配置操作
// ----------------------------------------

async function saveConfig() {
  try {
    await deviceStore.saveKeymap();
    showToast('success', '保存成功', '配置已保存到设备');
  } catch (error) {
    showToast('error', '保存失败', error instanceof Error ? error.message : '未知错误');
  }
}

function discardChanges() {
  deviceStore.discardChanges();
  showToast('info', '已撤销', '更改已放弃');
}

function confirmReset() {
  confirm.require({
    message: '确定要恢复出厂设置吗？所有自定义配置将丢失。',
    header: '恢复出厂设置',
    icon: 'pi pi-exclamation-triangle',
    acceptClass: 'p-button-danger',
    acceptLabel: '确定重置',
    rejectLabel: '取消',
    accept: async () => {
      try {
        await deviceStore.resetToFactory();
        showToast('success', '重置成功', '已恢复出厂设置');
      } catch (error) {
        showToast('error', '重置失败', error instanceof Error ? error.message : '未知错误');
      }
    },
  });
}

// ----------------------------------------
// 键位编辑
// ----------------------------------------

function onKeySelect(index: number) {
  selectedKeyIndex.value = index;
  editorVisible.value = true;
}

function onActionSave(action: KeyAction) {
  if (selectedKeyIndex.value >= 0) {
    deviceStore.setKeyAction(selectedKeyIndex.value, action);
  }
  editorVisible.value = false;
}

// ----------------------------------------
// 工具函数
// ----------------------------------------

function showToast(severity: 'success' | 'info' | 'warn' | 'error', summary: string, detail: string) {
  toast.add({ severity, summary, detail, life: 2500 });
}

// ----------------------------------------
// 生命周期
// ----------------------------------------

function onDeviceDisconnected(event: HIDConnectionEvent) {
  if (event.device === deviceStore.device) {
    deviceStore.device = null;
    showToast('warn', '设备断开', '键盘连接已丢失');
  }
}

onMounted(async () => {
  initTheme();
  navigator.hid.addEventListener('disconnect', onDeviceDisconnected);
  await autoConnect();
});

onUnmounted(() => {
  navigator.hid.removeEventListener('disconnect', onDeviceDisconnected);
});
</script>

<style>
/* ==========================================
   全局样式变量 (兼容旧代码)
========================================== */
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
  --app-danger: var(--c-danger);
  --app-warning: var(--c-warning);
}

* {
  box-sizing: border-box;
}

body {
  margin: 0;
  padding: 0;
  background: var(--c-bg-primary);
  color: var(--c-text-primary);
  font-family: 'Nunito', -apple-system, BlinkMacSystemFont, sans-serif;
}

.app-root {
  min-height: 100vh;
  display: flex;
  flex-direction: column;
}

/* ==========================================
   加载遮罩
========================================== */
.loading-overlay {
  position: fixed;
  inset: 0;
  background: rgba(0, 0, 0, 0.75);
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 1rem;
  z-index: 9999;
  backdrop-filter: blur(6px);
}

.loading-text {
  color: var(--c-text-muted);
  font-size: 0.9rem;
  font-weight: 500;
}

/* ==========================================
   主题切换按钮
========================================== */
.theme-toggle {
  width: 40px;
  height: 40px;
  border-radius: var(--radius-md);
  border: 1px solid var(--c-border);
  background: var(--c-bg-tertiary);
  color: var(--c-text-muted);
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: all var(--transition-fast);
}

.theme-toggle:hover {
  background: var(--c-bg-hover);
  color: var(--c-accent);
  border-color: var(--c-accent);
}

.theme-toggle i {
  font-size: 1.1rem;
}

.welcome-theme-toggle {
  position: absolute;
  top: 1.5rem;
  right: 1.5rem;
}

/* ==========================================
   欢迎页面
========================================== */
.welcome-screen {
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  position: relative;
  background: 
    radial-gradient(ellipse at top, var(--c-accent-soft) 0%, transparent 50%),
    var(--c-bg-primary);
}

.welcome-content {
  text-align: center;
  max-width: 600px;
  padding: 2rem;
}

.logo-section {
  margin-bottom: 3rem;
}

.logo-icon {
  font-size: 4rem;
  margin-bottom: 1rem;
  animation: float 3s ease-in-out infinite;
}

@keyframes float {
  0%, 100% { transform: translateY(0); }
  50% { transform: translateY(-10px); }
}

.app-title {
  font-size: 2.75rem;
  font-weight: 800;
  margin: 0 0 0.5rem;
  background: var(--c-accent-gradient);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
}

.app-subtitle {
  font-size: 1.1rem;
  color: var(--c-text-muted);
  margin: 0;
  font-weight: 500;
}

.connect-section {
  margin-bottom: 3rem;
}

.connect-card {
  background: var(--c-bg-secondary);
  border: 1px solid var(--c-border);
  border-radius: var(--radius-xl);
  padding: 2rem;
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.2);
}

.keyboard-preview {
  margin-bottom: 1.5rem;
}

.preview-keys {
  display: grid;
  grid-template-columns: repeat(3, 50px);
  grid-template-rows: repeat(2, 50px);
  gap: 8px;
  justify-content: center;
}

.preview-key {
  background: var(--c-bg-tertiary);
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  animation: pulse 2s ease-in-out infinite;
}

.preview-key.tall {
  grid-row: span 2;
}

.connect-button {
  width: 100%;
  font-size: 1.1rem !important;
  font-weight: 700 !important;
  padding: 0.875rem 1.5rem !important;
}

.connect-hint {
  margin: 1rem 0 0;
  font-size: 0.85rem;
  color: var(--c-text-muted);
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 0.5rem;
}

.features-section {
  display: flex;
  justify-content: center;
  gap: 2rem;
  flex-wrap: wrap;
}

.feature-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 0.5rem;
  color: var(--c-text-muted);
  font-size: 0.85rem;
  font-weight: 600;
}

.feature-item i {
  font-size: 1.5rem;
  color: var(--c-accent);
}

/* ==========================================
   主布局
========================================== */
.main-layout {
  display: flex;
  flex-direction: column;
  min-height: 100vh;
}

/* ==========================================
   顶部导航
========================================== */
.app-header {
  height: var(--header-height);
  background: var(--c-bg-secondary);
  border-bottom: 1px solid var(--c-border);
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 1.5rem;
  position: sticky;
  top: 0;
  z-index: 100;
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
  font-weight: 700;
  background: var(--c-accent-gradient);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
}

.header-center {
  position: absolute;
  left: 50%;
  transform: translateX(-50%);
}

.device-badge {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  background: var(--c-accent-soft);
  padding: 0.5rem 1rem;
  border-radius: var(--radius-xl);
  font-size: 0.9rem;
  font-weight: 600;
}

.connected-icon {
  color: var(--c-success);
}

.header-right {
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

/* ==========================================
   主内容
========================================== */
.app-main {
  flex: 1;
  display: flex;
  padding: 1.5rem;
  gap: 1.5rem;
}

/* ==========================================
   侧边栏
========================================== */
.sidebar {
  width: var(--sidebar-width);
  display: flex;
  flex-direction: column;
  gap: 1rem;
  flex-shrink: 0;
}

.panel {
  background: var(--c-bg-secondary);
  border: 1px solid var(--c-border);
  border-radius: var(--radius-lg);
  padding: 1rem;
}

.panel-title {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  font-size: 0.9rem;
  font-weight: 700;
  margin: 0 0 1rem;
  color: var(--c-text-muted);
}

.panel-title i {
  color: var(--c-accent);
  font-size: 0.9rem;
}

.info-list {
  display: flex;
  flex-direction: column;
  gap: 0.5rem;
}

.info-item {
  display: flex;
  justify-content: space-between;
  font-size: 0.85rem;
}

.info-label {
  color: var(--c-text-muted);
}

.info-value {
  font-weight: 600;
}

.layer-buttons {
  display: flex;
  gap: 0.5rem;
  flex-wrap: wrap;
}

.action-buttons {
  display: flex;
  flex-direction: column;
  gap: 0.5rem;
}

.action-btn {
  width: 100%;
  justify-content: flex-start !important;
}

/* ==========================================
   键盘区域
========================================== */
.keyboard-section {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
}

.keyboard-container {
  background: var(--c-bg-secondary);
  border: 1px solid var(--c-border);
  border-radius: var(--radius-xl);
  padding: 1.5rem;
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.15);
}

.changes-indicator {
  margin-top: 1rem;
  display: flex;
  align-items: center;
  gap: 0.5rem;
  color: var(--c-warning);
  font-size: 0.9rem;
  font-weight: 600;
}
</style>
