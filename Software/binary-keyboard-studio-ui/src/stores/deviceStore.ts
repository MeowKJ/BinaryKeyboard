/**
 * MeowKeyboard 设备状态管理
 */

import { defineStore } from 'pinia';
import { ref, computed } from 'vue';
import { hidService } from '@/services/HidService';
import {
  type DeviceInfo,
  type DeviceStatus,
  type KeymapConfig,
  type RgbConfig,
  type FnKeyConfig,
  type KeyAction,
  KeyboardType,
  KeyboardTypeInfo,
  ActionType,
  createEmptyKeymap,
  createEmptyFnKeyConfig,
  createDefaultRgbConfig,
  MAX_LAYERS,
} from '@/types/protocol';

export const useDeviceStore = defineStore('device', () => {
  // ========================================
  // 状态
  // ========================================

  /** HID 设备实例 */
  const device = ref<HIDDevice | null>(null);

  /** 设备信息 */
  const deviceInfo = ref<DeviceInfo | null>(null);

  /** 设备状态 */
  const deviceStatus = ref<DeviceStatus | null>(null);

  /** 按键映射配置 */
  const keymap = ref<KeymapConfig>(createEmptyKeymap());

  /** 原始按键映射 (用于比较变更) */
  const keymapOriginal = ref<KeymapConfig>(createEmptyKeymap());

  /** RGB 配置 */
  const rgbConfig = ref<RgbConfig>(createDefaultRgbConfig());

  /** FN 键配置 */
  const fnKeyConfig = ref<FnKeyConfig>(createEmptyFnKeyConfig());

  /** 当前编辑的层索引 */
  const currentEditLayer = ref(0);

  /** 加载状态 */
  const isLoading = ref(false);

  /** 错误信息 */
  const errorMessage = ref<string | null>(null);

  // ========================================
  // 计算属性
  // ========================================

  /** 是否已连接 */
  const isConnected = computed(() => device.value !== null && device.value.opened);

  /** 键盘类型名称 */
  const keyboardTypeName = computed(() => {
    if (!deviceInfo.value) return '未知设备';
    return KeyboardTypeInfo[deviceInfo.value.keyboardType]?.name || '未知型号';
  });

  /** 实际可用键数 */
  const actualKeyCount = computed(() => {
    if (!deviceInfo.value) return 4;
    return deviceInfo.value.actualKeyCount;
  });

  /** 固件版本字符串 */
  const firmwareVersion = computed(() => {
    if (!deviceInfo.value) return '0.0.0';
    return `${deviceInfo.value.versionMajor}.${deviceInfo.value.versionMinor}.${deviceInfo.value.versionPatch}`;
  });

  /** 当前层的按键列表 */
  const currentLayerKeys = computed(() => {
    return keymap.value.layers[currentEditLayer.value]?.keys.slice(0, actualKeyCount.value) || [];
  });

  /** 是否有未保存的更改 */
  const hasChanges = computed(() => {
    return JSON.stringify(keymap.value) !== JSON.stringify(keymapOriginal.value);
  });

  /** 设备信息列表 (用于 UI 显示) */
  const deviceInfoList = computed(() => {
    if (!deviceInfo.value) return [];
    return [
      { key: '型号名称', value: keyboardTypeName.value },
      { key: '键盘类型', value: `0x${deviceInfo.value.keyboardType.toString(16).padStart(2, '0').toUpperCase()}` },
      { key: '按键数量', value: `${actualKeyCount.value} 键` },
      { key: '固件版本', value: `v${firmwareVersion.value}` },
      { key: '支持层数', value: `${deviceInfo.value.maxLayers} 层` },
      { key: 'FN 键数', value: `${deviceInfo.value.fnKeyCount} 个` },
    ];
  });

  // ========================================
  // 方法
  // ========================================

  /** 连接设备 */
  async function connectDevice(hidDevice: HIDDevice): Promise<boolean> {
    isLoading.value = true;
    errorMessage.value = null;

    try {
      const success = await hidService.connect(hidDevice);
      if (!success) {
        throw new Error('无法打开设备');
      }

      device.value = hidDevice;
      await refreshDeviceInfo();
      await refreshKeymap();
      await refreshRgbConfig();
      await refreshFnKeyConfig();

      return true;
    } catch (error) {
      errorMessage.value = error instanceof Error ? error.message : '连接失败';
      device.value = null;
      return false;
    } finally {
      isLoading.value = false;
    }
  }

  /** 断开设备 */
  async function disconnectDevice(): Promise<void> {
    await hidService.disconnect();
    device.value = null;
    deviceInfo.value = null;
    deviceStatus.value = null;
    keymap.value = createEmptyKeymap();
    keymapOriginal.value = createEmptyKeymap();
  }

  /** 刷新设备信息 */
  async function refreshDeviceInfo(): Promise<void> {
    deviceInfo.value = await hidService.getSysInfo();
    deviceStatus.value = await hidService.getSysStatus();
  }

  /** 刷新按键映射 */
  async function refreshKeymap(): Promise<void> {
    const config = await hidService.getFullKeymap();
    keymap.value = config;
    keymapOriginal.value = JSON.parse(JSON.stringify(config)); // 深拷贝
  }

  /** 刷新 RGB 配置 */
  async function refreshRgbConfig(): Promise<void> {
    rgbConfig.value = await hidService.getRgbConfig();
  }

  /** 刷新 FN 键配置 */
  async function refreshFnKeyConfig(): Promise<void> {
    fnKeyConfig.value = await hidService.getFnKeyConfig();
  }

  /** 保存按键映射到设备 */
  async function saveKeymap(): Promise<void> {
    isLoading.value = true;
    errorMessage.value = null;

    try {
      await hidService.setFullKeymap(keymap.value);
      await hidService.saveConfig();
      keymapOriginal.value = JSON.parse(JSON.stringify(keymap.value));
    } catch (error) {
      errorMessage.value = error instanceof Error ? error.message : '保存失败';
      throw error;
    } finally {
      isLoading.value = false;
    }
  }

  /** 保存 RGB 配置到设备 */
  async function saveRgbConfig(): Promise<void> {
    isLoading.value = true;
    errorMessage.value = null;

    try {
      await hidService.setRgbConfig(rgbConfig.value);
      await hidService.saveConfig();
    } catch (error) {
      errorMessage.value = error instanceof Error ? error.message : '保存失败';
      throw error;
    } finally {
      isLoading.value = false;
    }
  }

  /** 保存 FN 键配置到设备 */
  async function saveFnKeyConfig(): Promise<void> {
    isLoading.value = true;
    errorMessage.value = null;

    try {
      await hidService.setFnKeyConfig(fnKeyConfig.value);
      await hidService.saveConfig();
    } catch (error) {
      errorMessage.value = error instanceof Error ? error.message : '保存失败';
      throw error;
    } finally {
      isLoading.value = false;
    }
  }

  /** 重置为出厂设置 */
  async function resetToFactory(): Promise<void> {
    isLoading.value = true;
    errorMessage.value = null;

    try {
      await hidService.resetConfig();
      await refreshKeymap();
      await refreshRgbConfig();
      await refreshFnKeyConfig();
    } catch (error) {
      errorMessage.value = error instanceof Error ? error.message : '重置失败';
      throw error;
    } finally {
      isLoading.value = false;
    }
  }

  /** 设置某个键的动作 */
  function setKeyAction(keyIndex: number, action: KeyAction, layerIndex?: number): void {
    const layer = layerIndex ?? currentEditLayer.value;
    if (layer >= 0 && layer < MAX_LAYERS && keyIndex >= 0 && keyIndex < actualKeyCount.value) {
      keymap.value.layers[layer].keys[keyIndex] = { ...action };
    }
  }

  /** 获取某个键的动作 */
  function getKeyAction(keyIndex: number, layerIndex?: number): KeyAction | null {
    const layer = layerIndex ?? currentEditLayer.value;
    if (layer >= 0 && layer < MAX_LAYERS && keyIndex >= 0 && keyIndex < actualKeyCount.value) {
      return keymap.value.layers[layer].keys[keyIndex];
    }
    return null;
  }

  /** 切换编辑层 */
  function setEditLayer(layerIndex: number): void {
    if (layerIndex >= 0 && layerIndex < keymap.value.numLayers) {
      currentEditLayer.value = layerIndex;
    }
  }

  /** 增加层数 */
  function addLayer(): boolean {
    if (keymap.value.numLayers < MAX_LAYERS) {
      keymap.value.numLayers++;
      return true;
    }
    return false;
  }

  /** 减少层数 */
  function removeLayer(): boolean {
    if (keymap.value.numLayers > 1) {
      keymap.value.numLayers--;
      if (currentEditLayer.value >= keymap.value.numLayers) {
        currentEditLayer.value = keymap.value.numLayers - 1;
      }
      return true;
    }
    return false;
  }

  /** 放弃更改 */
  function discardChanges(): void {
    keymap.value = JSON.parse(JSON.stringify(keymapOriginal.value));
  }

  return {
    // 状态
    device,
    deviceInfo,
    deviceStatus,
    keymap,
    keymapOriginal,
    rgbConfig,
    fnKeyConfig,
    currentEditLayer,
    isLoading,
    errorMessage,

    // 计算属性
    isConnected,
    keyboardTypeName,
    actualKeyCount,
    firmwareVersion,
    currentLayerKeys,
    hasChanges,
    deviceInfoList,

    // 方法
    connectDevice,
    disconnectDevice,
    refreshDeviceInfo,
    refreshKeymap,
    refreshRgbConfig,
    refreshFnKeyConfig,
    saveKeymap,
    saveRgbConfig,
    saveFnKeyConfig,
    resetToFactory,
    setKeyAction,
    getKeyAction,
    setEditLayer,
    addLayer,
    removeLayer,
    discardChanges,
  };
});
