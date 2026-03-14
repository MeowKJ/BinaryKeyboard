/**
 * BinaryKeyboard HID 通讯服务
 * 统一管理已注册的设备适配器插件
 */

import type { DeviceInfo, DeviceStatus, FnKeyConfig, KeymapConfig, LogConfig, RgbConfig } from '@/types/protocol';
import { showToast } from '@/services/toastService';
import { createHidAdapters } from './hid/registry';
import type { BatteryInfo, HidAdapter, HidOptionalOperations } from './hid/common/types';
import { OPTIONAL_OPERATION_LABELS } from './hid/common/types';

const ADAPTERS: HidAdapter[] = createHidAdapters();

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

  private requireOptionalOperation<K extends keyof HidOptionalOperations>(name: K): NonNullable<HidOptionalOperations[K]> {
    const adapter = this.requireAdapter();
    const operation = adapter.optional[name];
    if (!operation) {
      throw new Error(`当前设备不支持${OPTIONAL_OPERATION_LABELS[name]}`);
    }
    return operation as NonNullable<HidOptionalOperations[K]>;
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
    return this.requireOptionalOperation('getRgbConfig')();
  }

  async setRgbConfig(config: RgbConfig): Promise<void> {
    await this.requireOptionalOperation('setRgbConfig')(config);
  }

  async getFnKeyConfig(): Promise<FnKeyConfig> {
    return this.requireOptionalOperation('getFnKeyConfig')();
  }

  async setFnKeyConfig(config: FnKeyConfig): Promise<void> {
    await this.requireOptionalOperation('setFnKeyConfig')(config);
  }

  async saveConfig(): Promise<void> {
    await this.requireOptionalOperation('saveConfig')();
  }

  async loadConfig(): Promise<void> {
    await this.requireOptionalOperation('loadConfig')();
  }

  async resetConfig(): Promise<void> {
    await this.requireOptionalOperation('resetConfig')();
  }

  async getBattery(): Promise<BatteryInfo> {
    return this.requireOptionalOperation('getBattery')();
  }

  async getLogConfig(): Promise<LogConfig> {
    return this.requireOptionalOperation('getLogConfig')();
  }

  async setLogConfig(config: LogConfig): Promise<void> {
    await this.requireOptionalOperation('setLogConfig')(config);
  }
}

export const hidService = new HidService();
