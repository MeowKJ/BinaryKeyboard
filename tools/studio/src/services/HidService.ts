/**
 * BinaryKeyboard HID 通讯服务
 * 统一管理 CH592F / CH552G 两套协议适配器
 */

import type { DeviceInfo, DeviceStatus, FnKeyConfig, KeymapConfig, LogConfig, RgbConfig } from '@/types/protocol';
import { showToast } from '@/services/toastService';
import { Ch592HidAdapter } from './hid/Ch592HidAdapter';
import { Ch552HidAdapter } from './hid/Ch552HidAdapter';
import type { BatteryInfo, HidAdapter } from './hid/types';

const ADAPTERS: HidAdapter[] = [
  new Ch592HidAdapter(),
  new Ch552HidAdapter(),
];

export const KEYBOARD_FILTERS: HIDDeviceFilter[] = ADAPTERS.flatMap((adapter) => adapter.filters);

export class HidService {
  static isSupported(): boolean {
    return typeof navigator !== 'undefined' && 'hid' in navigator;
  }

  private activeAdapter: HidAdapter | null = null;

  private resolveAdapter(device: HIDDevice): HidAdapter | null {
    return ADAPTERS.find((adapter) => adapter.matches(device)) ?? null;
  }

  private requireAdapter(): HidAdapter {
    const adapter = this.activeAdapter;
    if (!adapter || !adapter.isConnected()) {
      throw new Error('设备未连接');
    }
    return adapter;
  }

  async requestDevice(): Promise<HIDDevice | null> {
    if (!HidService.isSupported()) {
      showToast('error', '不支持 WebHID', '请使用 Chrome / Edge 等支持 WebHID 的浏览器');
      return null;
    }
    try {
      const devices = await navigator.hid.requestDevice({ filters: KEYBOARD_FILTERS });
      if (devices.length === 0) return null;
      return devices[0];
    } catch (error) {
      showToast('error', '连接失败', error instanceof Error ? error.message : '请求设备时发生未知错误');
      return null;
    }
  }

  async getAuthorizedDevice(): Promise<HIDDevice | null> {
    if (!HidService.isSupported()) return null;
    const devices = await navigator.hid.getDevices();
    return devices.find((device) => this.resolveAdapter(device) !== null) ?? null;
  }

  async connect(device: HIDDevice): Promise<boolean> {
    const adapter = this.resolveAdapter(device);
    if (!adapter) {
      showToast('error', '打开设备失败', '当前设备不是受支持的 BinaryKeyboard 固件');
      return false;
    }

    try {
      if (this.activeAdapter && this.activeAdapter !== adapter) {
        await this.activeAdapter.disconnect();
      }

      const success = await adapter.connect(device);
      this.activeAdapter = success ? adapter : null;
      return success;
    } catch (error) {
      showToast('error', '打开设备失败', error instanceof Error ? error.message : '无法打开 HID 设备');
      return false;
    }
  }

  async disconnect(): Promise<void> {
    if (this.activeAdapter) {
      await this.activeAdapter.disconnect();
      this.activeAdapter = null;
    }
  }

  getDevice(): HIDDevice | null {
    return this.activeAdapter?.getDevice() ?? null;
  }

  isConnected(): boolean {
    return this.activeAdapter?.isConnected() ?? false;
  }

  async getSysInfo(): Promise<DeviceInfo> {
    return this.requireAdapter().getSysInfo();
  }

  async getSysStatus(): Promise<DeviceStatus> {
    return this.requireAdapter().getSysStatus();
  }

  async getFullKeymap(): Promise<KeymapConfig> {
    return this.requireAdapter().getFullKeymap();
  }

  async setFullKeymap(config: KeymapConfig): Promise<void> {
    await this.requireAdapter().setFullKeymap(config);
  }

  async getRgbConfig(): Promise<RgbConfig> {
    return this.requireAdapter().getRgbConfig();
  }

  async setRgbConfig(config: RgbConfig): Promise<void> {
    await this.requireAdapter().setRgbConfig(config);
  }

  async getFnKeyConfig(): Promise<FnKeyConfig> {
    return this.requireAdapter().getFnKeyConfig();
  }

  async setFnKeyConfig(config: FnKeyConfig): Promise<void> {
    await this.requireAdapter().setFnKeyConfig(config);
  }

  async saveConfig(): Promise<void> {
    await this.requireAdapter().saveConfig();
  }

  async loadConfig(): Promise<void> {
    await this.requireAdapter().loadConfig();
  }

  async resetConfig(): Promise<void> {
    await this.requireAdapter().resetConfig();
  }

  async getBattery(): Promise<BatteryInfo> {
    return this.requireAdapter().getBattery();
  }

  async getLogConfig(): Promise<LogConfig> {
    return this.requireAdapter().getLogConfig();
  }

  async setLogConfig(config: LogConfig): Promise<void> {
    await this.requireAdapter().setLogConfig(config);
  }
}

export const hidService = new HidService();
