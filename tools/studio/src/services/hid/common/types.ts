import type {
  DeviceInfo,
  DeviceStatus,
  KeymapConfig,
  RgbConfig,
  FnKeyConfig,
  LogConfig,
  DeviceProtocol,
} from '@/types/protocol';
import type { DeviceUiProvider } from '@/types/deviceUi';

export interface BatteryInfo {
  level: number;
  voltage: number;
  isCharging: boolean;
}

export interface HidOptionalOperations {
  getRgbConfig?: () => Promise<RgbConfig>;
  setRgbConfig?: (config: RgbConfig) => Promise<void>;
  getFnKeyConfig?: () => Promise<FnKeyConfig>;
  setFnKeyConfig?: (config: FnKeyConfig) => Promise<void>;
  saveConfig?: () => Promise<void>;
  loadConfig?: () => Promise<void>;
  resetConfig?: () => Promise<void>;
  getBattery?: () => Promise<BatteryInfo>;
  getLogConfig?: () => Promise<LogConfig>;
  setLogConfig?: (config: LogConfig) => Promise<void>;
}

export const OPTIONAL_OPERATION_LABELS: Record<keyof HidOptionalOperations, string> = {
  getRgbConfig: 'RGB 配置读取',
  setRgbConfig: 'RGB 配置写入',
  getFnKeyConfig: 'FN 键配置读取',
  setFnKeyConfig: 'FN 键配置写入',
  saveConfig: '配置保存',
  loadConfig: '配置加载',
  resetConfig: '恢复出厂设置',
  getBattery: '电池状态读取',
  getLogConfig: '日志配置读取',
  setLogConfig: '日志配置写入',
};

export interface HidAdapter {
  readonly protocol: DeviceProtocol;
  readonly filters: HIDDeviceFilter[];
  readonly optional: HidOptionalOperations;

  matches(device: HIDDevice): boolean;
  connect(device: HIDDevice): Promise<boolean>;
  disconnect(): Promise<void>;
  getDevice(): HIDDevice | null;
  isConnected(): boolean;

  getSysInfo(): Promise<DeviceInfo>;
  getSysStatus(): Promise<DeviceStatus>;
  getFullKeymap(): Promise<KeymapConfig>;
  setFullKeymap(config: KeymapConfig): Promise<void>;
}

export interface HidDevicePlugin extends DeviceUiProvider {
  readonly id: string;
  readonly displayName: string;
  createAdapter(): HidAdapter;
}
