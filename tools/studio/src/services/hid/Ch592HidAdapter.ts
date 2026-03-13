import {
  Command,
  ResponseCode,
  REPORT_ID_COMMAND,
  REPORT_ID_RESPONSE,
  FRAME_SIZE,
  MAX_KEYS,
  MAX_FN_KEYS,
  type DeviceInfo,
  type DeviceStatus,
  type KeymapConfig,
  type LayerConfig,
  type KeyAction,
  type RgbConfig,
  type FnKeyConfig,
  type FnKeyEntry,
  type LogConfig,
  DeviceProtocol,
  CH592_CAPABILITIES,
  CH592_VENDOR_ID,
  CH592_PRODUCT_ID,
  createEmptyKeymap,
  createEmptyAction,
} from '@/types/protocol';
import { useTerminalStore } from '@/stores/terminalStore';
import { parseSendFrame, parseReceiveFrame, parseLogFrame } from '@/utils/protocolParser';
import type { BatteryInfo, HidAdapter } from './types';

const RESP_HEADER_SIZE = 3;

export const CH592_FILTER: HIDDeviceFilter = {
  vendorId: CH592_VENDOR_ID,
  productId: CH592_PRODUCT_ID,
  usagePage: 0xFF00,
  usage: 0x01,
};

export class Ch592HidAdapter implements HidAdapter {
  readonly protocol = DeviceProtocol.CH592;
  readonly filters = [CH592_FILTER];

  private device: HIDDevice | null = null;
  private responsePromise: { resolve: (data: DataView) => void; reject: (err: Error) => void } | null = null;
  private responseTimeout: number | null = null;
  private readonly inputReportHandler = this.handleInputReport.bind(this);

  matches(device: HIDDevice): boolean {
    return device.vendorId === CH592_VENDOR_ID && device.productId === CH592_PRODUCT_ID;
  }

  async connect(device: HIDDevice): Promise<boolean> {
    try {
      if (this.device && this.device !== device) {
        this.device.removeEventListener('inputreport', this.inputReportHandler);
      }
      if (!device.opened) {
        await device.open();
      }
      this.device = device;
      device.removeEventListener('inputreport', this.inputReportHandler);
      device.addEventListener('inputreport', this.inputReportHandler);
      return true;
    } catch {
      return false;
    }
  }

  async disconnect(): Promise<void> {
    if (this.device) {
      try {
        this.device.removeEventListener('inputreport', this.inputReportHandler);
        await this.device.close();
      } catch {
        // ignore
      }
      this.device = null;
    }
    this.clearPendingResponse();
  }

  getDevice(): HIDDevice | null {
    return this.device;
  }

  isConnected(): boolean {
    return this.device !== null && this.device.opened;
  }

  private clearPendingResponse(error?: Error): void {
    if (this.responseTimeout) {
      clearTimeout(this.responseTimeout);
      this.responseTimeout = null;
    }
    if (error && this.responsePromise) {
      this.responsePromise.reject(error);
    }
    this.responsePromise = null;
  }

  private async sendCommand(
    cmd: Command,
    sub = 0,
    data: Uint8Array = new Uint8Array(0),
    timeout = 3000,
  ): Promise<DataView> {
    if (!this.device || !this.device.opened) {
      throw new Error('设备未连接');
    }
    if (data.length > FRAME_SIZE - 3) {
      throw new Error(`命令数据过长: ${data.length} > ${FRAME_SIZE - 3}`);
    }

    const frame = new Uint8Array(FRAME_SIZE);
    frame[0] = cmd;
    frame[1] = sub;
    frame[2] = data.length;
    frame.set(data, 3);

    try {
      const terminalStore = useTerminalStore();
      const parsed = parseSendFrame(frame);
      terminalStore.addEntry({
        direction: 'send',
        level: 'info',
        command: parsed.command,
        cmdHex: parsed.cmdHex,
        sub: parsed.sub,
        dataLen: parsed.dataLen,
        rawHex: parsed.rawHex,
        parsed: parsed.parsed,
      });
    } catch {
      // terminal may not be initialized
    }

    await this.device.sendReport(REPORT_ID_COMMAND, frame);

    return new Promise((resolve, reject) => {
      this.responsePromise = { resolve, reject };
      this.responseTimeout = window.setTimeout(() => {
        this.responsePromise = null;
        reject(new Error('命令响应超时'));
      }, timeout);
    });
  }

  private handleInputReport(event: HIDInputReportEvent): void {
    if (REPORT_ID_RESPONSE !== 0 && event.reportId !== REPORT_ID_RESPONSE) return;

    const frameBytes = new Uint8Array(event.data.buffer, event.data.byteOffset, event.data.byteLength);
    const cmd = frameBytes[0];

    if (cmd === Command.LOG) {
      try {
        const terminalStore = useTerminalStore();
        const parsed = parseLogFrame(frameBytes);
        terminalStore.addEntry({
          direction: 'device',
          level: 'info',
          command: parsed.command,
          cmdHex: parsed.cmdHex,
          sub: parsed.sub,
          dataLen: parsed.dataLen,
          rawHex: parsed.rawHex,
          parsed: parsed.parsed,
          category: parsed.category,
        });
      } catch {
        // ignore
      }
      return;
    }

    try {
      const terminalStore = useTerminalStore();
      const parsed = parseReceiveFrame(frameBytes);
      terminalStore.addEntry({
        direction: 'receive',
        level: parsed.isError ? 'error' : 'success',
        command: parsed.command,
        cmdHex: parsed.cmdHex,
        sub: parsed.sub,
        dataLen: parsed.dataLen,
        rawHex: parsed.rawHex,
        parsed: parsed.parsed,
        statusCode: parsed.statusCode,
      });
    } catch {
      // ignore
    }

    if (this.responsePromise) {
      const promise = this.responsePromise;
      this.clearPendingResponse();
      promise.resolve(new DataView(event.data.buffer, event.data.byteOffset, event.data.byteLength));
    }
  }

  async getSysInfo(): Promise<DeviceInfo> {
    const resp = await this.sendCommand(Command.SYS_INFO);
    const d = RESP_HEADER_SIZE;
    const status = resp.getUint8(d);
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
      keyboardType: resp.getUint8(d + 11),
      actualKeyCount: resp.getUint8(d + 12),
      fnKeyCount: resp.getUint8(d + 13),
      protocol: DeviceProtocol.CH592,
      capabilities: CH592_CAPABILITIES,
    };
  }

  async getSysStatus(): Promise<DeviceStatus> {
    const resp = await this.sendCommand(Command.SYS_STATUS);
    const d = RESP_HEADER_SIZE;
    const status = resp.getUint8(d);
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

  async getKeymap(layerIndex: number): Promise<{ numLayers: number; currentLayer: number; defaultLayer: number; layer: LayerConfig }> {
    const resp = await this.sendCommand(Command.KEYMAP_GET, layerIndex);
    const d = RESP_HEADER_SIZE;
    const status = resp.getUint8(d);
    if (status !== ResponseCode.OK) {
      throw new Error(`KEYMAP_GET 失败: 0x${status.toString(16)}`);
    }

    const numLayers = resp.getUint8(d + 1);
    const currentLayer = resp.getUint8(d + 2);
    const defaultLayer = resp.getUint8(d + 3);
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

  async getFullKeymap(): Promise<KeymapConfig> {
    const first = await this.getKeymap(0);
    const config = createEmptyKeymap();
    config.numLayers = first.numLayers;
    config.currentLayer = first.currentLayer;
    config.defaultLayer = first.defaultLayer;
    config.layers[0] = first.layer;

    for (let i = 1; i < first.numLayers; i++) {
      const result = await this.getKeymap(i);
      config.layers[i] = result.layer;
    }

    return config;
  }

  async setKeymap(layerIndex: number, numLayers: number, defaultLayer: number, layer: LayerConfig): Promise<void> {
    const data = new Uint8Array(35);
    data[0] = numLayers;
    data[1] = 0;
    data[2] = defaultLayer;

    for (let i = 0; i < MAX_KEYS; i++) {
      const key = layer.keys[i] || createEmptyAction();
      const offset = 3 + i * 4;
      data[offset] = key.type;
      data[offset + 1] = key.modifier;
      data[offset + 2] = key.param1;
      data[offset + 3] = key.param2;
    }

    const resp = await this.sendCommand(Command.KEYMAP_SET, layerIndex, data);
    const status = resp.getUint8(RESP_HEADER_SIZE);
    if (status !== ResponseCode.OK) {
      throw new Error(`KEYMAP_SET 失败: 0x${status.toString(16)}`);
    }
  }

  async setFullKeymap(config: KeymapConfig): Promise<void> {
    for (let i = 0; i < config.numLayers; i++) {
      await this.setKeymap(i, config.numLayers, config.defaultLayer, config.layers[i]);
    }
  }

  async getRgbConfig(): Promise<RgbConfig> {
    const resp = await this.sendCommand(Command.RGB_GET);
    const d = RESP_HEADER_SIZE;
    const status = resp.getUint8(d);
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
      indicatorBrightness: resp.byteLength >= d + 10 ? resp.getUint8(d + 9) : resp.getUint8(d + 3),
    };
  }

  async setRgbConfig(config: RgbConfig): Promise<void> {
    const data = new Uint8Array(9);
    data[0] = config.enabled ? 1 : 0;
    data[1] = config.mode;
    data[2] = config.brightness;
    data[3] = config.speed;
    data[4] = config.colorR;
    data[5] = config.colorG;
    data[6] = config.colorB;
    data[7] = config.indicatorEnabled ? 1 : 0;
    data[8] = config.indicatorBrightness ?? config.brightness;

    const resp = await this.sendCommand(Command.RGB_SET, 0, data);
    const status = resp.getUint8(RESP_HEADER_SIZE);
    if (status !== ResponseCode.OK) {
      throw new Error(`RGB_SET 失败: 0x${status.toString(16)}`);
    }
  }

  async getFnKeyConfig(): Promise<FnKeyConfig> {
    const resp = await this.sendCommand(Command.FNKEY_GET);
    const d = RESP_HEADER_SIZE;
    const status = resp.getUint8(d);
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
        longPressMs: resp.getUint16(offset + 4, true),
      });
    }

    return { fnKeys };
  }

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
    }

    const resp = await this.sendCommand(Command.FNKEY_SET, 0, data);
    const status = resp.getUint8(RESP_HEADER_SIZE);
    if (status !== ResponseCode.OK) {
      throw new Error(`FNKEY_SET 失败: 0x${status.toString(16)}`);
    }
  }

  async saveConfig(): Promise<void> {
    const resp = await this.sendCommand(Command.CFG_SAVE);
    const status = resp.getUint8(RESP_HEADER_SIZE);
    if (status !== ResponseCode.OK) {
      throw new Error(`CFG_SAVE 失败: 0x${status.toString(16)}`);
    }
  }

  async loadConfig(): Promise<void> {
    const resp = await this.sendCommand(Command.CFG_LOAD);
    const status = resp.getUint8(RESP_HEADER_SIZE);
    if (status !== ResponseCode.OK) {
      throw new Error(`CFG_LOAD 失败: 0x${status.toString(16)}`);
    }
  }

  async resetConfig(): Promise<void> {
    const resp = await this.sendCommand(Command.CFG_RESET);
    const status = resp.getUint8(RESP_HEADER_SIZE);
    if (status !== ResponseCode.OK) {
      throw new Error(`CFG_RESET 失败: 0x${status.toString(16)}`);
    }
  }

  async getBattery(): Promise<BatteryInfo> {
    const resp = await this.sendCommand(Command.BATTERY);
    const d = RESP_HEADER_SIZE;
    const status = resp.getUint8(d);
    if (status !== ResponseCode.OK) {
      throw new Error(`BATTERY 失败: 0x${status.toString(16)}`);
    }

    return {
      level: resp.getUint8(d + 1),
      isCharging: resp.getUint8(d + 2) !== 0,
      voltage: resp.getUint16(d + 3, true) / 1000,
    };
  }

  async getLogConfig(): Promise<LogConfig> {
    const resp = await this.sendCommand(Command.LOG_GET);
    const d = RESP_HEADER_SIZE;
    const status = resp.getUint8(d);
    if (status !== ResponseCode.OK) {
      throw new Error(`LOG_GET 失败: 0x${status.toString(16)}`);
    }

    return {
      enabled: resp.getUint8(d + 1) !== 0,
    };
  }

  async setLogConfig(config: LogConfig): Promise<void> {
    const data = new Uint8Array([config.enabled ? 1 : 0]);
    const resp = await this.sendCommand(Command.LOG_SET, 0, data);
    const d = RESP_HEADER_SIZE;
    const status = resp.getUint8(d);
    if (status !== ResponseCode.OK) {
      throw new Error(`LOG_SET 失败: 0x${status.toString(16)}`);
    }
  }
}
