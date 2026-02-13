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
          <h1 class="app-title">BinaryKeyboard</h1>
          <p class="app-subtitle">开源二进制键盘改键工具</p>
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

            <Button label="连接键盘" icon="pi pi-usb" size="large" @click="requestDevice" class="connect-button" />

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
          <!-- 预览模式切换器 -->
          <div class="preview-mode-selector">
            <select v-model="previewKeyboardType" @change="onPreviewTypeChange" class="preview-select"
              v-tooltip.bottom="'预览不同键盘型号的布局'">
              <option :value="-1">实际设备</option>
              <option :value="0">预览：基础款</option>
              <option :value="1">预览：五键款</option>
              <option :value="2">预览：旋钮款</option>
            </select>
          </div>
          <button class="theme-toggle" @click="toggleTheme" v-tooltip.bottom="'切换主题'">
            <i :class="currentTheme === 'dark' ? 'pi pi-sun' : 'pi pi-moon'"></i>
          </button>
          <Button icon="pi pi-sync" severity="secondary" text rounded v-tooltip.bottom="'刷新配置'" @click="refreshAll"
            class="header-btn btn-icon-secondary" />
          <Button icon="pi pi-power-off" severity="danger" text rounded v-tooltip.bottom="'断开连接'" @click="disconnect"
            class="header-btn btn-icon-danger" />
        </div>
      </header>

      <!-- 主内容区 -->
      <main class="app-main" :class="{ 'terminal-open': terminalStore.isOpen }">
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

          <KeyboardStatus />

          <div class="panel layer-panel">
            <h3 class="panel-title">
              <i class="pi pi-layer-group"></i>
              层选择
            </h3>
            <div class="layer-hint">
              <i class="pi pi-info-circle"></i>
              <span>按住 FN + 按键N 在键盘上切换层</span>
            </div>
            <div class="layer-legend">
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
            <div v-if="layerLayout" class="layer-keyboard-mini" :style="{
              gridTemplateColumns: `repeat(${layerLayout.cols}, 1fr)`,
              gridTemplateRows: `repeat(${layerLayout.rows}, 1fr)`,
            }">
              <div v-for="key in layerLayout.keys" :key="key.index" class="layer-key-mini" :class="{
                'current-layer': deviceStore.deviceStatus?.currentLayer === getLayerIndexByKeyIndex(key.index, currentKeyboardType),
                'edit-layer': deviceStore.currentEditLayer === getLayerIndexByKeyIndex(key.index, currentKeyboardType),
                [`key-${key.size}`]: true,
                'key-encoder-press': key.type === 'encoder-press',
                'disabled': isLayerKeyDisabled(key, currentKeyboardType)
              }" :style="getKeyStyle(key)"
                @click="onLayerKeyClick(key, currentKeyboardType)"
                :title="getLayerKeyTitle(key, currentKeyboardType)">
                <span class="layer-key-number" v-if="!isLayerKeyDisabled(key, currentKeyboardType)">{{ getLayerIndexByKeyIndex(key.index, currentKeyboardType) + 1 }}</span>
                <span class="layer-key-label" v-if="key.type !== 'encoder-press'">层{{ getLayerIndexByKeyIndex(key.index,
                  currentKeyboardType) + 1 }}</span>
                <span class="layer-key-label encoder-label" v-else>🎚️</span>
              </div>
            </div>
            <div v-else class="layer-keyboard-mini-placeholder">
              <span>未连接设备</span>
            </div>
          </div>

          <div class="panel fn-panel">
            <h3 class="panel-title">
              <i class="pi pi-bolt"></i>
              FN 键设置
            </h3>
            <div class="fn-config">
              <!-- FN1 -->
              <div class="fn-group">
                <span class="fn-group-title">FN1</span>
                <div class="fn-item">
                  <span class="fn-label">单击</span>
                  <select v-model="deviceStore.fnKeyConfig.fnKeys[0].clickAction" class="fn-select">
                    <option :value="0x00">无动作</option>
                    <option :value="0x01">切换模式</option>
                    <option :value="0x10">RGB 开关</option>
                    <option :value="0x11">RGB 下一模式</option>
                    <option :value="0x13">亮度+</option>
                    <option :value="0x14">亮度-</option>
                    <option :value="0x20">下一层</option>
                    <option :value="0x21">上一层</option>
                  </select>
                </div>
                <div class="fn-item">
                  <span class="fn-label">长按</span>
                  <select v-model="deviceStore.fnKeyConfig.fnKeys[0].longAction" class="fn-select">
                    <option :value="0x00">无动作</option>
                    <option :value="0x01">切换模式</option>
                    <option :value="0x02">蓝牙广播</option>
                    <option :value="0x03">蓝牙断开</option>
                    <option :value="0x04">清除配对</option>
                    <option :value="0x10">RGB 开关</option>
                    <option :value="0x11">RGB 下一模式</option>
                    <option :value="0x13">亮度+</option>
                    <option :value="0x14">亮度-</option>
                    <option :value="0x20">下一层</option>
                    <option :value="0x21">上一层</option>
                    <option :value="0x40">休眠</option>
                  </select>
                </div>
              </div>
              <!-- FN2 -->
              <div class="fn-group">
                <span class="fn-group-title">FN2</span>
                <div class="fn-item">
                  <span class="fn-label">单击</span>
                  <select v-model="deviceStore.fnKeyConfig.fnKeys[1].clickAction" class="fn-select">
                    <option :value="0x00">无动作</option>
                    <option :value="0x01">切换模式</option>
                    <option :value="0x10">RGB 开关</option>
                    <option :value="0x11">RGB 下一模式</option>
                    <option :value="0x13">亮度+</option>
                    <option :value="0x14">亮度-</option>
                    <option :value="0x20">下一层</option>
                    <option :value="0x21">上一层</option>
                  </select>
                </div>
                <div class="fn-item">
                  <span class="fn-label">长按</span>
                  <select v-model="deviceStore.fnKeyConfig.fnKeys[1].longAction" class="fn-select">
                    <option :value="0x00">无动作</option>
                    <option :value="0x01">切换模式</option>
                    <option :value="0x02">蓝牙广播</option>
                    <option :value="0x03">蓝牙断开</option>
                    <option :value="0x04">清除配对</option>
                    <option :value="0x10">RGB 开关</option>
                    <option :value="0x11">RGB 下一模式</option>
                    <option :value="0x13">亮度+</option>
                    <option :value="0x14">亮度-</option>
                    <option :value="0x20">下一层</option>
                    <option :value="0x21">上一层</option>
                    <option :value="0x40">休眠</option>
                  </select>
                </div>
              </div>
              <Button label="保存 FN" icon="pi pi-check" size="small" @click="saveFnConfig"
                class="fn-save-btn btn-primary" />
            </div>
          </div>

          <div class="panel rgb-panel">
            <h3 class="panel-title">
              <i class="pi pi-palette"></i>
              RGB 灯效
            </h3>
            <div class="rgb-config">
              <div class="rgb-item">
                <span class="rgb-label">模式</span>
                <select v-model="deviceStore.rgbConfig.mode" class="rgb-select">
                  <option :value="1">静态</option>
                  <option :value="2">呼吸</option>
                  <option :value="3">闪烁</option>
                  <option :value="4">彩虹</option>
                  <option :value="5">仅指示灯</option>
                </select>
              </div>
              <div v-if="showRgbColorPicker" class="rgb-item rgb-color-row">
                <span class="rgb-label">颜色</span>
                <div class="rgb-color-controls">
                  <ColorPicker
                    v-model="rgbColorHex"
                    format="hex"
                    inline
                    class="rgb-color-picker"
                  />
                  <input
                    v-model="rgbColorHex"
                    type="text"
                    class="rgb-hex-input"
                    placeholder="#ffffff"
                    maxlength="7"
                  />
                </div>
              </div>
              <div class="rgb-item">
                <span class="rgb-label">RGB 亮度 {{ Math.round(deviceStore.rgbConfig.brightness * 100 / 255) }}%</span>
                <input type="range" v-model.number="deviceStore.rgbConfig.brightness" min="0" max="255" class="rgb-slider" />
              </div>
              <div class="rgb-item">
                <span class="rgb-label">指示灯亮度 {{ Math.round(deviceStore.rgbConfig.indicatorBrightness * 100 / 255) }}%</span>
                <input type="range" v-model.number="deviceStore.rgbConfig.indicatorBrightness" :min="RGB_INDICATOR_MIN_BRIGHTNESS" max="255" class="rgb-slider" />
              </div>
              <Button label="保存 RGB" icon="pi pi-check" size="small" @click="saveRgbConfig"
                class="rgb-save-btn btn-primary" />
            </div>
          </div>

          <div class="panel actions-panel">
            <h3 class="panel-title">
              <i class="pi pi-cog"></i>
              操作
            </h3>
            <div class="action-buttons">
              <Button label="保存配置" icon="pi pi-save" :disabled="!deviceStore.hasChanges" @click="saveConfig"
                class="action-btn btn-primary" />
              <Button label="放弃更改" icon="pi pi-undo" severity="secondary" :disabled="!deviceStore.hasChanges"
                @click="discardChanges" class="action-btn btn-secondary" />
              <Divider />
              <Button label="恢复出厂" icon="pi pi-refresh" severity="danger" outlined @click="confirmReset"
                class="action-btn btn-danger-outline" />
            </div>
          </div>
        </aside>

        <!-- 占位，保持布局 -->
        <div class="keyboard-spacer"></div>

        <!-- 中央键盘区 - 悬浮居中 -->
        <section class="keyboard-section" :style="keyboardSectionStyle">
          <!-- 装饰背景 -->
          <div class="keyboard-decoration">
            <div class="deco-circle deco-1"></div>
            <div class="deco-circle deco-2"></div>
            <div class="deco-circle deco-3"></div>
            <!-- 贯穿屏幕的脚印轨迹 - S曲线路径 -->
            <span class="paw-walk paw-w1">🐾</span>
            <span class="paw-walk paw-w2">🐾</span>
            <span class="paw-walk paw-w3">🐾</span>
            <span class="paw-walk paw-w4">🐾</span>
            <span class="paw-walk paw-w5">🐾</span>
            <span class="paw-walk paw-w6">🐾</span>
            <span class="paw-walk paw-w7">🐾</span>
            <span class="paw-walk paw-w8">🐾</span>
            <span class="paw-walk paw-w9">🐾</span>

            <div class="deco-star deco-star-1">✨</div>
            <div class="deco-star deco-star-2">✨</div>
            <!-- 键盘装饰 -->
            <span class="deco-emoji deco-cat-1">😺</span>
            <span class="deco-emoji deco-cat-2">🐱</span>
            <span class="deco-emoji deco-cat-3">😸</span>
            <!-- 小装饰 -->
            <span class="deco-mini deco-yarn">🧶</span>
            <span class="deco-mini deco-star-3">⭐</span>
          </div>

          <div class="keyboard-card">
            <div class="card-header">
              <div class="card-title-section">
                <span class="card-title">🎹 键盘布局</span>
                <span class="card-layer-badge">层 {{ deviceStore.currentEditLayer + 1 }}</span>
              </div>
              <span class="card-subtitle">点击按键进行编辑 · 按住 FN + 按键N 切换到层N</span>
            </div>
            <div class="keyboard-container">
              <KeyboardLayout :keyboard-type="currentKeyboardType" :keys="currentLayerKeysForDisplay"
                :selected-index="selectedKeyIndex" :disabled="previewKeyboardType >= 0" @select="onKeySelect" />
            </div>
          </div>

          <!-- 变更状态提示 -->
          <div v-if="deviceStore.hasChanges" class="changes-indicator">
            <i class="pi pi-exclamation-circle"></i>
            <span>有未保存的更改</span>
          </div>
        </section>
      </main>

      <!-- 键位编辑器弹窗 -->
      <ActionEditor v-model:visible="editorVisible" :key-index="selectedKeyIndex" :action="selectedAction"
        @save="onActionSave" />
    </div>

    <!-- 确认对话框 -->
    <ConfirmDialog />

    <!-- 调试终端 -->
    <DebugTerminal />
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue';
import { useToast } from 'primevue/usetoast';
import { useConfirm } from 'primevue/useconfirm';
import { useDeviceStore } from '@/stores/deviceStore';
import { hidService } from '@/services/HidService';
import type { KeyAction } from '@/types/protocol';
import {
  createEmptyAction,
  KeyboardType,
  KeyboardTypeInfo,
  RGB_INDICATOR_MIN_BRIGHTNESS,
  rgbToHex,
  hexToRgb,
} from '@/types/protocol';
import { applyTheme, getSavedTheme, saveTheme, getSystemTheme, type ThemeMode } from '@/config/theme';
import { getLayoutByType, getLayerLayoutByType, type LayoutDef } from '@/config/layouts';
import KeyboardLayout from '@/components/KeyboardLayout.vue';
import ActionEditor from '@/components/ActionEditor.vue';
import DebugTerminal from '@/components/DebugTerminal.vue';
import KeyboardStatus from '@/components/KeyboardStatus.vue';
import { useTerminalStore } from '@/stores/terminalStore';

const toast = useToast();
const confirm = useConfirm();
const deviceStore = useDeviceStore();
const terminalStore = useTerminalStore();

// 主题
const currentTheme = ref<ThemeMode>('dark');

// 预览模式
const previewKeyboardType = ref(-1); // -1 表示使用实际设备，0-2 表示预览不同型号

// 编辑器状态
const editorVisible = ref(false);
const selectedKeyIndex = ref(-1);

const selectedAction = computed<KeyAction>(() => {
  if (selectedKeyIndex.value < 0) return createEmptyAction();
  return deviceStore.getKeyAction(selectedKeyIndex.value) || createEmptyAction();
});

// 静态/呼吸/闪烁模式使用的颜色 (调色盘 + hex)
const showRgbColorPicker = computed(
  () =>
    deviceStore.rgbConfig.mode === 1 ||
    deviceStore.rgbConfig.mode === 2 ||
    deviceStore.rgbConfig.mode === 3,
);
const rgbColorHex = computed({
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

// 键盘区域底部偏移（为终端面板腾出空间）
const keyboardSectionStyle = computed(() => {
  const statusBarH = 32;
  if (terminalStore.isOpen) {
    return { bottom: (terminalStore.panelHeight + statusBarH) + 'px' };
  }
  return { bottom: statusBarH + 'px' };
});

// 获取当前使用的键盘类型（预览模式或实际设备）
const currentKeyboardType = computed(() => {
  if (previewKeyboardType.value >= 0) {
    return previewKeyboardType.value; // 预览模式
  }
  return deviceStore.deviceInfo?.keyboardType ?? 0; // 实际设备
});

// 获取当前层的按键数据（预览模式或实际设备）
const currentLayerKeysForDisplay = computed(() => {
  if (previewKeyboardType.value >= 0) {
    // 预览模式：创建空的动作数据
    const keyCount = KeyboardTypeInfo[currentKeyboardType.value as KeyboardType]?.keys || 4;
    return Array.from({ length: keyCount }, () => createEmptyAction());
  }
  // 实际设备：使用实际数据
  return deviceStore.currentLayerKeys;
});

// 根据键盘类型获取应该显示的层数
const availableLayers = computed(() => {
  const keyboardType = currentKeyboardType.value;
  return KeyboardTypeInfo[keyboardType as KeyboardType]?.layers || 4;
});

// 获取当前键盘的层选择布局
const layerLayout = computed<LayoutDef | null>(() => {
  const keyboardType = currentKeyboardType.value;
  return getLayerLayoutByType(keyboardType);
});

// 将按键索引映射到层索引（用于层选择面板）
function getLayerIndexByKeyIndex(keyIndex: number, keyboardType: number): number {
  if (keyboardType === 2) { // 旋钮款
    // 旋钮款的层选择布局：按键顺序对应层顺序
    // index 6 (旋钮按下) -> 层 0
    // index 0 -> 层 1
    // index 2 -> 层 2
    // index 1 -> 层 3
    // index 3 -> 层 4
    const mapping: Record<number, number> = {
      6: 0, // 旋钮按下 -> 层 0
      0: 1, // 按键0 -> 层 1
      2: 2, // 按键2 -> 层 2
      1: 3, // 按键1 -> 层 3
      3: 4, // 按键3 -> 层 4
    };
    return mapping[keyIndex] ?? 0;
  } else {
    // 基础款和五键款：按键索引直接对应层索引
    return keyIndex;
  }
}

// 获取按键样式（用于缩小版键盘）
function getKeyStyle(key: any) {
  const style: Record<string, string> = {};
  style.gridRow = `${key.row + 1} / span ${key.size === '2u-v' ? 2 : 1}`;
  style.gridColumn = `${key.col + 1} / span ${key.size === '2u-h' ? 2 : 1}`;
  return style;
}

// 判断层按键是否禁用（旋钮款的旋钮位置禁用）
function isLayerKeyDisabled(key: any, keyboardType: number): boolean {
  // 旋钮款（type 2）的旋钮按下位置（encoder-press）禁用
  return keyboardType === 2 && key.type === 'encoder-press';
}

// 层按键点击处理
function onLayerKeyClick(key: any, keyboardType: number): void {
  if (isLayerKeyDisabled(key, keyboardType)) {
    return; // 禁用的按键不响应点击
  }
  const layerIndex = getLayerIndexByKeyIndex(key.index, keyboardType);
  deviceStore.setEditLayer(layerIndex);
}

// 获取层按键的提示文本
function getLayerKeyTitle(key: any, keyboardType: number): string {
  if (isLayerKeyDisabled(key, keyboardType)) {
    return '旋钮位置无RGB，不可用作层切换';
  }
  const layerIndex = getLayerIndexByKeyIndex(key.index, keyboardType);
  return `层 ${layerIndex + 1} - 点击编辑 | FN + ${layerIndex + 1} 切换`;
}

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
// 预览模式切换
// ----------------------------------------

function onPreviewTypeChange() {
  // 切换到预览模式时，重置当前编辑层为0
  if (previewKeyboardType.value >= 0) {
    deviceStore.setEditLayer(0);
    showToast('info', '预览模式', `正在预览 ${KeyboardTypeInfo[previewKeyboardType.value as KeyboardType]?.name || '未知型号'}`);
  } else {
    // 切换回实际设备时，恢复实际设备的当前层
    if (deviceStore.deviceInfo) {
      deviceStore.setEditLayer(deviceStore.keymap.currentLayer);
    }
  }
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
        deviceStore.startStatusPolling();
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
      deviceStore.startStatusPolling();
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

async function saveFnConfig() {
  try {
    await deviceStore.saveFnKeyConfig();
    showToast('success', 'FN 键已保存', 'FN 键配置已保存到设备');
  } catch (error) {
    showToast('error', '保存失败', error instanceof Error ? error.message : '未知错误');
  }
}

async function saveRgbConfig() {
  try {
    await deviceStore.saveRgbConfig();
    showToast('success', 'RGB 已保存', 'RGB 灯效配置已保存到设备');
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
  deviceStore.stopStatusPolling();
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
  padding-bottom: 32px; /* 底部状态栏高度 */
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

  0%,
  100% {
    transform: translateY(0);
  }

  50% {
    transform: translateY(-10px);
  }
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

.device-badge.preview-mode {
  background: rgba(255, 193, 7, 0.2);
  border: 1px solid rgba(255, 193, 7, 0.4);
  color: #ffc107;
}

.device-badge.preview-mode i {
  color: #ffc107;
}

[data-theme="light"] .device-badge.preview-mode {
  background: #fef3c7;
  border-color: #fbbf24;
  color: #d97706;
}

[data-theme="light"] .device-badge.preview-mode i {
  color: #d97706;
}

.connected-icon {
  color: var(--c-success);
}

.header-right {
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.preview-mode-selector {
  display: flex;
  align-items: center;
}

.preview-select {
  padding: 0.4rem 0.6rem;
  font-size: 0.8rem;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  background: var(--c-bg-secondary);
  color: var(--c-text-primary);
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

[data-theme="light"] .preview-select:hover {
  border-color: #3b82f6;
}

[data-theme="light"] .preview-select:focus {
  border-color: #3b82f6;
  box-shadow: 0 0 0 2px rgba(59, 130, 246, 0.1);
}

/* ==========================================
   主内容
========================================== */
.app-main {
  flex: 1;
  display: flex;
  padding: 1.5rem;
  padding-bottom: 50px; /* 默认只为状态栏留出空间 */
  gap: 1.5rem;
  transition: padding-bottom 0.3s ease;
}

/* 当终端打开时，增加底部间距 */
.app-main.terminal-open {
  padding-bottom: 340px; /* 为打开的终端留出空间 (280px 终端 + 32px 状态栏 + 余量) */
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

.layer-hint {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  font-size: 0.7rem;
  color: var(--c-text-muted);
  padding: 0.5rem;
  margin-bottom: 0.75rem;
  background: var(--c-bg-tertiary);
  border-radius: var(--radius-sm);
}

.layer-hint i {
  font-size: 0.7rem;
  color: var(--c-accent);
}

/* 层颜色图例 */
.layer-legend {
  display: flex;
  gap: 1rem;
  padding: 0.5rem;
  margin-bottom: 0.75rem;
  background: var(--c-bg-tertiary);
  border-radius: var(--radius-sm);
  justify-content: center;
}

.legend-item {
  display: flex;
  align-items: center;
  gap: 0.35rem;
}

.legend-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  flex-shrink: 0;
}

.legend-dot.current-dot {
  background: #22c55e;
  box-shadow: 0 0 6px rgba(34, 197, 94, 0.4);
}

.legend-dot.edit-dot {
  background: #f59e0b;
  box-shadow: 0 0 6px rgba(245, 158, 11, 0.4);
}

.legend-text {
  font-size: 0.7rem;
  color: var(--c-text-secondary);
  font-weight: 500;
}

/* 缩小版键盘布局 */
.layer-keyboard-mini {
  display: grid;
  gap: 0.3rem;
  margin-bottom: 0.75rem;
  padding: 0.5rem;
  background: var(--c-bg-tertiary);
  border-radius: var(--radius-sm);
  border: 1px solid var(--c-border-light);
  aspect-ratio: auto;
  width: 100%;
  max-width: 200px;
  margin-left: auto;
  margin-right: auto;
}

.layer-keyboard-mini-placeholder {
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 1rem;
  color: var(--c-text-muted);
  font-size: 0.75rem;
}

.layer-key-mini {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background: var(--c-bg-secondary);
  border: 1.5px solid var(--c-border);
  border-radius: calc(var(--radius-sm) * 0.8);
  cursor: pointer;
  transition: all var(--transition-fast);
  position: relative;
  min-height: 30px;
  min-width: 30px;
}

/* 按键尺寸 */
.layer-key-mini.key-1u {
  aspect-ratio: 1;
}

.layer-key-mini.key-2u-h {
  aspect-ratio: 2;
}

.layer-key-mini.key-2u-v {
  height: 100%;
  /* 确保填满grid行 */
  min-height: calc(2 * 30px + 0.3rem);
  /* 2个按键高度 + gap */
}

/* 旋钮按下按键样式 */
.layer-key-mini.key-encoder-press {
  aspect-ratio: 1;
  border-radius: 50%;
}

.layer-key-mini:hover {
  border-color: var(--c-accent);
  transform: translateY(-1px);
  box-shadow: 0 1px 4px var(--c-key-shadow);
}

/* 当前层（键盘硬件状态）- 绿色 */
.layer-key-mini.current-layer {
  background: rgba(34, 197, 94, 0.1);
  border-color: #22c55e;
  box-shadow: 0 0 0 1.5px rgba(34, 197, 94, 0.15);
}

/* 编辑层（软件UI状态）- 橙色 */
.layer-key-mini.edit-layer {
  background: rgba(245, 158, 11, 0.1);
  border-color: #f59e0b;
  box-shadow: 0 0 0 1.5px rgba(245, 158, 11, 0.15);
}

/* 当前层和编辑层是同一层时 - 混合渐变效果 */
.layer-key-mini.current-layer.edit-layer {
  background: linear-gradient(135deg, rgba(34, 197, 94, 0.15) 0%, rgba(245, 158, 11, 0.15) 100%);
  border: 2px solid transparent;
  background-clip: padding-box;
  position: relative;
}

.layer-key-mini.current-layer.edit-layer::before {
  content: '';
  position: absolute;
  inset: -2px;
  border-radius: calc(var(--radius-sm) * 0.8);
  padding: 2px;
  background: linear-gradient(135deg, #22c55e 0%, #f59e0b 100%);
  -webkit-mask: linear-gradient(#fff 0 0) content-box, linear-gradient(#fff 0 0);
  -webkit-mask-composite: xor;
  mask-composite: exclude;
  pointer-events: none;
}

/* 旋钮按键的圆形渐变边框 */
.layer-key-mini.key-encoder-press.current-layer.edit-layer::before {
  border-radius: 50%;
}

.layer-key-mini.disabled {
  opacity: 0.35;
  cursor: not-allowed;
  background: var(--c-bg-tertiary) !important;
  border-color: var(--c-border-light) !important;
  box-shadow: none !important;
}

.layer-key-mini.disabled:hover {
  transform: none;
  border-color: var(--c-border-light) !important;
}

.encoder-label {
  font-size: 0.75rem !important;
}

.layer-key-number {
  font-size: 0.9rem;
  font-weight: 700;
  color: var(--c-accent);
  line-height: 1;
}

.layer-key-mini.current-layer .layer-key-number {
  color: #22c55e;
  font-size: 1rem;
}

.layer-key-mini.edit-layer .layer-key-number {
  color: #f59e0b;
  font-size: 1rem;
}

.layer-key-mini.current-layer.edit-layer .layer-key-number {
  background: linear-gradient(135deg, #22c55e 0%, #f59e0b 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
  font-size: 1.05rem;
  font-weight: 800;
}

.layer-key-label {
  font-size: 0.55rem;
  color: var(--c-text-muted);
  margin-top: 0.15rem;
  font-weight: 600;
}

.layer-key-mini.current-layer .layer-key-label {
  color: #22c55e;
}

.layer-key-mini.edit-layer .layer-key-label {
  color: #f59e0b;
}

.layer-key-mini.current-layer.edit-layer .layer-key-label {
  background: linear-gradient(135deg, #22c55e 0%, #f59e0b 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
}

.layer-info {
  margin-top: 0.5rem;
}

.current-layer-badge {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0.5rem 0.75rem;
  background: var(--c-accent-soft);
  border-radius: var(--radius-sm);
}

.layer-label {
  font-size: 0.75rem;
  color: var(--c-text-muted);
  font-weight: 600;
}

.layer-number {
  font-size: 0.9rem;
  font-weight: 700;
  color: var(--c-accent);
}

/* 浅色模式优化 */
[data-theme="light"] .current-layer-badge {
  background: #e0f2fe;
}

[data-theme="light"] .layer-number {
  color: #1d4ed8;
}

[data-theme="light"] .layer-keyboard-mini {
  background: #f8fafc;
  border-color: #e2e8f0;
}

[data-theme="light"] .layer-key-mini {
  background: #ffffff;
  border-color: #cbd5e1;
}

[data-theme="light"] .layer-key-mini:hover {
  border-color: #3b82f6;
  box-shadow: 0 2px 8px rgba(59, 130, 246, 0.2);
}

/* 亮色主题 - 当前层 */
[data-theme="light"] .layer-key-mini.current-layer {
  background: rgba(34, 197, 94, 0.15);
  border-color: #16a34a;
  box-shadow: 0 0 0 2px rgba(34, 197, 94, 0.1);
}

/* 亮色主题 - 编辑层 */
[data-theme="light"] .layer-key-mini.edit-layer {
  background: rgba(245, 158, 11, 0.15);
  border-color: #d97706;
  box-shadow: 0 0 0 2px rgba(245, 158, 11, 0.1);
}

/* 亮色主题 - 同时是当前层和编辑层 */
[data-theme="light"] .layer-key-mini.current-layer.edit-layer {
  background: linear-gradient(135deg, rgba(34, 197, 94, 0.2) 0%, rgba(245, 158, 11, 0.2) 100%);
}

[data-theme="light"] .layer-key-mini.current-layer.edit-layer::before {
  background: linear-gradient(135deg, #16a34a 0%, #d97706 100%);
}

[data-theme="light"] .layer-key-mini.current-layer .layer-key-number,
[data-theme="light"] .layer-key-mini.current-layer .layer-key-label {
  color: #16a34a;
}

[data-theme="light"] .layer-key-mini.edit-layer .layer-key-number,
[data-theme="light"] .layer-key-mini.edit-layer .layer-key-label {
  color: #d97706;
}

[data-theme="light"] .layer-key-mini.current-layer.edit-layer .layer-key-number,
[data-theme="light"] .layer-key-mini.current-layer.edit-layer .layer-key-label {
  background: linear-gradient(135deg, #16a34a 0%, #d97706 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
}

[data-theme="light"] .card-layer-badge {
  background: #dbeafe;
  border-color: #93c5fd;
  color: #1d4ed8;
}

/* 响应式 */
@media (max-width: 768px) {
  .layer-keyboard-mini {
    gap: 0.25rem;
    padding: 0.4rem;
    max-width: 160px;
  }

  .layer-key-mini {
    min-height: 28px;
    min-width: 28px;
  }

  .layer-key-number {
    font-size: 0.8rem;
  }

  .layer-key-mini.current-layer .layer-key-number,
  .layer-key-mini.edit-layer .layer-key-number {
    font-size: 0.9rem;
  }

  .layer-key-mini.current-layer.edit-layer .layer-key-number {
    font-size: 0.95rem;
  }

  .layer-key-label {
    font-size: 0.5rem;
  }
}

@media (max-width: 480px) {
  .layer-keyboard-mini {
    gap: 0.2rem;
    padding: 0.35rem;
    max-width: 140px;
  }

  .layer-key-mini {
    min-height: 25px;
    min-width: 25px;
  }
}

/* FN 键配置 */
.fn-config {
  display: flex;
  flex-direction: column;
  gap: 0.75rem;
}

.fn-group {
  display: flex;
  flex-direction: column;
  gap: 0.5rem;
  padding: 0.75rem;
  background: var(--c-bg-tertiary);
  border-radius: var(--radius-sm);
  border: 1px solid var(--c-border-light);
}

.fn-group-title {
  font-size: 0.75rem;
  font-weight: 700;
  color: var(--c-accent);
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

.fn-item {
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.fn-label {
  font-size: 0.75rem;
  font-weight: 600;
  color: var(--c-text-muted);
  min-width: 32px;
}

.fn-select {
  flex: 1;
  padding: 0.5rem 0.75rem;
  font-size: 0.85rem;
  font-weight: 500;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  background: var(--c-bg-tertiary);
  color: var(--c-text-primary);
  cursor: pointer;
  outline: none;
  transition: all var(--transition-fast);
}

.fn-select:hover {
  border-color: var(--c-accent);
}

.fn-select:focus {
  border-color: var(--c-accent);
  box-shadow: 0 0 0 2px var(--c-accent-soft);
}

.fn-save-btn {
  margin-top: 0.25rem;
}

/* RGB 灯效配置 */
.rgb-config {
  display: flex;
  flex-direction: column;
  gap: 0.75rem;
}

.rgb-item {
  display: flex;
  flex-direction: column;
  gap: 0.25rem;
}

.rgb-label {
  font-size: 0.75rem;
  font-weight: 600;
  color: var(--c-text-muted);
}

.rgb-select {
  width: 100%;
  padding: 0.5rem 0.75rem;
  font-size: 0.85rem;
  font-weight: 500;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  background: var(--c-bg-tertiary);
  color: var(--c-text-primary);
  cursor: pointer;
  outline: none;
}

.rgb-slider {
  width: 100%;
  height: 6px;
  border-radius: 3px;
  background: var(--c-bg-tertiary);
  outline: none;
  -webkit-appearance: none;
  appearance: none;
}

.rgb-slider::-webkit-slider-thumb {
  -webkit-appearance: none;
  width: 16px;
  height: 16px;
  border-radius: 50%;
  background: var(--c-accent);
  cursor: pointer;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.2);
}

.rgb-save-btn {
  margin-top: 0.25rem;
}

.rgb-color-row .rgb-label {
  margin-bottom: 0.25rem;
}

.rgb-color-controls {
  display: flex;
  flex-direction: column;
  gap: 0.5rem;
}

.rgb-color-picker {
  --p-colorpicker-preview-width: 2rem;
  --p-colorpicker-preview-height: 2rem;
}

.rgb-hex-input {
  width: 100%;
  padding: 0.5rem 0.75rem;
  font-size: 0.85rem;
  font-family: monospace;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  background: var(--c-bg-tertiary);
  color: var(--c-text-primary);
  outline: none;
}

.rgb-hex-input:focus {
  border-color: var(--c-accent);
}

/* 浅色模式 FN select */
[data-theme="light"] .fn-select {
  background: #f8fafc;
  border-color: #cbd5e1;
  color: #1e293b;
}

[data-theme="light"] .fn-select:hover,
[data-theme="light"] .fn-select:focus {
  border-color: #3b82f6;
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

/* 键盘区占位，保持 flex 布局 */
.keyboard-spacer {
  flex: 1;
  min-width: 200px;
}

/* ==========================================
   键盘区域 - 始终悬浮在屏幕正中间
========================================== */
.keyboard-section {
  position: fixed;
  left: var(--sidebar-width);
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

/* 装饰背景 */
.keyboard-decoration {
  position: absolute;
  inset: 0;
  pointer-events: none;
  overflow: hidden;
}

.deco-circle {
  position: absolute;
  border-radius: 50%;
  opacity: 0.1;
  background: var(--c-accent);
}

.deco-1 {
  width: 300px;
  height: 300px;
  top: -100px;
  right: -50px;
  animation: float 8s ease-in-out infinite;
}

.deco-2 {
  width: 180px;
  height: 180px;
  top: 60%;
  left: 0;
  animation: float 6s ease-in-out infinite reverse;
}

.deco-3 {
  width: 120px;
  height: 120px;
  top: 40%;
  right: 3%;
  animation: float 10s ease-in-out infinite;
}

/* 贯穿屏幕的脚印动画 - S形曲线路径 */
.paw-walk {
  position: absolute;
  font-size: 1rem;
  opacity: 0;
}

/* 9个脚印沿S曲线分布 - 步长紧凑 */
.paw-w1 {
  bottom: 20%;
  left: 14%;
  --paw-angle: 22deg;
  animation: pawStep 18s ease-in-out infinite 0s;
}

.paw-w2 {
  bottom: 24%;
  left: 18%;
  --paw-angle: 28deg;
  animation: pawStep 18s ease-in-out infinite 1.2s;
}

.paw-w3 {
  bottom: 29%;
  left: 23%;
  --paw-angle: 38deg;
  animation: pawStep 18s ease-in-out infinite 2.4s;
}

.paw-w4 {
  bottom: 33%;
  left: 28%;
  --paw-angle: 52deg;
  animation: pawStep 18s ease-in-out infinite 3.6s;
}

.paw-w5 {
  bottom: 35%;
  left: 34%;
  --paw-angle: 72deg;
  animation: pawStep 18s ease-in-out infinite 4.8s;
}

.paw-w6 {
  bottom: 37%;
  left: 40%;
  --paw-angle: 58deg;
  animation: pawStep 18s ease-in-out infinite 6s;
}

.paw-w7 {
  bottom: 42%;
  left: 45%;
  --paw-angle: 40deg;
  animation: pawStep 18s ease-in-out infinite 7.2s;
}

.paw-w8 {
  bottom: 48%;
  left: 50%;
  --paw-angle: 28deg;
  animation: pawStep 18s ease-in-out infinite 8.4s;
}

.paw-w9 {
  bottom: 54%;
  left: 55%;
  --paw-angle: 22deg;
  animation: pawStep 18s ease-in-out infinite 9.6s;
}

@keyframes pawStep {
  0% {
    opacity: 0;
    transform: rotate(var(--paw-angle)) scale(0.5);
  }

  5% {
    opacity: 0.32;
    transform: rotate(var(--paw-angle)) scale(1);
  }

  18% {
    opacity: 0.28;
    transform: rotate(var(--paw-angle)) scale(1);
  }

  28% {
    opacity: 0;
    transform: rotate(var(--paw-angle)) scale(0.8);
  }

  100% {
    opacity: 0;
    transform: rotate(var(--paw-angle)) scale(0.8);
  }
}

.deco-star {
  position: absolute;
  font-size: 1.5rem;
  opacity: 0.2;
  animation: twinkle 2s ease-in-out infinite;
}

.deco-star-1 {
  top: 25%;
  right: 20%;
}

.deco-star-2 {
  bottom: 30%;
  left: 15%;
  animation-delay: 1s;
}

/* Emoji 装饰图标 */
.deco-emoji {
  position: absolute;
  font-size: 3rem;
  opacity: 0.25;
  filter: grayscale(30%);
  user-select: none;
  pointer-events: none;
}

.deco-cat-1 {
  top: 8%;
  left: 3%;
  font-size: 3.5rem;
  animation: emojiFloat 4s ease-in-out infinite;
}

.deco-cat-2 {
  bottom: 12%;
  right: 6%;
  font-size: 3rem;
  animation: emojiFloat 5s ease-in-out infinite 1s;
}

.deco-cat-3 {
  top: 50%;
  right: 5%;
  font-size: 2.5rem;
  opacity: 0.18;
  animation: emojiFloat 6s ease-in-out infinite 0.5s;
}

@keyframes emojiFloat {

  0%,
  100% {
    transform: translateY(0) rotate(-5deg);
  }

  50% {
    transform: translateY(-15px) rotate(5deg);
  }
}

/* 小装饰元素 */
.deco-mini {
  position: absolute;
  opacity: 0.2;
  user-select: none;
  pointer-events: none;
}

.deco-yarn {
  font-size: 1.3rem;
  top: 65%;
  left: 3%;
  animation: yarnRoll 5s ease-in-out infinite;
}

.deco-star-3 {
  font-size: 1.2rem;
  top: 30%;
  right: 5%;
  animation: twinkle 2s ease-in-out infinite 0.3s;
}

@keyframes yarnRoll {

  0%,
  100% {
    transform: rotate(0deg);
  }

  50% {
    transform: rotate(20deg) translateX(5px);
  }
}

@keyframes twinkle {

  0%,
  100% {
    opacity: 0.2;
    transform: scale(1);
  }

  50% {
    opacity: 0.4;
    transform: scale(1.2);
  }
}

/* 键盘卡片 */
.keyboard-card {
  position: relative;
  z-index: 1;
  background: var(--c-bg-secondary);
  border: 2px solid var(--c-border);
  border-radius: var(--radius-xl);
  overflow: hidden;
  box-shadow:
    0 8px 32px rgba(0, 0, 0, 0.2),
    0 0 0 1px rgba(255, 255, 255, 0.05) inset;
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
  padding: 1.5rem;
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
</style>
