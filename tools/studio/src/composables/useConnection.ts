/**
 * 连接状态管理 composable
 * 管理设备连接状态机和相关操作
 */
import { ref, watch, computed } from "vue";
import { useDeviceStore } from "@/stores/deviceStore";
import { HidService, hidService } from "@/services/HidService";
import { showToast } from "@/services/toastService";
import { useTheme } from "@/composables/useTheme";

export type ViewPhase = "welcome" | "connecting" | "connected";

const INIT_TIMEOUT_MS = 15000;
const CONNECT_RETRY_COUNT = 3;
const CONNECT_RETRY_DELAY_MS = 900;

export function useConnection() {
  const deviceStore = useDeviceStore();
  const { syncFromVersion } = useTheme();

  const viewPhase = ref<ViewPhase>("welcome");
  const welcomeReturning = ref(false);

  function onConnectionResult(success: boolean) {
    if (success && deviceStore.isConnected) {
      viewPhase.value = "connected";
      // 自动同步版本配色
      const fwVer = deviceStore.firmwareVersion;
      if (fwVer && fwVer !== "0.0.0") syncFromVersion(fwVer);
    } else {
      welcomeReturning.value = true;
      viewPhase.value = "welcome";
      setTimeout(() => {
        welcomeReturning.value = false;
      }, 600);
    }
  }

  watch(
    () => deviceStore.isConnected,
    (connected) => {
      if (
        !connected &&
        viewPhase.value === "connected" &&
        !deviceStore.iapInProgress
      ) {
        viewPhase.value = "welcome";
      }
    },
  );

  async function connectAndInitialize(
    device: HIDDevice,
    successTitle: string,
  ): Promise<boolean> {
    let lastError: unknown = null;

    for (let attempt = 1; attempt <= CONNECT_RETRY_COUNT; attempt++) {
      const opened = await deviceStore.openDevice(device);
      if (!opened) {
        lastError = new Error(deviceStore.errorMessage || "无法连接设备");
      } else {
        // HID 打开成功后立即切到主界面，后续配置加载走 connected 页遮罩。
        viewPhase.value = "connected";

        try {
          await Promise.race([
            deviceStore.initializeConnectedDevice(),
            new Promise<never>((_, reject) =>
              setTimeout(
                () => reject(new Error("初始化超时，请重试连接")),
                INIT_TIMEOUT_MS,
              ),
            ),
          ]);
          deviceStore.startStatusPolling();
          void deviceStore.refreshMacroOverview().catch(() => {});
          showToast("success", successTitle, `已连接到 ${device.productName}`);
          onConnectionResult(true);
          return true;
        } catch (error) {
          lastError = error;
          await deviceStore.disconnectDevice();
        }
      }

      if (attempt < CONNECT_RETRY_COUNT) {
        await new Promise((r) => setTimeout(r, CONNECT_RETRY_DELAY_MS));
      }
    }

    showToast(
      "error",
      "连接失败",
      lastError instanceof Error ? lastError.message : "未知错误",
    );
    onConnectionResult(false);
    return false;
  }

  async function connectAuthorizedDevice(
    successTitle: string,
  ): Promise<boolean> {
    const device = await hidService.getAuthorizedDevice();
    if (!device) {
      showToast("error", "连接失败", "未找到已授权设备");
      onConnectionResult(false);
      return false;
    }
    return connectAndInitialize(device, successTitle);
  }

  async function requestDevice() {
    try {
      const requestedDevice = await hidService.requestDevice();
      if (!requestedDevice) return;

      viewPhase.value = "connecting";
      // 手动连接只负责授权，真正连接流程与自动重连保持完全一致。
      await new Promise((r) => setTimeout(r, 800));
      await connectAuthorizedDevice("连接成功");
    } catch (error) {
      showToast(
        "error",
        "连接失败",
        error instanceof Error ? error.message : "未知错误",
      );
      onConnectionResult(false);
    }
  }

  async function autoConnect() {
    await connectAuthorizedDevice("自动连接");
  }

  async function disconnect() {
    await deviceStore.disconnectDevice();
    viewPhase.value = "welcome";
    showToast("info", "已断开", "设备连接已关闭");
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
      showToast("success", "刷新成功", "配置已从设备重新加载");
    } catch (error) {
      showToast(
        "error",
        "刷新失败",
        error instanceof Error ? error.message : "未知错误",
      );
    }
  }

  function onDeviceDisconnected(event: HIDConnectionEvent) {
    if (event.device === deviceStore.device) {
      if (deviceStore.iapInProgress) return;
      deviceStore.device = null;
      showToast("warn", "设备断开", "键盘连接已丢失");
    }
  }

  async function onDeviceConnected(_event: HIDConnectionEvent) {
    // 固件刷写后设备重新枚举，自动重连（仅在未连接时触发）
    if (deviceStore.isConnected) return;
    try {
      // 给设备 USB 枚举后留一点稳定时间再尝试连接
      await new Promise((r) => setTimeout(r, 800));
      if (deviceStore.isConnected) return;
      await autoConnect();
    } catch {
      // 连接事件里的失败由连接流程统一提示，这里避免未捕获 Promise
    }
  }

  function setupHidListeners() {
    if (HidService.isSupported()) {
      navigator.hid.addEventListener("disconnect", onDeviceDisconnected);
      navigator.hid.addEventListener("connect", onDeviceConnected);
    }
  }

  function teardownHidListeners() {
    deviceStore.stopStatusPolling();
    if (HidService.isSupported()) {
      navigator.hid.removeEventListener("disconnect", onDeviceDisconnected);
      navigator.hid.removeEventListener("connect", onDeviceConnected);
    }
  }

  const isConnected = computed(() => viewPhase.value === "connected");
  const isConnecting = computed(() => viewPhase.value === "connecting");

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
