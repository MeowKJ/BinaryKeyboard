import type {
  DeviceInfo,
  DeviceStatus,
  KeymapConfig,
  RgbConfig,
  FnKeyConfig,
  LogConfig,
  DeviceProtocol,
} from '@/types/protocol';

export interface BatteryInfo {
  level: number;
  voltage: number;
  isCharging: boolean;
}

export interface HidAdapter {
  readonly protocol: DeviceProtocol;
  readonly filters: HIDDeviceFilter[];

  matches(device: HIDDevice): boolean;
  connect(device: HIDDevice): Promise<boolean>;
  disconnect(): Promise<void>;
  getDevice(): HIDDevice | null;
  isConnected(): boolean;

  getSysInfo(): Promise<DeviceInfo>;
  getSysStatus(): Promise<DeviceStatus>;
  getFullKeymap(): Promise<KeymapConfig>;
  setFullKeymap(config: KeymapConfig): Promise<void>;

  getRgbConfig(): Promise<RgbConfig>;
  setRgbConfig(config: RgbConfig): Promise<void>;
  getFnKeyConfig(): Promise<FnKeyConfig>;
  setFnKeyConfig(config: FnKeyConfig): Promise<void>;

  saveConfig(): Promise<void>;
  loadConfig(): Promise<void>;
  resetConfig(): Promise<void>;

  getBattery(): Promise<BatteryInfo>;
  getLogConfig(): Promise<LogConfig>;
  setLogConfig(config: LogConfig): Promise<void>;
}
