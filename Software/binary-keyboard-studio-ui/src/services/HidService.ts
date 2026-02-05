/**
 * MeowKeyboard HID 通讯服务
 * 封装与键盘的 WebHID 通讯
 */

import {
  Command,
  ResponseCode,
  VENDOR_ID,
  PRODUCT_ID,
  REPORT_ID_COMMAND,
  REPORT_ID_RESPONSE,
  FRAME_SIZE,
  MAX_KEYS,
  MAX_LAYERS,
  MAX_FN_KEYS,
  type DeviceInfo,
  type DeviceStatus,
  type KeymapConfig,
  type LayerConfig,
  type KeyAction,
  type RgbConfig,
  type FnKeyConfig,
  type FnKeyEntry,
  KeyboardType,
  ActionType,
  createEmptyKeymap,
  createEmptyLayer,
  createEmptyAction,
  createEmptyFnKeyConfig,
  createDefaultRgbConfig,
} from '@/types/protocol';

// ============================================================================
// HID 过滤器
// ============================================================================

// HID 过滤器 - 只选择 Vendor Defined Usage Page (0xFF00) 的 Collection
// HID 过滤器 - 只选择 Vendor Defined Usage Page (0xFF00) 的 Collection
export const KEYBOARD_FILTER: HIDDeviceFilter = {
  vendorId: VENDOR_ID,
  productId: PRODUCT_ID,
  usagePage: 0xFF00,  // Vendor Defined
  usage: 0x01,        // Vendor Usage 1
};

// 响应帧头大小 [CMD:1][SUB:1][LEN:1]
const RESP_HEADER_SIZE = 3;

// ============================================================================
// HID 服务类
// ============================================================================

export class HidService {
  private device: HIDDevice | null = null;
  private responsePromise: { resolve: (data: DataView) => void; reject: (err: Error) => void } | null = null;
  private responseTimeout: number | null = null;

  // ----------------------------------------
  // 设备管理
  // ----------------------------------------

  /** 请求并选择设备 */
  async requestDevice(): Promise<HIDDevice | null> {
    try {
      const devices = await navigator.hid.requestDevice({ filters: [KEYBOARD_FILTER] });
      if (devices.length === 0) return null;
      return devices[0];
    } catch (error) {
      console.error('请求 HID 设备失败:', error);
      return null;
    }
  }

  /** 获取已授权的设备 */
  async getAuthorizedDevice(): Promise<HIDDevice | null> {
    const devices = await navigator.hid.getDevices();
    return devices.find(d => d.vendorId === VENDOR_ID && d.productId === PRODUCT_ID) || null;
  }

  /** 连接设备 */
  async connect(device: HIDDevice): Promise<boolean> {
    try {
      if (!device.opened) {
        await device.open();
      }
      this.device = device;
      device.addEventListener('inputreport', this.handleInputReport.bind(this));
      return true;
    } catch (error) {
      console.error('打开 HID 设备失败:', error);
      return false;
    }
  }

  /** 断开设备 */
  async disconnect(): Promise<void> {
    if (this.device) {
      try {
        await this.device.close();
      } catch { /* ignore */ }
      this.device = null;
    }
  }

  /** 获取当前设备 */
  getDevice(): HIDDevice | null {
    return this.device;
  }

  /** 是否已连接 */
  isConnected(): boolean {
    return this.device !== null && this.device.opened;
  }

  // ----------------------------------------
  // 帧处理
  // ----------------------------------------

  /** 发送命令帧并等待响应 */
  private async sendCommand(cmd: Command, sub = 0, data: Uint8Array = new Uint8Array(0), timeout = 3000): Promise<DataView> {
    if (!this.device || !this.device.opened) {
      throw new Error('设备未连接');
    }

    // 构造帧
    const frame = new Uint8Array(FRAME_SIZE);
    frame[0] = cmd;
    frame[1] = sub;
    frame[2] = data.length;
    frame.set(data, 3);

    console.log('发送命令:', { cmd: '0x' + cmd.toString(16), sub, dataLen: data.length, reportId: REPORT_ID_COMMAND });

    // 发送
    try {
      await this.device.sendReport(REPORT_ID_COMMAND, frame);
    } catch (e) {
      console.error('sendReport 失败:', e);
      throw e;
    }

    // 等待响应
    return new Promise((resolve, reject) => {
      this.responsePromise = { resolve, reject };
      this.responseTimeout = window.setTimeout(() => {
        this.responsePromise = null;
        reject(new Error('命令响应超时'));
      }, timeout);
    });
  }

  /** 处理输入报告 */
  private handleInputReport(event: HIDInputReportEvent): void {
    // 无 Report ID 模式：reportId 为 0
    // 有 Report ID 模式：检查 reportId 是否匹配
    if (REPORT_ID_RESPONSE !== 0 && event.reportId !== REPORT_ID_RESPONSE) return;

    console.log('收到响应, reportId:', event.reportId, 'len:', event.data.byteLength);

    if (this.responsePromise) {
      if (this.responseTimeout) {
        clearTimeout(this.responseTimeout);
        this.responseTimeout = null;
      }
      this.responsePromise.resolve(new DataView(event.data.buffer));
      this.responsePromise = null;
    }
  }

  // ----------------------------------------
  // 命令实现
  // ----------------------------------------

  /** 获取设备信息 */
  async getSysInfo(): Promise<DeviceInfo> {
    const resp = await this.sendCommand(Command.SYS_INFO);

    // 响应帧格式: [CMD:1][SUB:1][LEN:1][DATA...]
    // DATA[0] = status, DATA[1..] = 实际数据
    const d = RESP_HEADER_SIZE;  // data offset
    const status = resp.getUint8(d + 0);
    if (status !== ResponseCode.OK) {
      throw new Error(`SYS_INFO 失败: 0x${status.toString(16)}`);
    }

    return {
      vendorId: (resp.getUint8(d + 1) << 8) | resp.getUint8(d + 2),
      productId: (resp.getUint8(d + 3) << 8) | resp.getUint8(d + 4),
      versionMajor: resp.getUint8(d + 5),
      versionMinor: resp.getUint8(d + 6),
      versionPatch: resp.getUint8(d + 7),
      maxLayers: resp.getUint8(d + 8),
      maxKeys: resp.getUint8(d + 9),
      macroSlots: resp.getUint8(d + 10),
      keyboardType: resp.getUint8(d + 11) as KeyboardType,
      actualKeyCount: resp.getUint8(d + 12),
      fnKeyCount: resp.getUint8(d + 13),
    };
  }

  /** 获取系统状态 */
  async getSysStatus(): Promise<DeviceStatus> {
    const resp = await this.sendCommand(Command.SYS_STATUS);

    const d = RESP_HEADER_SIZE;
    const status = resp.getUint8(d + 0);
    if (status !== ResponseCode.OK) {
      throw new Error(`SYS_STATUS 失败: 0x${status.toString(16)}`);
    }

    return {
      workMode: resp.getUint8(d + 1),
      connectionState: resp.getUint8(d + 2),
      currentLayer: resp.getUint8(d + 3),
      batteryLevel: resp.getUint8(d + 4),
      isCharging: resp.getUint8(d + 5) !== 0,
    };
  }

  /** 获取指定层的按键映射 */
  async getKeymap(layerIndex: number): Promise<{ numLayers: number; currentLayer: number; defaultLayer: number; layer: LayerConfig }> {
    const resp = await this.sendCommand(Command.KEYMAP_GET, layerIndex);

    const d = RESP_HEADER_SIZE;
    const status = resp.getUint8(d + 0);
    if (status !== ResponseCode.OK) {
      throw new Error(`KEYMAP_GET 失败: 0x${status.toString(16)}`);
    }

    const numLayers = resp.getUint8(d + 1);
    const currentLayer = resp.getUint8(d + 2);
    const defaultLayer = resp.getUint8(d + 3);

    // 解析层数据 (偏移 4, 32 字节 = 8 键 × 4 字节)
    const keys: KeyAction[] = [];
    for (let i = 0; i < MAX_KEYS; i++) {
      const offset = d + 4 + i * 4;
      keys.push({
        type: resp.getUint8(offset),
        modifier: resp.getUint8(offset + 1),
        param1: resp.getUint8(offset + 2),
        param2: resp.getUint8(offset + 3),
      });
    }

    return { numLayers, currentLayer, defaultLayer, layer: { keys } };
  }

  /** 获取完整按键映射 */
  async getFullKeymap(): Promise<KeymapConfig> {
    // 先获取第一层以获取层信息
    const first = await this.getKeymap(0);
    const config = createEmptyKeymap();
    config.numLayers = first.numLayers;
    config.currentLayer = first.currentLayer;
    config.defaultLayer = first.defaultLayer;
    config.layers[0] = first.layer;

    // 获取其余层
    for (let i = 1; i < first.numLayers; i++) {
      const result = await this.getKeymap(i);
      config.layers[i] = result.layer;
    }

    return config;
  }

  /** 设置指定层的按键映射 */
  async setKeymap(layerIndex: number, numLayers: number, defaultLayer: number, layer: LayerConfig): Promise<void> {
    const data = new Uint8Array(35); // 3 + 32
    data[0] = numLayers;
    data[1] = 0; // reserved
    data[2] = defaultLayer;

    // 编码层数据
    for (let i = 0; i < MAX_KEYS; i++) {
      const key = layer.keys[i] || createEmptyAction();
      const offset = 3 + i * 4;
      data[offset] = key.type;
      data[offset + 1] = key.modifier;
      data[offset + 2] = key.param1;
      data[offset + 3] = key.param2;
    }

    const resp = await this.sendCommand(Command.KEYMAP_SET, layerIndex, data);
    const status = resp.getUint8(RESP_HEADER_SIZE + 0);
    if (status !== ResponseCode.OK) {
      throw new Error(`KEYMAP_SET 失败: 0x${status.toString(16)}`);
    }
  }

  /** 设置完整按键映射 */
  async setFullKeymap(config: KeymapConfig): Promise<void> {
    for (let i = 0; i < config.numLayers; i++) {
      await this.setKeymap(i, config.numLayers, config.defaultLayer, config.layers[i]);
    }
  }

  /** 获取 RGB 配置 */
  async getRgbConfig(): Promise<RgbConfig> {
    const resp = await this.sendCommand(Command.RGB_GET);

    const d = RESP_HEADER_SIZE;
    const status = resp.getUint8(d + 0);
    if (status !== ResponseCode.OK) {
      throw new Error(`RGB_GET 失败: 0x${status.toString(16)}`);
    }

    return {
      enabled: resp.getUint8(d + 1) !== 0,
      mode: resp.getUint8(d + 2),
      brightness: resp.getUint8(d + 3),
      speed: resp.getUint8(d + 4),
      colorR: resp.getUint8(d + 5),
      colorG: resp.getUint8(d + 6),
      colorB: resp.getUint8(d + 7),
      indicatorEnabled: resp.getUint8(d + 8) !== 0,
    };
  }

  /** 设置 RGB 配置 */
  async setRgbConfig(config: RgbConfig): Promise<void> {
    const data = new Uint8Array(8);
    data[0] = config.enabled ? 1 : 0;
    data[1] = config.mode;
    data[2] = config.brightness;
    data[3] = config.speed;
    data[4] = config.colorR;
    data[5] = config.colorG;
    data[6] = config.colorB;
    data[7] = config.indicatorEnabled ? 1 : 0;

    const resp = await this.sendCommand(Command.RGB_SET, 0, data);
    const status = resp.getUint8(RESP_HEADER_SIZE + 0);
    if (status !== ResponseCode.OK) {
      throw new Error(`RGB_SET 失败: 0x${status.toString(16)}`);
    }
  }

  /** 获取 FN 键配置 */
  async getFnKeyConfig(): Promise<FnKeyConfig> {
    const resp = await this.sendCommand(Command.FNKEY_GET);

    const d = RESP_HEADER_SIZE;
    const status = resp.getUint8(d + 0);
    if (status !== ResponseCode.OK) {
      throw new Error(`FNKEY_GET 失败: 0x${status.toString(16)}`);
    }

    const fnKeys: FnKeyEntry[] = [];
    for (let i = 0; i < MAX_FN_KEYS; i++) {
      const offset = d + 1 + i * 8;
      fnKeys.push({
        clickAction: resp.getUint8(offset),
        clickParam: resp.getUint8(offset + 1),
        longAction: resp.getUint8(offset + 2),
        longParam: resp.getUint8(offset + 3),
        longPressMs: resp.getUint16(offset + 4, true), // little-endian
      });
    }

    return { fnKeys };
  }

  /** 设置 FN 键配置 */
  async setFnKeyConfig(config: FnKeyConfig): Promise<void> {
    const data = new Uint8Array(32);
    for (let i = 0; i < MAX_FN_KEYS; i++) {
      const fn = config.fnKeys[i];
      const offset = i * 8;
      data[offset] = fn.clickAction;
      data[offset + 1] = fn.clickParam;
      data[offset + 2] = fn.longAction;
      data[offset + 3] = fn.longParam;
      data[offset + 4] = fn.longPressMs & 0xff;
      data[offset + 5] = (fn.longPressMs >> 8) & 0xff;
      data[offset + 6] = 0; // reserved
      data[offset + 7] = 0; // reserved
    }

    const resp = await this.sendCommand(Command.FNKEY_SET, 0, data);
    const status = resp.getUint8(RESP_HEADER_SIZE + 0);
    if (status !== ResponseCode.OK) {
      throw new Error(`FNKEY_SET 失败: 0x${status.toString(16)}`);
    }
  }

  /** 保存配置到 Flash */
  async saveConfig(): Promise<void> {
    const resp = await this.sendCommand(Command.CFG_SAVE);
    const status = resp.getUint8(RESP_HEADER_SIZE + 0);
    if (status !== ResponseCode.OK) {
      throw new Error(`CFG_SAVE 失败: 0x${status.toString(16)}`);
    }
  }

  /** 从 Flash 加载配置 */
  async loadConfig(): Promise<void> {
    const resp = await this.sendCommand(Command.CFG_LOAD);
    const status = resp.getUint8(RESP_HEADER_SIZE + 0);
    if (status !== ResponseCode.OK) {
      throw new Error(`CFG_LOAD 失败: 0x${status.toString(16)}`);
    }
  }

  /** 恢复出厂设置 */
  async resetConfig(): Promise<void> {
    const resp = await this.sendCommand(Command.CFG_RESET);
    const status = resp.getUint8(RESP_HEADER_SIZE + 0);
    if (status !== ResponseCode.OK) {
      throw new Error(`CFG_RESET 失败: 0x${status.toString(16)}`);
    }
  }

  /** 获取电池信息 */
  async getBattery(): Promise<{ level: number; voltage: number; isCharging: boolean }> {
    const resp = await this.sendCommand(Command.BATTERY);

    const d = RESP_HEADER_SIZE;
    const status = resp.getUint8(d + 0);
    if (status !== ResponseCode.OK) {
      throw new Error(`BATTERY 失败: 0x${status.toString(16)}`);
    }

    return {
      level: resp.getUint8(d + 1),
      isCharging: resp.getUint8(d + 2) !== 0,
      voltage: resp.getUint8(d + 3) / 10, // 0.1V 单位
    };
  }
}

// 单例实例
export const hidService = new HidService();
