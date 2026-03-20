<script setup lang="ts">
import { ref, computed, watch, onMounted, onUnmounted } from 'vue';
import { useDeviceStore } from '@/stores/deviceStore';
import { useConfirm } from 'primevue/useconfirm';
import { useMacroStore } from '@/stores/macroStore';
import { useTerminalStore } from '@/stores/terminalStore';
import { showToast } from '@/services/toastService';
import type { KeyAction } from '@/types/protocol';
import { createEmptyAction, KeyboardType, KeyboardTypeInfo } from '@/types/protocol';
import { createDeviceUiDefinition, hasUiSection } from '@/types/deviceUi';
import { getHidDevicePlugin } from '@/services/hid/registry';

import AppHeader from '@/layouts/AppHeader.vue';
import CatAssistant from '@/components/CatAssistant.vue';
import KeyboardLayout from '@/components/KeyboardLayout.vue';
import KeyboardDecoration from '@/components/KeyboardDecoration.vue';
import ActionEditor from '@/components/ActionEditor.vue';
import MacroEditor from '@/components/MacroEditor.vue';
import DeviceInfoPanel from '@/components/DeviceInfoPanel.vue';
import KeyboardStatus from '@/components/KeyboardStatus.vue';
import LayerPanel from '@/components/LayerPanel.vue';
import FnPanel from '@/components/FnPanel.vue';
import RgbPanel from '@/components/RgbPanel.vue';
import ActionsPanel from '@/components/ActionsPanel.vue';
import MacroPanel from '@/components/MacroPanel.vue';
import ThemeConfigurator from '@/components/ThemeConfigurator.vue';
import FallingEffect from '@/components/FallingEffect.vue';
import CatEars from '@/components/CatEars.vue';
import VideoBackground from '@/components/VideoBackground.vue';
import { useTheme } from '@/composables/useTheme';

const props = defineProps<{
  onRefresh: () => void;
  onDisconnect: () => void;
  onToggleTheme: () => void;
}>();

const confirm = useConfirm();
const deviceStore = useDeviceStore();
const macroStore = useMacroStore();
const terminalStore = useTerminalStore();
const { openConfigurator, themeId } = useTheme();

// 预览模式
const previewKeyboardType = ref(-1);

// 编辑器状态
const editorVisible = ref(false);
const selectedKeyIndex = ref(-1);
const macroEditorVisible = ref(false);
const macroEditorSlot = ref(0);
const kbCardRef = ref<HTMLElement | null>(null);
const earStyle = ref<Record<string, string>>({});

function updateEarPosition() {
  if (!kbCardRef.value) return;
  const rect = kbCardRef.value.getBoundingClientRect();
  earStyle.value = {
    top: `${rect.top - 50}px`,
    left: `${rect.left + rect.width / 2}px`,
    transform: 'translateX(-50%)',
  };
}

let earRaf = 0;
function earLoop() {
  updateEarPosition();
  earRaf = requestAnimationFrame(earLoop);
}

onMounted(() => { if (themeId.value === 'neko') earLoop(); });
onUnmounted(() => cancelAnimationFrame(earRaf));

watch(themeId, (id) => {
  if (id === 'neko') { earLoop(); } else { cancelAnimationFrame(earRaf); }
});

async function openMacroEditor(slot: number) {
  await macroStore.startEditing(slot);
  macroEditorSlot.value = slot;
  macroEditorVisible.value = true;
}

const selectedAction = computed<KeyAction>(() => {
  if (selectedKeyIndex.value < 0) return createEmptyAction();
  return deviceStore.getKeyAction(selectedKeyIndex.value) || createEmptyAction();
});

const keyboardSectionStyle = computed(() => {
  const statusBarH = 32;
  if (terminalStore.isOpen) {
    return { bottom: (terminalStore.panelHeight + statusBarH) + 'px' };
  }
  return { bottom: statusBarH + 'px' };
});

const currentKeyboardType = computed(() => {
  if (previewKeyboardType.value >= 0) return previewKeyboardType.value;
  return deviceStore.deviceInfo?.keyboardType ?? 0;
});

const currentUiDefinition = computed(() => {
  if (previewKeyboardType.value >= 0) {
    return createDeviceUiDefinition('preview', ['device-info', 'keyboard-status', 'layer-panel', 'actions-panel', 'debug-terminal']);
  }
  const protocol = deviceStore.deviceInfo?.protocol;
  if (!protocol) {
    return createDeviceUiDefinition('preview', ['device-info', 'keyboard-status', 'actions-panel', 'debug-terminal']);
  }
  const plugin = getHidDevicePlugin(protocol);
  if (!plugin) {
    return createDeviceUiDefinition(protocol, ['device-info', 'keyboard-status', 'actions-panel', 'debug-terminal']);
  }
  return plugin.getUiDefinition(deviceStore.capabilities);
});

const showLayerPanel = computed(() => hasUiSection(currentUiDefinition.value, 'layer-panel'));
const showFnPanel = computed(() => previewKeyboardType.value < 0 && hasUiSection(currentUiDefinition.value, 'fn-panel'));
const showRgbPanel = computed(() => previewKeyboardType.value < 0 && hasUiSection(currentUiDefinition.value, 'rgb-panel'));
const showMacroPanel = computed(() => previewKeyboardType.value < 0 && deviceStore.supportsMacroActions);
const showResetButton = computed(() => previewKeyboardType.value < 0 && deviceStore.supportsFactoryReset);
const showLayerBadge = computed(() => previewKeyboardType.value >= 0 || deviceStore.supportsMultiLayer);
const saveKeymapLabel = computed(() => deviceStore.supportsExplicitSave ? '保存配置' : '写入键位');

const keyboardCardSubtitle = computed(() => {
  if (previewKeyboardType.value >= 0) return '预览布局模式';
  if (deviceStore.supportsMultiLayer) {
    const modifierLabel = deviceStore.deviceInfo?.protocol === 'ch552' ? 'FUNC' : 'FN';
    return `点击按键进行编辑 · 按住 ${modifierLabel} + 按键N 切换到层N`;
  }
  return '点击按键进行编辑 · 当前设备仅支持单层键位映射';
});

const currentLayerKeysForDisplay = computed(() => {
  if (previewKeyboardType.value >= 0) {
    const keyCount = KeyboardTypeInfo[currentKeyboardType.value as KeyboardType]?.keys || 4;
    return Array.from({ length: keyCount }, () => createEmptyAction());
  }
  return deviceStore.currentLayerKeys;
});

// 配置操作
async function saveConfig() {
  try {
    await deviceStore.saveKeymap();
    showToast('success', '保存成功', deviceStore.supportsExplicitSave ? '配置已保存到设备' : '键位已写入设备');
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

function onCatAction(action: string) {
  switch (action) {
    case 'refresh': props.onRefresh(); break;
    case 'theme': props.onToggleTheme(); break;
    case 'themeConfig': openConfigurator(); break;
    case 'disconnect': props.onDisconnect(); break;
    case 'scrollTop': window.scrollTo({ top: 0, behavior: 'smooth' }); break;
  }
}
</script>

<template>
  <div class="main-layout">
    <!-- 安哥拉兔主题视频背景（lazy加载） -->
    <VideoBackground v-if="themeId === 'angora'" src="https://file.icve.com.cn/file_doc/qdqqd/WV62vskP_vip.mp4" />
    <!-- 琉璃主题飘落效果 -->
    <FallingEffect v-if="themeId === 'liuli'" type="sakura" :count="15" />
    <!-- 安哥拉兔主题飘落效果 -->
    <FallingEffect v-if="themeId === 'angora'" type="sakura" :count="10" />
    <!-- 青蛙主题飘落花瓣 -->
    <FallingEffect v-if="themeId === 'frog'" type="sakura" :count="10" />

    <CatAssistant @action="onCatAction" />

    <AppHeader v-model:preview-type="previewKeyboardType" @refresh="onRefresh" @disconnect="onDisconnect" />

    <main class="app-main" :class="{ 'terminal-open': terminalStore.isOpen }">
      <!-- 左侧面板 -->
      <aside class="sidebar">
        <DeviceInfoPanel />
        <KeyboardStatus />
        <LayerPanel v-if="showLayerPanel" :keyboard-type="currentKeyboardType" :preview-mode="previewKeyboardType >= 0" />
        <FnPanel v-if="showFnPanel" />
        <RgbPanel v-if="showRgbPanel" />
        <MacroPanel v-if="showMacroPanel" @edit="openMacroEditor" />
        <ActionsPanel :show-reset-button="showResetButton" :save-label="saveKeymapLabel" @save="saveConfig"
          @discard="discardChanges" @reset="confirmReset" />
      </aside>

      <div class="keyboard-spacer"></div>

      <!-- 中央键盘区 -->
      <section class="keyboard-section" :style="keyboardSectionStyle">
        <KeyboardDecoration />

        <!-- 猫咪主题猫耳（独立浮层，不侵入键盘组件） -->
        <div v-if="themeId === 'neko'" class="cat-ears-overlay" :style="earStyle">
          <CatEars />
        </div>

        <div ref="kbCardRef" class="keyboard-card">
          <div class="card-header">
            <div class="card-title-section">
              <span class="card-title">🎹 键盘布局</span>
              <span v-if="showLayerBadge" class="card-layer-badge">层 {{ deviceStore.currentEditLayer + 1 }}</span>
            </div>
            <span class="card-subtitle">{{ keyboardCardSubtitle }}</span>
          </div>
          <div class="keyboard-container">
            <KeyboardLayout :keyboard-type="currentKeyboardType" :keys="currentLayerKeysForDisplay"
              :selected-index="selectedKeyIndex" :disabled="previewKeyboardType >= 0" @select="onKeySelect" />
          </div>
        </div>

        <div v-if="deviceStore.hasChanges" class="changes-indicator">
          <i class="pi pi-exclamation-circle"></i>
          <span>有未保存的更改</span>
        </div>
      </section>
    </main>

    <ActionEditor v-model:visible="editorVisible" :key-index="selectedKeyIndex" :action="selectedAction"
      @save="onActionSave" />
    <MacroEditor v-model:visible="macroEditorVisible" :slot="macroEditorSlot" />
    <ThemeConfigurator />
  </div>
</template>

<style scoped>
.main-layout {
  display: flex;
  flex-direction: column;
  height: 100vh;
  overflow: hidden;
}

.app-main {
  flex: 1;
  display: flex;
  padding: 1.5rem;
  padding-top: calc(var(--header-height) + 1.5rem);
  padding-bottom: 50px;
  gap: 1.5rem;
  overflow: hidden;
  transition: padding-bottom 0.3s ease;
}

.app-main.terminal-open {
  padding-bottom: 340px;
}

.sidebar {
  width: var(--sidebar-width);
  display: flex;
  flex-direction: column;
  gap: 0.75rem;
  flex-shrink: 0;
  height: calc(100vh - var(--header-height) - 3rem);
  overflow-y: auto;
  overflow-x: hidden;
  scrollbar-width: thin;
  scrollbar-color: var(--c-border) transparent;
}

.sidebar::-webkit-scrollbar { width: 4px; }
.sidebar::-webkit-scrollbar-track { background: transparent; }
.sidebar::-webkit-scrollbar-thumb { background: var(--c-border); border-radius: 2px; }

.keyboard-spacer {
  flex: 1;
  min-width: 200px;
}

.keyboard-section {
  position: fixed;
  left: calc(var(--sidebar-width) + 3rem);
  right: 0;
  top: var(--header-height);
  bottom: 0;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  overflow: hidden;
  pointer-events: none;
}

.keyboard-section .keyboard-card,
.keyboard-section .changes-indicator {
  pointer-events: auto;
}

.keyboard-card {
  position: relative;
  z-index: 1;
  background: var(--c-bg-secondary);
  border: 1px solid var(--c-border);
  border-radius: var(--radius-xl);
  overflow: hidden;
}

.card-header {
  background: var(--c-bg-tertiary);
  border-bottom: 1px solid var(--c-border);
  padding: 1rem 1.5rem;
  display: flex;
  flex-direction: column;
  gap: 0.5rem;
}

.card-title-section {
  display: flex;
  align-items: center;
  gap: 0.75rem;
}

.card-title {
  font-size: 1rem;
  font-weight: 700;
  color: var(--c-text-primary);
}

.card-layer-badge {
  font-size: 0.75rem;
  font-weight: 700;
  color: var(--c-accent);
  background: var(--c-accent-soft);
  padding: 0.25rem 0.6rem;
  border-radius: var(--radius-sm);
  border: 1px solid var(--c-accent);
}

.card-subtitle {
  font-size: 0.75rem;
  color: var(--c-text-muted);
  line-height: 1.4;
}

.keyboard-container {
  padding: 2rem 2.5rem;
}

.changes-indicator {
  margin-top: 1.25rem;
  display: flex;
  align-items: center;
  gap: 0.5rem;
  color: var(--c-warning);
  font-size: 0.9rem;
  font-weight: 600;
  padding: 0.5rem 1rem;
  background: rgba(251, 191, 36, 0.1);
  border-radius: var(--radius-md);
  position: relative;
  z-index: 1;
}

/* 猫耳独立浮层 — fixed 定位，JS 动态跟踪 keyboard-card 顶部 */
.cat-ears-overlay {
  position: fixed;
  z-index: 2;
  pointer-events: none;
}
</style>
