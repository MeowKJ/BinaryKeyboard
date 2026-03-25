/**
 * BinaryKeyboard 设备状态管理
 */

import { defineStore } from "pinia";
import { ref, computed } from "vue";
import { hidService } from "@/services/HidService";
import { useMacroStore } from "@/stores/macroStore";
import {
  type DeviceCapabilities,
  type DeviceInfo,
  type DeviceStatus,
  type KeymapConfig,
  type RgbConfig,
  type FnKeyConfig,
  type KeyAction,
  KeyboardTypeInfo,
  createEmptyKeymap,
  createEmptyFnKeyConfig,
  createDefaultRgbConfig,
  MAX_LAYERS,
} from "@/types/protocol";

export const useDeviceStore = defineStore("device", () => {
  const EMPTY_CAPABILITIES: DeviceCapabilities = {
    multiLayer: false,
    layerKeyActions: false,
    rgb: false,
    rgbOverlay: false,
    fnKeys: false,
    macroActions: false,
    wheelClickAction: false,
    battery: false,
    logs: false,
    reset: false,
    explicitSave: false,
    wireless: false,
    iap: false,
  };

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

  /** 电池电压 (V, 如 4.12) */
  const batteryVoltage = ref(0);

  /** 加载状态 */
  const isLoading = ref(false);

  /** 错误信息 */
  const errorMessage = ref<string | null>(null);

  /** 实时轮询定时器 */
  let _pollTimer: ReturnType<typeof setInterval> | null = null;
  /** 轮询周期计数 (用于电压低频采样) */
  let _pollTick = 0;

  function cloneKeymapConfig(config: KeymapConfig): KeymapConfig {
    return JSON.parse(JSON.stringify(config)) as KeymapConfig;
  }

  function normalizeKeymapConfig(config: KeymapConfig): KeymapConfig {
    const normalized = cloneKeymapConfig(config);
    const info = deviceInfo.value;

    if (!info?.capabilities.multiLayer) {
      normalized.numLayers = 1;
      normalized.currentLayer = 0;
      normalized.defaultLayer = 0;
      return normalized;
    }

    const supportedLayers = Math.max(
      1,
      info.maxLayers ||
        KeyboardTypeInfo[info.keyboardType]?.layers ||
        normalized.numLayers ||
        1,
    );

    normalized.numLayers = Math.min(
      Math.max(normalized.numLayers || 1, 1),
      supportedLayers,
    );
    normalized.currentLayer = Math.min(
      normalized.currentLayer || 0,
      normalized.numLayers - 1,
    );
    normalized.defaultLayer = Math.min(
      normalized.defaultLayer || 0,
      normalized.numLayers - 1,
    );

    return normalized;
  }

  // ========================================
  // 计算属性
  // ========================================

  /** 是否已连接 */
  const isConnected = computed(
    () => device.value !== null && device.value.opened,
  );

  /** 当前设备能力 */
  const capabilities = computed<DeviceCapabilities>(() => {
    return deviceInfo.value?.capabilities ?? EMPTY_CAPABILITIES;
  });

  const supportsMultiLayer = computed(() => capabilities.value.multiLayer);
  const supportsLayerKeyActions = computed(
    () => capabilities.value.layerKeyActions,
  );
  const supportsRgb = computed(() => capabilities.value.rgb);
  const supportsRgbOverlay = computed(() => capabilities.value.rgbOverlay);
  const supportsFnKeys = computed(() => capabilities.value.fnKeys);
  const supportsMacroActions = computed(() => capabilities.value.macroActions);
  const supportsWheelClickAction = computed(
    () => capabilities.value.wheelClickAction,
  );
  const supportsBattery = computed(() => capabilities.value.battery);
  const supportsLogs = computed(() => capabilities.value.logs);
  const supportsFactoryReset = computed(() => capabilities.value.reset);
  const supportsExplicitSave = computed(() => capabilities.value.explicitSave);
  const supportsWireless = computed(() => capabilities.value.wireless);

  /** 键盘类型名称 */
  const keyboardTypeName = computed(() => {
    if (!deviceInfo.value) return "未知设备";
    return KeyboardTypeInfo[deviceInfo.value.keyboardType]?.name || "未知型号";
  });

  /** 实际可用键数 */
  const actualKeyCount = computed(() => {
    if (!deviceInfo.value) return 4;
    return deviceInfo.value.actualKeyCount;
  });

  /** 固件版本字符串 */
  const firmwareVersion = computed(() => {
    if (!deviceInfo.value) return "0.0.0";
    return `${deviceInfo.value.versionMajor}.${deviceInfo.value.versionMinor}.${deviceInfo.value.versionPatch}`;
  });

  /** 当前层的按键列表 */
  const currentLayerKeys = computed(() => {
    return (
      keymap.value.layers[currentEditLayer.value]?.keys.slice(
        0,
        actualKeyCount.value,
      ) || []
    );
  });

  /** 是否有未保存的更改 */
  const hasChanges = computed(() => {
    return (
      JSON.stringify(keymap.value) !== JSON.stringify(keymapOriginal.value)
    );
  });

  /** 设备信息列表 (用于 UI 显示) */
  const deviceInfoList = computed(() => {
    if (!deviceInfo.value) return [];
    return [
      { key: "芯片家族", value: deviceInfo.value.chipFamily },
      { key: "型号名称", value: keyboardTypeName.value },
      { key: "按键数量", value: `${actualKeyCount.value} 键` },
      { key: "固件版本", value: `v${firmwareVersion.value}` },
    ];
  });

  // ========================================
  // 方法
  // ========================================

  /** 连接设备 */
  async function connectDevice(hidDevice: HIDDevice): Promise<boolean> {
    const opened = await openDevice(hidDevice);
    if (!opened) {
      return false;
    }

    try {
      await initializeConnectedDevice();
      if (supportsMacroActions.value) {
        await useMacroStore().refreshOverview().catch(() => {});
      }
      return true;
    } catch {
      return false;
    }
  }

  function resetDeviceSession(): void {
    device.value = null;
    deviceInfo.value = null;
    deviceStatus.value = null;
    batteryVoltage.value = 0;
    keymap.value = createEmptyKeymap();
    keymapOriginal.value = createEmptyKeymap();
    rgbConfig.value = createDefaultRgbConfig();
    fnKeyConfig.value = createEmptyFnKeyConfig();
    currentEditLayer.value = 0;
    useMacroStore().reset();
  }

  async function openDevice(hidDevice: HIDDevice): Promise<boolean> {
    isLoading.value = true;
    errorMessage.value = null;
    const macroStore = useMacroStore();

    try {
      macroStore.reset();
      const success = await hidService.connect(hidDevice);
      if (!success) {
        throw new Error("无法打开设备");
      }

      device.value = hidDevice;
      return true;
    } catch (error) {
      errorMessage.value = error instanceof Error ? error.message : "连接失败";
      try {
        await hidService.disconnect();
      } catch {
        // ignore disconnect cleanup errors
      }
      resetDeviceSession();
      isLoading.value = false;
      return false;
    }
  }

  async function initializeConnectedDevice(): Promise<void> {
    if (!device.value) {
      isLoading.value = false;
      throw new Error("设备未连接");
    }

    errorMessage.value = null;

    try {
      await refreshDeviceInfo();
      await refreshKeymap();
      if (supportsRgb.value) {
        await refreshRgbConfig();
      } else {
        rgbConfig.value = createDefaultRgbConfig();
      }
      if (supportsFnKeys.value) {
        await refreshFnKeyConfig();
      } else {
        fnKeyConfig.value = createEmptyFnKeyConfig();
      }
    } catch (error) {
      errorMessage.value = error instanceof Error ? error.message : "连接失败";
      try {
        await hidService.disconnect();
      } catch {
        // ignore disconnect cleanup errors
      }
      resetDeviceSession();
      throw error instanceof Error ? error : new Error("连接失败");
    } finally {
      isLoading.value = false;
    }
  }

  async function refreshMacroOverview(): Promise<void> {
    if (!supportsMacroActions.value) {
      useMacroStore().reset();
      return;
    }
    await useMacroStore().refreshOverview();
  }

  /** 断开设备 */
  async function disconnectDevice(): Promise<void> {
    stopStatusPolling();
    await hidService.disconnect();
    resetDeviceSession();
    isLoading.value = false;
  }

  /** 刷新设备信息 */
  async function refreshDeviceInfo(): Promise<void> {
    deviceInfo.value = await hidService.getSysInfo();
    deviceStatus.value = await hidService.getSysStatus();
  }

  /** 刷新按键映射 */
  async function refreshKeymap(): Promise<void> {
    const config = normalizeKeymapConfig(await hidService.getFullKeymap());
    keymap.value = config;
    keymapOriginal.value = cloneKeymapConfig(config);
    currentEditLayer.value = config.currentLayer;
  }

  /** 刷新 RGB 配置 */
  async function refreshRgbConfig(): Promise<void> {
    if (!supportsRgb.value) {
      rgbConfig.value = createDefaultRgbConfig();
      return;
    }
    rgbConfig.value = await hidService.getRgbConfig();
  }

  /** 刷新 FN 键配置 */
  async function refreshFnKeyConfig(): Promise<void> {
    if (!supportsFnKeys.value) {
      fnKeyConfig.value = createEmptyFnKeyConfig();
      return;
    }
    fnKeyConfig.value = await hidService.getFnKeyConfig();
  }

  /** 保存按键映射到设备 */
  async function saveKeymap(): Promise<void> {
    isLoading.value = true;
    errorMessage.value = null;

    try {
      await hidService.setFullKeymap(keymap.value);
      if (supportsExplicitSave.value) {
        await hidService.saveConfig();
      }
      keymapOriginal.value = cloneKeymapConfig(keymap.value);
    } catch (error) {
      errorMessage.value = error instanceof Error ? error.message : "保存失败";
      throw error;
    } finally {
      isLoading.value = false;
    }
  }

  /** 保存 RGB 配置到设备 */
  async function saveRgbConfig(): Promise<void> {
    if (!supportsRgb.value) {
      throw new Error("当前设备不支持 RGB 配置");
    }
    isLoading.value = true;
    errorMessage.value = null;

    try {
      await hidService.setRgbConfig(rgbConfig.value);
      if (supportsExplicitSave.value) {
        await hidService.saveConfig();
      }
    } catch (error) {
      errorMessage.value = error instanceof Error ? error.message : "保存失败";
      throw error;
    } finally {
      isLoading.value = false;
    }
  }

  /** 保存 FN 键配置到设备 */
  async function saveFnKeyConfig(): Promise<void> {
    if (!supportsFnKeys.value) {
      throw new Error("当前设备不支持 FN 键配置");
    }
    isLoading.value = true;
    errorMessage.value = null;

    try {
      await hidService.setFnKeyConfig(fnKeyConfig.value);
      if (supportsExplicitSave.value) {
        await hidService.saveConfig();
      }
    } catch (error) {
      errorMessage.value = error instanceof Error ? error.message : "保存失败";
      throw error;
    } finally {
      isLoading.value = false;
    }
  }

  /** 重置为出厂设置 */
  async function resetToFactory(): Promise<void> {
    if (!supportsFactoryReset.value) {
      throw new Error("当前设备不支持恢复出厂");
    }
    isLoading.value = true;
    errorMessage.value = null;

    try {
      const macroStore = useMacroStore();
      await hidService.resetConfig();
      macroStore.reset();
      await refreshDeviceInfo();
      await refreshKeymap();
      if (supportsRgb.value) {
        await refreshRgbConfig();
      }
      if (supportsFnKeys.value) {
        await refreshFnKeyConfig();
      }
      if (supportsMacroActions.value) {
        await macroStore.refreshOverview().catch(() => {});
      }
    } catch (error) {
      errorMessage.value = error instanceof Error ? error.message : "重置失败";
      throw error;
    } finally {
      isLoading.value = false;
    }
  }

  /** 设置某个键的动作 */
  function setKeyAction(
    keyIndex: number,
    action: KeyAction,
    layerIndex?: number,
  ): void {
    const layer = layerIndex ?? currentEditLayer.value;
    if (
      layer >= 0 &&
      layer < keymap.value.numLayers &&
      keyIndex >= 0 &&
      keyIndex < actualKeyCount.value
    ) {
      keymap.value.layers[layer].keys[keyIndex] = { ...action };
    }
  }

  /** 获取某个键的动作 */
  function getKeyAction(
    keyIndex: number,
    layerIndex?: number,
  ): KeyAction | null {
    const layer = layerIndex ?? currentEditLayer.value;
    if (
      layer >= 0 &&
      layer < keymap.value.numLayers &&
      keyIndex >= 0 &&
      keyIndex < actualKeyCount.value
    ) {
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
    const supportedLayers = Math.max(
      1,
      deviceInfo.value?.maxLayers ||
        KeyboardTypeInfo[deviceInfo.value?.keyboardType ?? 0]?.layers ||
        1,
    );

    if (keymap.value.numLayers < Math.min(MAX_LAYERS, supportedLayers)) {
      keymap.value.numLayers++;
      return true;
    }
    return false;
  }

  /** 减少层数 */
  function removeLayer(): boolean {
    if (keymap.value.numLayers > 1) {
      keymap.value.numLayers--;
      if (keymap.value.currentLayer >= keymap.value.numLayers) {
        keymap.value.currentLayer = keymap.value.numLayers - 1;
      }
      if (keymap.value.defaultLayer >= keymap.value.numLayers) {
        keymap.value.defaultLayer = 0;
      }
      if (currentEditLayer.value >= keymap.value.numLayers) {
        currentEditLayer.value = keymap.value.numLayers - 1;
      }
      return true;
    }
    return false;
  }

  /** 放弃更改 */
  function discardChanges(): void {
    keymap.value = cloneKeymapConfig(keymapOriginal.value);
    currentEditLayer.value = keymap.value.currentLayer;
  }

  // ========================================
  // 实时轮询
  // ========================================

  /** 内部轮询: 每次取 SysStatus, 每 5 次额外取电压 */
  async function _pollStatus(): Promise<void> {
    try {
      const status = await hidService.getSysStatus();
      deviceStatus.value = status;

      // 注释掉自动同步：让编辑层和当前层独立
      // 用户可以在设备使用层5的同时，在软件上编辑层2
      // if (status.currentLayer !== currentEditLayer.value) {
      //   currentEditLayer.value = status.currentLayer;
      // }

      // 每 5 个 tick (~10s) 采样电压
      _pollTick++;
      if (supportsBattery.value && _pollTick % 5 === 0) {
        const bat = await hidService.getBattery();
        batteryVoltage.value = bat.voltage;
      }
    } catch {
      /* 轮询失败静默忽略, 下次重试 */
    }
  }

  /** 启动实时状态轮询 (2s 间隔) */
  function startStatusPolling(): void {
    stopStatusPolling();
    _pollTick = 0;
    if (supportsBattery.value) {
      hidService
        .getBattery()
        .then((bat) => {
          batteryVoltage.value = bat.voltage;
        })
        .catch(() => {});
    } else {
      batteryVoltage.value = 0;
    }
    _pollTimer = setInterval(_pollStatus, 2000);
  }

  /** 停止轮询 */
  function stopStatusPolling(): void {
    if (_pollTimer) {
      clearInterval(_pollTimer);
      _pollTimer = null;
    }
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
    batteryVoltage,
    isLoading,
    errorMessage,

    // 计算属性
    isConnected,
    capabilities,
    supportsMultiLayer,
    supportsLayerKeyActions,
    supportsRgb,
    supportsRgbOverlay,
    supportsFnKeys,
    supportsMacroActions,
    supportsWheelClickAction,
    supportsBattery,
    supportsLogs,
    supportsFactoryReset,
    supportsExplicitSave,
    supportsWireless,
    keyboardTypeName,
    actualKeyCount,
    firmwareVersion,
    currentLayerKeys,
    hasChanges,
    deviceInfoList,

    // 方法
    connectDevice,
    openDevice,
    initializeConnectedDevice,
    disconnectDevice,
    refreshDeviceInfo,
    refreshKeymap,
    refreshRgbConfig,
    refreshFnKeyConfig,
    refreshMacroOverview,
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
    startStatusPolling,
    stopStatusPolling,
  };
});
