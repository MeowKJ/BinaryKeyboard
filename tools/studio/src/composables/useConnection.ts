/**
 * 连接状态管理 composable
 * 管理设备连接状态机和相关操作
 */
import { ref, watch, computed } from 'vue';
import { useDeviceStore } from '@/stores/deviceStore';
import { HidService, hidService } from '@/services/HidService';
import { showToast } from '@/services/toastService';
import { useTheme } from '@/composables/useTheme';

export type ViewPhase = 'welcome' | 'connecting' | 'connected';

export function useConnection() {
  const deviceStore = useDeviceStore();
  const { syncFromVersion } = useTheme();

  const viewPhase = ref<ViewPhase>('welcome');
  const welcomeReturning = ref(false);

  function onConnectionResult(success: boolean) {
    if (success && deviceStore.isConnected) {
      viewPhase.value = 'connected';
      // 自动同步版本配色
      const fwVer = deviceStore.firmwareVersion;
      if (fwVer && fwVer !== '0.0.0') syncFromVersion(fwVer);
    } else {
      welcomeReturning.value = true;
      viewPhase.value = 'welcome';
      setTimeout(() => { welcomeReturning.value = false; }, 600);
    }
  }

  watch(() => deviceStore.isConnected, (connected) => {
    if (!connected && viewPhase.value === 'connected') {
      viewPhase.value = 'welcome';
    }
  });

  async function requestDevice() {
    try {
      const device = await hidService.requestDevice();
      if (!device) return;

      viewPhase.value = 'connecting';

      const success = await deviceStore.connectDevice(device);
      if (success) {
        deviceStore.startStatusPolling();
        showToast('success', '连接成功', `已连接到 ${device.productName}`);
      } else {
        showToast('error', '连接失败', deviceStore.errorMessage || '无法连接设备');
      }
      onConnectionResult(success);
    } catch (error) {
      showToast('error', '连接失败', error instanceof Error ? error.message : '未知错误');
      onConnectionResult(false);
    }
  }

  async function autoConnect() {
    const device = await hidService.getAuthorizedDevice();
    if (!device) return;

    const success = await deviceStore.connectDevice(device);
    if (!success) return;

    deviceStore.startStatusPolling();
    showToast('success', '自动连接', `已连接到 ${device.productName}`);
    onConnectionResult(true);
  }

  async function disconnect() {
    await deviceStore.disconnectDevice();
    viewPhase.value = 'welcome';
    showToast('info', '已断开', '设备连接已关闭');
  }

  async function refreshAll() {
    try {
      await deviceStore.refreshKeymap();
      if (deviceStore.supportsRgb) {
        await deviceStore.refreshRgbConfig();
      }
      if (deviceStore.supportsFnKeys) {
        await deviceStore.refreshFnKeyConfig();
      }
      showToast('success', '刷新成功', '配置已从设备重新加载');
    } catch (error) {
      showToast('error', '刷新失败', error instanceof Error ? error.message : '未知错误');
    }
  }

  function onDeviceDisconnected(event: HIDConnectionEvent) {
    if (event.device === deviceStore.device) {
      deviceStore.device = null;
      showToast('warn', '设备断开', '键盘连接已丢失');
    }
  }

  async function onDeviceConnected(_event: HIDConnectionEvent) {
    // 固件刷写后设备重新枚举，自动重连（仅在未连接时触发）
    if (deviceStore.isConnected) return;
    // 给设备 USB 枚举后留一点稳定时间再尝试连接
    await new Promise((r) => setTimeout(r, 800));
    if (deviceStore.isConnected) return;
    await autoConnect();
  }

  function setupHidListeners() {
    if (HidService.isSupported()) {
      navigator.hid.addEventListener('disconnect', onDeviceDisconnected);
      navigator.hid.addEventListener('connect', onDeviceConnected);
    }
  }

  function teardownHidListeners() {
    deviceStore.stopStatusPolling();
    if (HidService.isSupported()) {
      navigator.hid.removeEventListener('disconnect', onDeviceDisconnected);
      navigator.hid.removeEventListener('connect', onDeviceConnected);
    }
  }

  const isConnected = computed(() => viewPhase.value === 'connected');
  const isConnecting = computed(() => viewPhase.value === 'connecting');

  return {
    viewPhase,
    welcomeReturning,
    isConnected,
    isConnecting,
    requestDevice,
    autoConnect,
    disconnect,
    refreshAll,
    setupHidListeners,
    teardownHidListeners,
  };
}
