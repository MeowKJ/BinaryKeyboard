import {
  ActionType,
  CH552_CAPABILITIES,
  CH552_FRAME_SIZE,
  CH552_PRODUCT_ID,
  CH552_REPORT_ID_COMMAND,
  CH552_REPORT_ID_RESPONSE,
  CH552_VENDOR_ID,
  DeviceProtocol,
  KeyboardType,
  KeyboardTypeInfo,
  MouseButton,
  WheelDirection,
  createDefaultLogConfig,
  createEmptyAction,
  createEmptyKeymap,
  type DeviceInfo,
  type DeviceStatus,
  type FnKeyConfig,
  type KeyAction,
  type KeymapConfig,
  type LogConfig,
  type RgbConfig,
} from '@/types/protocol';
import { useTerminalStore } from '@/stores/terminalStore';
import { toHexDump } from '@/utils/protocolParser';
import type { BatteryInfo, HidAdapter } from './types';

const CH552_FILTER: HIDDeviceFilter = {
  vendorId: CH552_VENDOR_ID,
  productId: CH552_PRODUCT_ID,
  usagePage: 0xFF00,
  usage: 0x01,
};

const CH552_SLOT_COUNT = 8;
const CH552_SLOT_SIZE = 3;
const CH552_PAYLOAD_START = 3;

enum LegacyCh552Command {
  READ_CONFIG = 0x01,
  WRITE_CONFIG = 0x02,
}

enum LegacyCh552KeyType {
  INVALID = 0x00,
  KEYBOARD = 0x01,
  MEDIA = 0x02,
  MOUSE = 0x03,
}

interface LegacyConfigBlob {
  version: number;
  deviceType: number;
  payload: Uint8Array;
}

function ch552VersionToSemver(versionByte: number): { major: number; minor: number; patch: number } {
  return {
    major: 2,
    minor: 0,
    patch: versionByte & 0xff,
  };
}

function unsupported(feature: string): Error {
  return new Error(`当前 CH552G 固件不支持${feature}`);
}

function hexByte(value: number): string {
  return `0x${value.toString(16).padStart(2, '0').toUpperCase()}`;
}

function mapDeviceType(deviceType: number): KeyboardType {
  switch (deviceType) {
    case 0x01:
      return KeyboardType.BASIC;
    case 0x02:
      return KeyboardType.KNOB;
    case 0x03:
      return KeyboardType.FIVE_KEYS;
    default:
      return KeyboardType.BASIC;
  }
}

function wheelDirectionToScroll(direction: number): number {
  switch (direction) {
    case WheelDirection.UP:
      return 1;
    case WheelDirection.DOWN:
      return 0xff;
    default:
      return 0;
  }
}

function decodeLegacyAction(type: number, value: number): KeyAction {
  switch (type) {
    case LegacyCh552KeyType.KEYBOARD:
      return {
        type: ActionType.KEYBOARD,
        modifier: (value >> 8) & 0xff,
        param1: value & 0xff,
        param2: 0,
      };
    case LegacyCh552KeyType.MEDIA:
      return {
        type: ActionType.CONSUMER,
        modifier: 0,
        param1: value & 0xff,
        param2: (value >> 8) & 0xff,
      };
    case LegacyCh552KeyType.MOUSE: {
      const button = value & 0xff;
      const scroll = (value >> 8) & 0xff;
      if (button !== 0) {
        return {
          type: ActionType.MOUSE_BTN,
          modifier: 0,
          param1: button,
          param2: 0,
        };
      }
      if (scroll !== 0) {
        return {
          type: ActionType.MOUSE_WHEEL,
          modifier: 0,
          param1: scroll < 0x80 ? WheelDirection.UP : WheelDirection.DOWN,
          param2: 0,
        };
      }
      return createEmptyAction();
    }
    default:
      return createEmptyAction();
  }
}

function encodeLegacyAction(action: KeyAction): { type: number; value: number } {
  switch (action.type) {
    case ActionType.NONE:
      return { type: LegacyCh552KeyType.INVALID, value: 0 };
    case ActionType.KEYBOARD:
      return {
        type: LegacyCh552KeyType.KEYBOARD,
        value: ((action.modifier & 0xff) << 8) | (action.param1 & 0xff),
      };
    case ActionType.CONSUMER:
      return {
        type: LegacyCh552KeyType.MEDIA,
        value: ((action.param2 & 0xff) << 8) | (action.param1 & 0xff),
      };
    case ActionType.MOUSE_BTN:
      return {
        type: LegacyCh552KeyType.MOUSE,
        value: action.param1 & 0xff,
      };
    case ActionType.MOUSE_WHEEL: {
      if (action.param1 === WheelDirection.CLICK) {
        return {
          type: LegacyCh552KeyType.MOUSE,
          value: MouseButton.MIDDLE,
        };
      }
      return {
        type: LegacyCh552KeyType.MOUSE,
        value: (wheelDirectionToScroll(action.param1) & 0xff) << 8,
      };
    }
    default:
      throw unsupported('该动作类型');
  }
}

export class Ch552HidAdapter implements HidAdapter {
  readonly protocol = DeviceProtocol.CH552;
  readonly filters = [CH552_FILTER];

  private device: HIDDevice | null = null;
  private responsePromise: { resolve: (data: Uint8Array) => void; reject: (err: Error) => void } | null = null;
  private responseTimeout: number | null = null;
  private readonly inputReportHandler = this.handleInputReport.bind(this);

  matches(device: HIDDevice): boolean {
    return device.vendorId === CH552_VENDOR_ID && device.productId === CH552_PRODUCT_ID;
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

  private addTerminalEntry(
    direction: 'send' | 'receive',
    cmd: number,
    data: Uint8Array,
    parsed: string,
  ): void {
    try {
      const terminalStore = useTerminalStore();
      terminalStore.addEntry({
        direction,
        level: 'info',
        command: cmd === LegacyCh552Command.READ_CONFIG ? 'CH552_READ_CFG' : 'CH552_WRITE_CFG',
        cmdHex: cmd.toString(16).padStart(2, '0'),
        sub: 0,
        dataLen: data.length,
        rawHex: toHexDump(data, CH552_FRAME_SIZE),
        parsed,
        glowColor: cmd === LegacyCh552Command.READ_CONFIG ? '#60a5fa' : '#4ade80',
      });
    } catch {
      // terminal may not be ready
    }
  }

  private async sendReadCommand(timeout = 1500): Promise<Uint8Array> {
    if (!this.device || !this.device.opened) {
      throw new Error('设备未连接');
    }

    const frame = new Uint8Array(CH552_FRAME_SIZE);
    frame[0] = LegacyCh552Command.READ_CONFIG;
    this.addTerminalEntry('send', LegacyCh552Command.READ_CONFIG, frame, '读取 EEPROM 键位配置');

    await this.device.sendReport(CH552_REPORT_ID_COMMAND, frame);

    return new Promise((resolve, reject) => {
      this.responsePromise = { resolve, reject };
      this.responseTimeout = window.setTimeout(() => {
        this.responsePromise = null;
        reject(new Error('CH552G 配置读取超时'));
      }, timeout);
    });
  }

  private async readConfigBlob(): Promise<LegacyConfigBlob> {
    const data = await this.sendReadCommand();
    if (data[0] !== LegacyCh552Command.READ_CONFIG) {
      throw new Error(`CH552G 返回了未知响应 ${hexByte(data[0])}`);
    }
    return {
      version: data[1],
      deviceType: data[2],
      payload: data,
    };
  }

  private handleInputReport(event: HIDInputReportEvent): void {
    if (event.reportId !== CH552_REPORT_ID_RESPONSE) return;

    const data = new Uint8Array(event.data.buffer, event.data.byteOffset, event.data.byteLength);
    const keyboardType = mapDeviceType(data[2] ?? 0);
    this.addTerminalEntry(
      'receive',
      data[0] ?? 0,
      data,
      `版本=${hexByte(data[1] ?? 0)} 设备=${KeyboardTypeInfo[keyboardType].name}`,
    );

    if (this.responsePromise) {
      const promise = this.responsePromise;
      this.clearPendingResponse();
      promise.resolve(data);
    }
  }

  async getSysInfo(): Promise<DeviceInfo> {
    const blob = await this.readConfigBlob();
    const keyboardType = mapDeviceType(blob.deviceType);
    const semver = ch552VersionToSemver(blob.version);

    return {
      vendorId: CH552_VENDOR_ID,
      productId: CH552_PRODUCT_ID,
      versionMajor: semver.major,
      versionMinor: semver.minor,
      versionPatch: semver.patch,
      maxLayers: 1,
      maxKeys: CH552_SLOT_COUNT,
      macroSlots: 0,
      keyboardType,
      actualKeyCount: KeyboardTypeInfo[keyboardType].keys,
      fnKeyCount: 0,
      protocol: DeviceProtocol.CH552,
      capabilities: CH552_CAPABILITIES,
    };
  }

  async getSysStatus(): Promise<DeviceStatus> {
    return {
      workMode: 0,
      connectionState: 2,
      currentLayer: 0,
      batteryLevel: 100,
      isCharging: false,
    };
  }

  async getFullKeymap(): Promise<KeymapConfig> {
    const blob = await this.readConfigBlob();
    const config = createEmptyKeymap();
    config.numLayers = 1;
    config.currentLayer = 0;
    config.defaultLayer = 0;

    for (let i = 0; i < CH552_SLOT_COUNT; i++) {
      const offset = CH552_PAYLOAD_START + i * CH552_SLOT_SIZE;
      const type = blob.payload[offset];
      const value = ((blob.payload[offset + 1] ?? 0) << 8) | (blob.payload[offset + 2] ?? 0);
      config.layers[0].keys[i] = decodeLegacyAction(type, value);
    }

    return config;
  }

  async setFullKeymap(config: KeymapConfig): Promise<void> {
    if (!this.device || !this.device.opened) {
      throw new Error('设备未连接');
    }

    const blob = await this.readConfigBlob();
    const layer = config.layers[0] ?? createEmptyKeymap().layers[0];
    const frame = new Uint8Array(CH552_FRAME_SIZE);
    frame[0] = LegacyCh552Command.WRITE_CONFIG;
    frame[1] = blob.version;
    frame[2] = blob.deviceType;

    for (let i = 0; i < CH552_SLOT_COUNT; i++) {
      const action = layer.keys[i] ?? createEmptyAction();
      const encoded = encodeLegacyAction(action);
      const offset = CH552_PAYLOAD_START + i * CH552_SLOT_SIZE;
      frame[offset] = encoded.type;
      frame[offset + 1] = (encoded.value >> 8) & 0xff;
      frame[offset + 2] = encoded.value & 0xff;
    }

    this.addTerminalEntry('send', LegacyCh552Command.WRITE_CONFIG, frame, '写入 EEPROM 键位配置');
    await this.device.sendReport(CH552_REPORT_ID_COMMAND, frame);
  }

  async getRgbConfig(): Promise<RgbConfig> {
    throw unsupported(' RGB 配置');
  }

  async setRgbConfig(_config: RgbConfig): Promise<void> {
    throw unsupported(' RGB 配置');
  }

  async getFnKeyConfig(): Promise<FnKeyConfig> {
    throw unsupported(' FN 键配置');
  }

  async setFnKeyConfig(_config: FnKeyConfig): Promise<void> {
    throw unsupported(' FN 键配置');
  }

  async saveConfig(): Promise<void> {
    // CH552G 的写入命令会立即落到 EEPROM，这里保持兼容为 no-op。
  }

  async loadConfig(): Promise<void> {
    // CH552G 无显式 load 命令，调用方重新读取即可。
  }

  async resetConfig(): Promise<void> {
    throw unsupported('恢复出厂');
  }

  async getBattery(): Promise<BatteryInfo> {
    throw unsupported('电池状态');
  }

  async getLogConfig(): Promise<LogConfig> {
    return { ...createDefaultLogConfig(), enabled: false };
  }

  async setLogConfig(_config: LogConfig): Promise<void> {
    throw unsupported(' HID 日志配置');
  }
}
