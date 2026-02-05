<template>
  <div class="app-root">
    <Toast position="top-center" />

    <!-- 加载遮罩 -->
    <div v-if="deviceStore.isLoading" class="loading-overlay">
      <ProgressSpinner strokeWidth="4" />
      <span class="loading-text">正在通讯...</span>
    </div>

    <!-- 未连接状态 -->
    <div v-if="!deviceStore.isConnected" class="welcome-screen">
      <div class="welcome-content">
        <div class="logo-section">
          <i class="pi pi-bolt logo-icon"></i>
          <h1 class="app-title">MeowKeyboard Studio</h1>
          <p class="app-subtitle">无线版改键工具</p>
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
            <i class="pi pi-cloud-upload"></i>
            <span>云端备份</span>
          </div>
        </div>
      </div>
    </div>

    <!-- 已连接状态 -->
    <div v-else class="main-layout">
      <!-- 顶部导航 -->
      <header class="app-header">
        <div class="header-left">
          <i class="pi pi-bolt header-logo"></i>
          <span class="header-title">MeowKeyboard Studio</span>
        </div>

        <div class="header-center">
          <div class="device-badge">
            <i class="pi pi-check-circle connected-icon"></i>
            <span>{{ deviceStore.device?.productName }} - {{ deviceStore.keyboardTypeName }}</span>
          </div>
        </div>

        <div class="header-right">
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
        <!-- 左侧面板 - 设备信息 -->
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
              <i class="pi pi-layer-group" style="font-size: 1rem;"></i>
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

    <!-- 确认重置对话框 -->
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
import KeyboardLayout from '@/components/KeyboardLayout.vue';
import ActionEditor from '@/components/ActionEditor.vue';

const toast = useToast();
const confirm = useConfirm();
const deviceStore = useDeviceStore();

// 编辑器状态
const editorVisible = ref(false);
const selectedKeyIndex = ref(-1);

const selectedAction = computed<KeyAction>(() => {
  if (selectedKeyIndex.value < 0) return createEmptyAction();
  return deviceStore.getKeyAction(selectedKeyIndex.value) || createEmptyAction();
});

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
  navigator.hid.addEventListener('disconnect', onDeviceDisconnected);
  await autoConnect();
});

onUnmounted(() => {
  navigator.hid.removeEventListener('disconnect', onDeviceDisconnected);
});
</script>

<style>
/* 全局样式变量 */
:root {
  --app-bg: #0a0a0f;
  --app-surface: #12121a;
  --app-surface-hover: #1a1a24;
  --app-border: #2a2a3a;
  --app-text: #e8e8f0;
  --app-text-muted: #8888a0;
  --app-accent: #7c5cff;
  --app-accent-soft: rgba(124, 92, 255, 0.15);
  --app-success: #4ade80;
  --app-danger: #f87171;
  --app-warning: #fbbf24;
  --header-height: 60px;
  --sidebar-width: 280px;
}

* {
  box-sizing: border-box;
}

body {
  margin: 0;
  padding: 0;
  background: var(--app-bg);
  color: var(--app-text);
  font-family: 'Inter', 'SF Pro Display', -apple-system, BlinkMacSystemFont, sans-serif;
}

.app-root {
  min-height: 100vh;
  display: flex;
  flex-direction: column;
}

/* 加载遮罩 */
.loading-overlay {
  position: fixed;
  inset: 0;
  background: rgba(0, 0, 0, 0.7);
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 1rem;
  z-index: 9999;
  backdrop-filter: blur(4px);
}

.loading-text {
  color: var(--app-text-muted);
  font-size: 0.9rem;
}

/* 欢迎页面 */
.welcome-screen {
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  background: 
    radial-gradient(ellipse at top, rgba(124, 92, 255, 0.1) 0%, transparent 50%),
    var(--app-bg);
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
  color: var(--app-accent);
  margin-bottom: 1rem;
  display: block;
}

.app-title {
  font-size: 2.5rem;
  font-weight: 700;
  margin: 0 0 0.5rem;
  background: linear-gradient(135deg, #fff 0%, var(--app-accent) 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
}

.app-subtitle {
  font-size: 1.1rem;
  color: var(--app-text-muted);
  margin: 0;
}

.connect-section {
  margin-bottom: 3rem;
}

.connect-card {
  background: var(--app-surface);
  border: 1px solid var(--app-border);
  border-radius: 16px;
  padding: 2rem;
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
  background: var(--app-surface-hover);
  border: 1px solid var(--app-border);
  border-radius: 8px;
  animation: pulse 2s ease-in-out infinite;
}

.preview-key.tall {
  grid-row: span 2;
}

@keyframes pulse {
  0%, 100% { opacity: 0.5; }
  50% { opacity: 1; }
}

.connect-button {
  width: 100%;
  font-size: 1.1rem !important;
  padding: 0.875rem 1.5rem !important;
}

.connect-hint {
  margin: 1rem 0 0;
  font-size: 0.85rem;
  color: var(--app-text-muted);
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
  color: var(--app-text-muted);
  font-size: 0.85rem;
}

.feature-item i {
  font-size: 1.5rem;
  color: var(--app-accent);
}

/* 主布局 */
.main-layout {
  display: flex;
  flex-direction: column;
  min-height: 100vh;
}

/* 顶部导航 */
.app-header {
  height: var(--header-height);
  background: var(--app-surface);
  border-bottom: 1px solid var(--app-border);
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
  color: var(--app-accent);
}

.header-title {
  font-size: 1.1rem;
  font-weight: 600;
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
  background: var(--app-accent-soft);
  padding: 0.5rem 1rem;
  border-radius: 20px;
  font-size: 0.9rem;
}

.connected-icon {
  color: var(--app-success);
}

.header-right {
  display: flex;
  gap: 0.25rem;
}

/* 主内容 */
.app-main {
  flex: 1;
  display: flex;
  padding: 1.5rem;
  gap: 1.5rem;
}

/* 侧边栏 */
.sidebar {
  width: var(--sidebar-width);
  display: flex;
  flex-direction: column;
  gap: 1rem;
  flex-shrink: 0;
}

.panel {
  background: var(--app-surface);
  border: 1px solid var(--app-border);
  border-radius: 12px;
  padding: 1rem;
}

.panel-title {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  font-size: 0.9rem;
  font-weight: 600;
  margin: 0 0 1rem;
  color: var(--app-text-muted);
}

.panel-title i {
  color: var(--app-accent);
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
  color: var(--app-text-muted);
}

.info-value {
  font-weight: 500;
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

/* 键盘区域 */
.keyboard-section {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
}

.keyboard-container {
  padding: 2rem;
  background: var(--app-surface);
  border: 1px solid var(--app-border);
  border-radius: 16px;
}

.changes-indicator {
  margin-top: 1rem;
  display: flex;
  align-items: center;
  gap: 0.5rem;
  color: var(--app-warning);
  font-size: 0.9rem;
}
</style>
