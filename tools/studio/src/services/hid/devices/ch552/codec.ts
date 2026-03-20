import {
  ActionType,
  CH552_CAPABILITIES,
  CH552_FRAME_SIZE,
  CH552_MEOWFS_APPEND_SLOTS,
  CH552_MEOWFS_MAX_ACTIONS,
  CH552_MEOWFS_HEADER_SIZE,
  CH552_MEOWFS_ACTION_SIZE,
  DeviceProtocol,
  KeyboardType,
  KeyboardTypeInfo,
  MacroActionType,
  MouseButton,
  PressEffect,
  type MacroAction,
  type MacroData,
  type MacroHeader,
  type MacroOverview,
  type RgbConfig,
  WheelDirection,
  createDeviceCapabilities,
  createEmptyAction,
  createEmptyKeymap,
  type DeviceCapabilities,
  type DeviceInfo,
  type DeviceStatus,
  type KeyAction,
  type KeymapConfig,
  type LayerConfig,
} from '@/types/protocol';
import { FIRMWARE_VERSION_META } from '@/generated/versionConfig';
import { toHexDump } from '@/utils/protocolParser';
import type {
  CodecInboundPacket,
  CodecTransport,
  DeviceCodec,
  TerminalEntryDraft,
} from '../../common/codecTypes';
import type { HidOptionalOperations } from '../../common/types';

const CH552_SLOT_COUNT = 8;
const CH552_SLOT_SIZE = 3;
const FW_VERSION_MULTILAYER = 0x0C;
const FW_VERSION_RGB = 0x0D;
const CH552_READ_RETRY_COUNT = 3;

enum Ch552Command {
  READ_CONFIG = 0x01,
  WRITE_LAYER = 0x02,
  READ_LAYER = 0x03,
  SET_LAYER = 0x04,
  READ_META = 0x05,
  READ_RGB = 0x06,
  WRITE_RGB = 0x07,
  FACTORY_RESET = 0x08,
  MACRO = 0x40,
}

/** MeowFS HID 子命令 */
enum Ch552MacroSub {
  INFO = 0,
  READ = 1,
  ERASE = 2,
  WRITE = 3,
  DELETE = 4,
}

const MEOWFS_VALID_MARKER = 0xAA;
/** 单次 READ 最大字节数 (CustomBuf[4..30] = 26 bytes) */
const MEOWFS_READ_CHUNK = 26;
/** 单次 WRITE 最大字节数 (frame[5..30] = 22 bytes) */
const MEOWFS_WRITE_CHUNK = 22;

/** 内部解析后的宏条目 */
interface MeowFsMacroEntry {
  actionCount: number;
  actions: MacroAction[];
}

/** 缓存的 MeowFS 状态 */
interface MeowFsCache {
  fsTotal: number;
  fsFree: number;
  macros: MeowFsMacroEntry[];
}

enum LegacyCh552KeyType {
  INVALID = 0x00,
  KEYBOARD = 0x01,
  MEDIA = 0x02,
  MOUSE = 0x03,
  MACRO = 0x05, // valueHi=trigger, valueLo=slot
}

export interface Ch552ConfigBlob {
  version: number;
  deviceType: number;
  currentLayer: number;
  maxLayers: number;
  payload: Uint8Array;
}

export interface Ch552MetaInfo {
  firmwareMajor: number;
  firmwareMinor: number;
  firmwarePatch: number;
}

export class Ch552Codec implements DeviceCodec<Uint8Array> {
  readonly protocol = DeviceProtocol.CH552;
  readonly protocolLabel = 'CH552G USB';
  readonly chipFamily = FIRMWARE_VERSION_META.CH552G.chipFamily;

  private version = 0;
  private deviceType = 0;
  private maxLayers = 1;
  private currentLayer = 0;
  private configBlobCache: Ch552ConfigBlob | null = null;
  private configBlobCachedAt = 0;
  private meta: Ch552MetaInfo | null = null;
  private metaReadSupported: boolean | null = null;
  private meowfsCache: MeowFsCache | null = null;

  resetState(): void {
    this.version = 0;
    this.deviceType = 0;
    this.maxLayers = 1;
    this.currentLayer = 0;
    this.configBlobCache = null;
    this.configBlobCachedAt = 0;
    this.meta = null;
    this.metaReadSupported = null;
    this.meowfsCache = null;
  }

  get cachedVersion(): number {
    return this.version;
  }

  get cachedMaxLayers(): number {
    return this.maxLayers;
  }

  get isMultiLayer(): boolean {
    return this.version >= FW_VERSION_MULTILAYER;
  }

  get shouldTryReadMeta(): boolean {
    return this.metaReadSupported !== false;
  }

  async warmupConnection(transport: CodecTransport<Uint8Array>): Promise<void> {
    const frame = this.buildReadConfigFrame();

    for (let attempt = 0; attempt < 3; attempt++) {
      try {
        const data = await transport.sendAndWait(frame, {
          timeout: 220,
          timeoutLabel: 'CH552G 初始握手超时',
        });
        this.parseConfigBlob(data);
        return;
      } catch {
        if (attempt + 1 >= 3) {
          return;
        }
        await new Promise((resolve) => setTimeout(resolve, attempt === 0 ? 120 : 180));
      }
    }
  }

  getOptionalOperations(transport: CodecTransport<Uint8Array>): HidOptionalOperations {
    return {
      getRgbConfig: () => this.getRgbConfig(transport),
      setRgbConfig: (config) => this.setRgbConfig(transport, config),
      resetConfig: () => this.resetConfig(transport),
      getMacroOverview: () => this.getMacroOverview(transport),
      getMacroInfo: (slot) => this.getMacroInfo(transport, slot),
      getMacroData: (slot) => this.getMacroData(transport, slot),
      setMacroData: (slot, macro) => this.setMacroData(transport, slot, macro),
      deleteMacro: (slot) => this.deleteMacro(transport, slot),
    };
  }

  buildReadConfigFrame(): Uint8Array {
    const frame = new Uint8Array(CH552_FRAME_SIZE);
    frame[0] = Ch552Command.READ_CONFIG;
    return frame;
  }

  buildReadLayerFrame(layerIndex: number): Uint8Array {
    const frame = new Uint8Array(CH552_FRAME_SIZE);
    frame[0] = Ch552Command.READ_LAYER;
    frame[1] = layerIndex;
    return frame;
  }

  buildReadMetaFrame(): Uint8Array {
    const frame = new Uint8Array(CH552_FRAME_SIZE);
    frame[0] = Ch552Command.READ_META;
    return frame;
  }

  buildReadRgbFrame(): Uint8Array {
    const frame = new Uint8Array(CH552_FRAME_SIZE);
    frame[0] = Ch552Command.READ_RGB;
    return frame;
  }

  buildSetLayerFrame(layerIndex: number): Uint8Array {
    const frame = new Uint8Array(CH552_FRAME_SIZE);
    frame[0] = Ch552Command.SET_LAYER;
    frame[1] = layerIndex;
    return frame;
  }

  buildWriteRgbFrame(config: RgbConfig): Uint8Array {
    const frame = new Uint8Array(CH552_FRAME_SIZE);
    frame[0] = Ch552Command.WRITE_RGB;
    frame[1] = config.enabled ? 1 : 0;
    frame[2] = config.mode;
    frame[3] = config.brightness;
    frame[4] = config.speed;
    frame[5] = config.colorR;
    frame[6] = config.colorG;
    frame[7] = config.colorB;
    frame[8] = config.indicatorEnabled ? 1 : 0;
    frame[9] = config.indicatorBrightness ?? config.brightness;
    frame[10] = config.pressEffect ?? PressEffect.NONE;
    frame[11] = config.pollRate ?? 10;
    return frame;
  }

  buildWriteLayerFrame(layerIndex: number, layer: LayerConfig): Uint8Array {
    if (this.version === 0) {
      throw new Error('需要先读取 CH552G 配置后才能写入键位');
    }

    const frame = new Uint8Array(CH552_FRAME_SIZE);
    frame[0] = Ch552Command.WRITE_LAYER;
    frame[1] = this.version;
    frame[2] = this.deviceType;

    const startOffset = this.isMultiLayer ? 4 : 3;
    if (this.isMultiLayer) {
      frame[3] = layerIndex;
    }

    for (let i = 0; i < CH552_SLOT_COUNT; i++) {
      const action = layer.keys[i] ?? createEmptyAction();
      const encoded = this.encodeLegacyAction(action);
      const offset = startOffset + i * CH552_SLOT_SIZE;
      frame[offset] = encoded.type;
      frame[offset + 1] = (encoded.value >> 8) & 0xff;
      frame[offset + 2] = encoded.value & 0xff;
    }

    return frame;
  }

  describeOutgoingFrame(frame: Uint8Array): TerminalEntryDraft {
    const cmd = frame[0] ?? 0;
    return {
      direction: 'send',
      level: 'info',
      command: this.commandName(cmd),
      cmdHex: cmd.toString(16).padStart(2, '0'),
      sub: 0,
      dataLen: frame.length,
      rawHex: toHexDump(frame, CH552_FRAME_SIZE),
      parsed: this.describeOutgoingPayload(frame),
      glowColor: cmd === Ch552Command.READ_CONFIG || cmd === Ch552Command.READ_LAYER ? '#60a5fa' : '#4ade80',
    };
  }

  describeIncomingFrame(frame: Uint8Array): TerminalEntryDraft {
    const cmd = frame[0] ?? 0;
    return {
      direction: 'receive',
      level: 'info',
      command: this.commandName(cmd),
      cmdHex: cmd.toString(16).padStart(2, '0'),
      sub: 0,
      dataLen: frame.length,
      rawHex: toHexDump(frame, CH552_FRAME_SIZE),
      parsed: this.describeIncomingPayload(frame),
      glowColor: cmd === Ch552Command.READ_CONFIG || cmd === Ch552Command.READ_LAYER ? '#60a5fa' : '#4ade80',
    };
  }

  parseConfigBlob(data: Uint8Array): Ch552ConfigBlob {
    const payload = data.slice();
    if (payload[0] !== Ch552Command.READ_CONFIG) {
      throw new Error(`CH552G 返回了未知响应 ${this.hexByte(payload[0] ?? 0)}`);
    }

    const version = payload[1] ?? 0;
    const deviceType = payload[2] ?? 0;
    const blob: Ch552ConfigBlob = version >= FW_VERSION_MULTILAYER
      ? {
          version,
          deviceType,
          currentLayer: payload[3] ?? 0,
          maxLayers: payload[4] ?? 4,
          payload,
        }
      : {
          version,
          deviceType,
          currentLayer: 0,
          maxLayers: 1,
          payload,
        };

    this.cacheConfigBlob(blob);
    return blob;
  }

  parseLayerData(data: Uint8Array): { layerIndex: number; keys: KeyAction[] } {
    if (data[0] !== Ch552Command.READ_LAYER) {
      throw new Error(`CH552G READ_LAYER 返回了未知响应 ${this.hexByte(data[0] ?? 0)}`);
    }
    return {
      layerIndex: data[1] ?? 0,
      keys: this.decodeLayerSlots(data, 2),
    };
  }

  parseMetaInfo(data: Uint8Array): Ch552MetaInfo {
    if (data[0] !== Ch552Command.READ_META) {
      throw new Error(`CH552G READ_META 返回了未知响应 ${this.hexByte(data[0] ?? 0)}`);
    }
    const meta: Ch552MetaInfo = {
      firmwareMajor: data[1] ?? FIRMWARE_VERSION_META.CH552G.firmwareVersion.major,
      firmwareMinor: data[2] ?? FIRMWARE_VERSION_META.CH552G.firmwareVersion.minor,
      firmwarePatch: data[3] ?? FIRMWARE_VERSION_META.CH552G.firmwareVersion.patch,
    };
    this.meta = meta;
    this.metaReadSupported = true;
    return meta;
  }

  parseRgbConfig(data: Uint8Array): RgbConfig {
    if (data[0] !== Ch552Command.READ_RGB) {
      throw new Error(`CH552G READ_RGB 返回了未知响应 ${this.hexByte(data[0] ?? 0)}`);
    }

    // CH552 固件已移除 BLINK(3) 和 INDICATOR(5)，映射为 OFF
    let mode = data[2] ?? 0;
    if (mode === 5) mode = 0;

    // pollRate: bInterval (1=1000Hz, 2=500Hz, 5=200Hz, 10=100Hz)
    let pollRate = data[11] ?? 10;
    if (pollRate === 0 || pollRate === 0xFF) pollRate = 10;

    return {
      enabled: (data[1] ?? 1) !== 0,
      mode,
      brightness: data[3] ?? 0,
      speed: data[4] ?? 128,
      colorR: data[5] ?? 255,
      colorG: data[6] ?? 255,
      colorB: data[7] ?? 255,
      indicatorEnabled: (data[8] ?? 0) !== 0,
      indicatorBrightness: data[9] ?? data[3] ?? 0,
      pressEffect: data[10] ?? PressEffect.NONE,
      pollRate,
    };
  }

  toDeviceInfo(blob: Ch552ConfigBlob): DeviceInfo {
    const keyboardType = this.mapDeviceType(blob.deviceType);
    const semver = this.versionToSemver(blob.version);
    const meta = this.meta;

    return {
      vendorId: 0x1209,
      productId: 0xc55d,
      chipFamily: this.chipFamily,
      versionMajor: meta?.firmwareMajor ?? semver.major,
      versionMinor: meta?.firmwareMinor ?? semver.minor,
      versionPatch: meta?.firmwarePatch ?? semver.patch,
      maxLayers: blob.maxLayers,
      maxKeys: CH552_SLOT_COUNT,
      macroSlots: this.meowfsCache?.macros.length ?? 0,
      keyboardType,
      actualKeyCount: KeyboardTypeInfo[keyboardType].keys,
      fnKeyCount: 0,
      protocol: this.protocol,
      protocolLabel: this.protocolLabel,
      capabilities: this.capabilitiesForVersion(blob.version),
    };
  }

  toDeviceStatus(): DeviceStatus {
    return {
      workMode: 0,
      connectionState: 2,
      currentLayer: this.currentLayer,
      batteryLevel: 100,
      isCharging: false,
    };
  }

  createKeymapFromBlob(blob: Ch552ConfigBlob): KeymapConfig {
    const config = createEmptyKeymap();
    config.numLayers = blob.maxLayers;
    config.currentLayer = blob.currentLayer;
    config.defaultLayer = 0;

    if (blob.version >= FW_VERSION_MULTILAYER) {
      config.layers[0].keys = this.decodeLayerSlots(blob.payload, 5);
    } else {
      config.numLayers = 1;
      config.layers[0].keys = this.decodeLayerSlots(blob.payload, 3);
    }

    return config;
  }

  updateCurrentLayer(layerIndex: number): void {
    this.currentLayer = layerIndex;
  }

  markMetaUnsupported(): void {
    this.metaReadSupported = false;
  }

  capabilitiesForVersion(version: number): DeviceCapabilities {
    if (version >= FW_VERSION_RGB) {
      return CH552_CAPABILITIES;
    }
    return createDeviceCapabilities({
      multiLayer: version >= FW_VERSION_MULTILAYER,
      layerKeyActions: false,
      rgb: false,
      rgbOverlay: false,
      fnKeys: false,
      macroActions: version >= FW_VERSION_RGB,
      wheelClickAction: false,
      battery: false,
      logs: false,
      reset: false,
      explicitSave: false,
      wireless: false,
    });
  }

  private syncState(blob: Ch552ConfigBlob): void {
    this.version = blob.version;
    this.deviceType = blob.deviceType;
    this.maxLayers = blob.maxLayers;
    this.currentLayer = blob.currentLayer;
  }

  private cacheConfigBlob(blob: Ch552ConfigBlob): void {
    this.configBlobCache = blob;
    this.configBlobCachedAt = Date.now();
    this.syncState(blob);
  }

  private hasFreshConfigBlob(maxAgeMs: number): boolean {
    return this.configBlobCache !== null && Date.now() - this.configBlobCachedAt <= maxAgeMs;
  }

  private invalidateConfigBlobCache(): void {
    this.configBlobCache = null;
    this.configBlobCachedAt = 0;
  }

  private commandName(cmd: number): string {
    switch (cmd) {
      case Ch552Command.READ_CONFIG: return 'CH552_READ_CFG';
      case Ch552Command.WRITE_LAYER: return 'CH552_WRITE_LAYER';
      case Ch552Command.READ_LAYER: return 'CH552_READ_LAYER';
      case Ch552Command.SET_LAYER: return 'CH552_SET_LAYER';
      case Ch552Command.READ_META: return 'CH552_READ_META';
      case Ch552Command.READ_RGB: return 'CH552_READ_RGB';
      case Ch552Command.WRITE_RGB: return 'CH552_WRITE_RGB';
      case Ch552Command.FACTORY_RESET: return 'CH552_FACTORY_RESET';
      case Ch552Command.MACRO: return 'CH552_MACRO';
      default: return `CH552_CMD_${cmd.toString(16)}`;
    }
  }

  private describeOutgoingPayload(frame: Uint8Array): string {
    switch (frame[0]) {
      case Ch552Command.READ_CONFIG:
        return '读取系统信息 + Layer 0';
      case Ch552Command.WRITE_LAYER:
        return this.isMultiLayer ? `写入 Layer ${frame[3] ?? 0}` : '写入 EEPROM 键位配置';
      case Ch552Command.READ_LAYER:
        return `读取 Layer ${frame[1] ?? 0}`;
      case Ch552Command.SET_LAYER:
        return `切换当前层到 ${frame[1] ?? 0}`;
      case Ch552Command.READ_META:
        return '读取固件/协议/存储版本';
      case Ch552Command.READ_RGB:
        return '读取 RGB 配置';
      case Ch552Command.WRITE_RGB:
        return `写入 RGB: enabled=${frame[1] ?? 0} mode=${frame[2] ?? 0} press=${frame[10] ?? 0} poll=${frame[11] ?? 10}`;
      case Ch552Command.FACTORY_RESET:
        return '恢复出厂设置';
      case Ch552Command.MACRO: {
        const sub = frame[1] ?? 0;
        if (sub === Ch552MacroSub.READ) {
          const off = ((frame[2] ?? 0) << 8) | (frame[3] ?? 0);
          return `MeowFS READ off=${off} len=${frame[4] ?? 0}`;
        }
        if (sub === Ch552MacroSub.WRITE) {
          const off = ((frame[2] ?? 0) << 8) | (frame[3] ?? 0);
          return `MeowFS WRITE off=${off} len=${frame[4] ?? 0}`;
        }
        if (sub === Ch552MacroSub.ERASE) {
          const page = frame[2] ?? 0;
          return `MeowFS ERASE ${page === 0xFF ? 'ALL' : `page=${page}`}`;
        }
        if (sub === Ch552MacroSub.DELETE) {
          return `MeowFS DELETE index=${frame[2] ?? 0}`;
        }
        return 'MeowFS INFO';
      }
      default:
        return `cmd=${this.hexByte(frame[0] ?? 0)}`;
    }
  }

  private describeIncomingPayload(frame: Uint8Array): string {
    if (frame[0] === Ch552Command.READ_CONFIG) {
      const keyboardType = this.mapDeviceType(frame[2] ?? 0);
      let parsed = `版本=${this.hexByte(frame[1] ?? 0)} 设备=${KeyboardTypeInfo[keyboardType].name}`;
      if ((frame[1] ?? 0) >= FW_VERSION_MULTILAYER) {
        parsed += ` 层=${frame[3]}/${frame[4]}`;
      }
      return parsed;
    }

    if (frame[0] === Ch552Command.READ_LAYER) {
      return `Layer ${frame[1] ?? 0} 数据`;
    }

    if (frame[0] === Ch552Command.READ_META) {
      return `FW ${frame[1] ?? 0}.${frame[2] ?? 0}.${frame[3] ?? 0}`;
    }

    if (frame[0] === Ch552Command.READ_RGB) {
      return `RGB mode=${frame[2] ?? 0} bright=${frame[3] ?? 0} press=${frame[10] ?? 0}`;
    }

    if (frame[0] === Ch552Command.FACTORY_RESET) {
      return `出厂设置已恢复 meowfs=[${this.hexByte(frame[1] ?? 0)}, ${this.hexByte(frame[2] ?? 0)}]`;
    }

    if (frame[0] === Ch552Command.MACRO) {
      const sub = frame[1] ?? 0;
      if (sub === Ch552MacroSub.INFO) {
        const total = ((frame[3] ?? 0) << 8) | (frame[4] ?? 0);
        const count = frame[6] ?? 0;
        const free = ((frame[7] ?? 0) << 8) | (frame[8] ?? 0);
        return `MeowFS: total=${total} macros=${count} free=${free}`;
      }
      if (sub === Ch552MacroSub.READ) {
        return `MeowFS 数据: len=${frame[3] ?? 0}`;
      }
      if (sub === Ch552MacroSub.ERASE) {
        return `MeowFS ERASE 后首字节=[${this.hexByte(frame[3] ?? 0)}, ${this.hexByte(frame[4] ?? 0)}]`;
      }
      return `MeowFS 响应 sub=${sub}`;
    }

    return `cmd=${this.hexByte(frame[0] ?? 0)}`;
  }

  private versionToSemver(featureLevel: number): { major: number; minor: number; patch: number } {
    const legacyPatch = Number.isFinite(featureLevel) && featureLevel > 0 ? Math.trunc(featureLevel) : 0;
    return {
      // Legacy CH552 firmware without READ_META only exposes a feature level byte.
      // Map it to the CH552 major line so Studio keeps a consistent version UI.
      major: 2,
      minor: 0,
      patch: legacyPatch,
    };
  }

  private mapDeviceType(deviceType: number): KeyboardType {
    switch (deviceType) {
      case 0x01: return KeyboardType.BASIC;
      case 0x02: return KeyboardType.KNOB;
      case 0x03: return KeyboardType.FIVE_KEYS;
      default: return KeyboardType.BASIC;
    }
  }

  private hexByte(value: number): string {
    return `0x${value.toString(16).padStart(2, '0').toUpperCase()}`;
  }

  private wheelDirectionToScroll(direction: number): number {
    switch (direction) {
      case WheelDirection.UP: return 1;
      case WheelDirection.DOWN: return 0xff;
      default: return 0;
    }
  }

  private decodeLegacyAction(type: number, value: number): KeyAction {
    switch (type) {
      case LegacyCh552KeyType.KEYBOARD:
        return { type: ActionType.KEYBOARD, modifier: (value >> 8) & 0xff, param1: value & 0xff, param2: 0 };
      case LegacyCh552KeyType.MEDIA:
        return { type: ActionType.CONSUMER, modifier: 0, param1: value & 0xff, param2: (value >> 8) & 0xff };
      case LegacyCh552KeyType.MOUSE: {
        const button = value & 0xff;
        const scroll = (value >> 8) & 0xff;
        if (button !== 0) return { type: ActionType.MOUSE_BTN, modifier: 0, param1: button, param2: 0 };
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
      case LegacyCh552KeyType.MACRO:
        // valueHi=trigger, valueLo=slot
        return { type: ActionType.MACRO, modifier: (value >> 8) & 0xff, param1: value & 0xff, param2: 0 };
      default:
        return createEmptyAction();
    }
  }

  private encodeLegacyAction(action: KeyAction): { type: number; value: number } {
    switch (action.type) {
      case ActionType.NONE:
        return { type: LegacyCh552KeyType.INVALID, value: 0 };
      case ActionType.KEYBOARD:
        return { type: LegacyCh552KeyType.KEYBOARD, value: ((action.modifier & 0xff) << 8) | (action.param1 & 0xff) };
      case ActionType.CONSUMER:
        return { type: LegacyCh552KeyType.MEDIA, value: ((action.param2 & 0xff) << 8) | (action.param1 & 0xff) };
      case ActionType.MOUSE_BTN:
        return { type: LegacyCh552KeyType.MOUSE, value: action.param1 & 0xff };
      case ActionType.MOUSE_WHEEL:
        if (action.param1 === WheelDirection.CLICK) {
          return { type: LegacyCh552KeyType.MOUSE, value: MouseButton.MIDDLE };
        }
        return { type: LegacyCh552KeyType.MOUSE, value: (this.wheelDirectionToScroll(action.param1) & 0xff) << 8 };
      case ActionType.MACRO:
        // modifier=trigger, param1=slot
        return { type: LegacyCh552KeyType.MACRO, value: ((action.modifier & 0xff) << 8) | (action.param1 & 0xff) };
      default:
        throw new Error('当前 CH552G 固件不支持该动作类型');
    }
  }

  private decodeLayerSlots(data: Uint8Array, offset: number): KeyAction[] {
    const keys: KeyAction[] = [];
    for (let i = 0; i < CH552_SLOT_COUNT; i++) {
      const slotOffset = offset + i * CH552_SLOT_SIZE;
      const type = data[slotOffset] ?? 0;
      const value = ((data[slotOffset + 1] ?? 0) << 8) | (data[slotOffset + 2] ?? 0);
      keys.push(this.decodeLegacyAction(type, value));
    }
    return keys;
  }

  parseIncomingPacket(frame: Uint8Array): CodecInboundPacket<Uint8Array> {
    return {
      kind: 'response',
      entry: this.describeIncomingFrame(frame),
      response: frame,
    };
  }

  async getSysInfo(transport: CodecTransport<Uint8Array>): Promise<DeviceInfo> {
    const blob = await this.readConfigBlob(transport);
    if (this.shouldTryReadMeta) {
      try {
        await this.readMetaInfo(transport);
      } catch {
        this.markMetaUnsupported();
      }
    }
    if (this.configBlobCache) {
      this.configBlobCachedAt = Date.now();
    }
    return this.toDeviceInfo(blob);
  }

  async getSysStatus(transport: CodecTransport<Uint8Array>): Promise<DeviceStatus> {
    if (this.cachedVersion === 0) {
      await this.readConfigBlob(transport, { force: true });
    } else if (this.isMultiLayer && !this.hasFreshConfigBlob(1000)) {
      await this.readConfigBlob(transport, { force: true });
    }
    return this.toDeviceStatus();
  }

  async getFullKeymap(transport: CodecTransport<Uint8Array>): Promise<KeymapConfig> {
    const blob = await this.readConfigBlob(transport);
    const config = this.createKeymapFromBlob(blob);

    if (this.isMultiLayer) {
      for (let layer = 1; layer < blob.maxLayers; layer++) {
        const parsed = await this.readLayerData(transport, layer);
        config.layers[layer].keys = parsed.keys;
      }
    }

    return config;
  }

  async setFullKeymap(transport: CodecTransport<Uint8Array>, config: KeymapConfig): Promise<void> {
    if (this.cachedVersion === 0) {
      await this.readConfigBlob(transport);
    }

    const numLayers = Math.min(config.numLayers, this.cachedMaxLayers);
    for (let layer = 0; layer < numLayers; layer++) {
      const layerConfig = config.layers[layer] ?? createEmptyKeymap().layers[0];
      const frame = this.buildWriteLayerFrame(layer, layerConfig);
      await transport.sendNoWait(frame);
      await new Promise((resolve) => setTimeout(resolve, 50));
    }

    if (this.isMultiLayer) {
      const targetLayer = Math.min(config.currentLayer, Math.max(0, numLayers - 1));
      await transport.sendNoWait(this.buildSetLayerFrame(targetLayer));
      this.updateCurrentLayer(targetLayer);
    }
    this.invalidateConfigBlobCache();
  }

  private async readConfigBlob(
    transport: CodecTransport<Uint8Array>,
    options: { force?: boolean } = {},
  ): Promise<Ch552ConfigBlob> {
    if (!options.force && this.configBlobCache) {
      return this.configBlobCache;
    }
    const data = await this.readWithRetry('CH552G 配置读取超时', () => transport.sendAndWait(this.buildReadConfigFrame(), {
      timeout: 1500,
      timeoutLabel: 'CH552G 配置读取超时',
    }));
    return this.parseConfigBlob(data);
  }

  private async readLayerData(
    transport: CodecTransport<Uint8Array>,
    layerIndex: number,
  ): Promise<{ layerIndex: number; keys: KeyAction[] }> {
    const data = await this.readWithRetry('CH552G 层读取超时', () => transport.sendAndWait(this.buildReadLayerFrame(layerIndex), {
      timeout: 1500,
      timeoutLabel: 'CH552G 层读取超时',
    }));
    return this.parseLayerData(data);
  }

  private async readMetaInfo(transport: CodecTransport<Uint8Array>): Promise<Ch552MetaInfo> {
    const data = await this.readWithRetry('CH552G 元信息读取超时', () => transport.sendAndWait(this.buildReadMetaFrame(), {
      timeout: 1500,
      timeoutLabel: 'CH552G 元信息读取超时',
    }));
    return this.parseMetaInfo(data);
  }

  private async getRgbConfig(transport: CodecTransport<Uint8Array>): Promise<RgbConfig> {
    const data = await this.readWithRetry('CH552G RGB 配置读取超时', () => transport.sendAndWait(this.buildReadRgbFrame(), {
      timeout: 1500,
      timeoutLabel: 'CH552G RGB 配置读取超时',
    }));
    return this.parseRgbConfig(data);
  }

  private async setRgbConfig(transport: CodecTransport<Uint8Array>, config: RgbConfig): Promise<void> {
    await transport.sendNoWait(this.buildWriteRgbFrame(config));
    await new Promise((resolve) => setTimeout(resolve, 50));
  }

  private async resetConfig(transport: CodecTransport<Uint8Array>): Promise<void> {
    const frame = new Uint8Array(CH552_FRAME_SIZE);
    frame[0] = Ch552Command.FACTORY_RESET;
    await this.readWithRetry('CH552G 恢复出厂超时', () =>
      transport.sendAndWait(frame, {
        timeout: 3000,
        timeoutLabel: 'CH552G 恢复出厂超时',
      }),
    );
    // CH552 的宏区在代码 Flash 尾部，显式再擦一次 MeowFS，
    // 避免旧固件或异常状态下“恢复出厂”未同步清空宏区。
    await this.eraseAllFs(transport);
    this.invalidateConfigBlobCache();
    this.meowfsCache = null;
  }

  // ==========================================================================
  // MeowFS 宏操作 (CH552G: 1KB Flash, 动态宏存储)
  //
  // Frame 布局 (frame → Ep1Buffer 映射):
  //   frame[0] → Ep1Buffer[1] = 0x40 (MACRO cmd)
  //   frame[1] → Ep1Buffer[2] = sub
  //   frame[2..] → Ep1Buffer[3..] = params
  //
  // 子命令:
  //   INFO(0)   : → [total_hi, total_lo, page_size, macro_count, free_hi, free_lo]
  //   READ(1)   : [off_hi, off_lo, len] → [len, data...]
  //   ERASE(2)  : [page | 0xFF=all]
  //   WRITE(3)  : [off_hi, off_lo, len, data...]
  //   DELETE(4)  : [macro_index]
  // ==========================================================================

  private buildMeowFsFrame(sub: Ch552MacroSub, params?: Uint8Array): Uint8Array {
    const frame = new Uint8Array(CH552_FRAME_SIZE);
    frame[0] = Ch552Command.MACRO;
    frame[1] = sub;
    if (params) frame.set(params, 2);
    return frame;
  }

  private async sendMeowFsCmd(
    transport: CodecTransport<Uint8Array>,
    sub: Ch552MacroSub,
    params?: Uint8Array,
  ): Promise<Uint8Array> {
    const frame = this.buildMeowFsFrame(sub, params);
    return this.readWithRetry('MeowFS 命令超时', () =>
      transport.sendAndWait(frame, {
        timeout: 2000,
        timeoutLabel: 'MeowFS 命令超时',
      }),
    );
  }

  /** 分块读取 MeowFS 原始数据 */
  private async readFsChunked(
    transport: CodecTransport<Uint8Array>,
    offset: number,
    length: number,
  ): Promise<Uint8Array> {
    const result = new Uint8Array(length);
    let pos = 0;
    while (pos < length) {
      const chunkLen = Math.min(MEOWFS_READ_CHUNK, length - pos);
      const absOff = offset + pos;
      const resp = await this.sendMeowFsCmd(
        transport,
        Ch552MacroSub.READ,
        new Uint8Array([(absOff >> 8) & 0xFF, absOff & 0xFF, chunkLen]),
      );
      const readLen = resp[3] ?? 0;
      for (let i = 0; i < readLen && pos + i < length; i++) {
        result[pos + i] = resp[4 + i] ?? 0;
      }
      pos += readLen || chunkLen;
    }
    return result;
  }

  /** 分块写入 MeowFS 原始数据 */
  private async writeFsChunked(
    transport: CodecTransport<Uint8Array>,
    offset: number,
    data: Uint8Array,
  ): Promise<void> {
    let pos = 0;
    while (pos < data.length) {
      const chunkLen = Math.min(MEOWFS_WRITE_CHUNK, data.length - pos);
      const absOff = offset + pos;
      const params = new Uint8Array(3 + chunkLen);
      params[0] = (absOff >> 8) & 0xFF;
      params[1] = absOff & 0xFF;
      params[2] = chunkLen;
      params.set(data.subarray(pos, pos + chunkLen), 3);
      await this.sendMeowFsCmd(transport, Ch552MacroSub.WRITE, params);
      await new Promise((resolve) => setTimeout(resolve, 20));
      pos += chunkLen;
    }
  }

  /** 擦除整个 MeowFS 区域 */
  private async eraseAllFs(transport: CodecTransport<Uint8Array>): Promise<void> {
    await this.sendMeowFsCmd(transport, Ch552MacroSub.ERASE, new Uint8Array([0xFF]));
    await new Promise((resolve) => setTimeout(resolve, 50));
  }

  /** 解析 MeowFS 原始数据为宏列表 */
  private parseFsData(raw: Uint8Array): MeowFsMacroEntry[] {
    const macros: MeowFsMacroEntry[] = [];
    let pos = 0;
    while (pos + CH552_MEOWFS_HEADER_SIZE <= raw.length) {
      const marker = raw[pos];
      const count = raw[pos + 1];
      // 终止: 0xFF=空闲, {0x00,0x00}=格式化写零兜底
      if (marker === 0xFF || (marker === 0x00 && count === 0x00)) break;
      const entrySize = CH552_MEOWFS_HEADER_SIZE + count * CH552_MEOWFS_ACTION_SIZE;
      if (pos + entrySize > raw.length) break;
      if (marker === MEOWFS_VALID_MARKER) {
        const actions: MacroAction[] = [];
        for (let i = 0; i < count; i++) {
          actions.push({
            type: raw[pos + CH552_MEOWFS_HEADER_SIZE + i * 2] as MacroActionType,
            param: raw[pos + CH552_MEOWFS_HEADER_SIZE + i * 2 + 1],
          });
        }
        macros.push({ actionCount: count, actions });
      }
      pos += entrySize;
    }
    return macros;
  }

  /** 序列化宏列表为 MeowFS 原始数据 */
  private serializeMacros(macros: MeowFsMacroEntry[]): Uint8Array {
    let totalSize = 0;
    for (const m of macros) {
      totalSize += CH552_MEOWFS_HEADER_SIZE + m.actionCount * CH552_MEOWFS_ACTION_SIZE;
    }
    const buf = new Uint8Array(totalSize);
    let pos = 0;
    for (const m of macros) {
      buf[pos] = MEOWFS_VALID_MARKER;
      buf[pos + 1] = m.actionCount;
      for (let i = 0; i < m.actionCount; i++) {
        buf[pos + CH552_MEOWFS_HEADER_SIZE + i * 2] = m.actions[i].type;
        buf[pos + CH552_MEOWFS_HEADER_SIZE + i * 2 + 1] = m.actions[i].param;
      }
      pos += CH552_MEOWFS_HEADER_SIZE + m.actionCount * CH552_MEOWFS_ACTION_SIZE;
    }
    return buf;
  }

  /** 读取并缓存 MeowFS 完整状态 */
  private async ensureMeowFsCache(transport: CodecTransport<Uint8Array>): Promise<MeowFsCache> {
    if (this.meowfsCache) return this.meowfsCache;

    // INFO → [0x40, sub, 0, total_hi, total_lo, page_size, macro_count, free_hi, free_lo]
    const info = await this.sendMeowFsCmd(transport, Ch552MacroSub.INFO);
    const fsTotal = ((info[3] ?? 0) << 8) | (info[4] ?? 0);
    const macroCount = info[6] ?? 0;
    const fsFree = ((info[7] ?? 0) << 8) | (info[8] ?? 0);
    const usedBytes = fsTotal - fsFree;

    let macros: MeowFsMacroEntry[] = [];
    if (macroCount > 0 && usedBytes > 0) {
      const raw = await this.readFsChunked(transport, 0, usedBytes);
      macros = this.parseFsData(raw);
    }

    this.meowfsCache = { fsTotal, fsFree, macros };
    return this.meowfsCache;
  }

  private async getMacroOverview(transport: CodecTransport<Uint8Array>): Promise<MacroOverview> {
    this.meowfsCache = null; // 强制刷新
    const cache = await this.ensureMeowFsCache(transport);
    const count = cache.macros.length;
    const totalSlots = count + CH552_MEOWFS_APPEND_SLOTS;
    return {
      totalSlots,
      usedCount: count,
      slotValid: [
        ...new Array(count).fill(true),
        ...new Array(totalSlots - count).fill(false),
      ],
      dynamic: true,
      fsTotal: cache.fsTotal,
      fsFree: cache.fsFree,
    };
  }

  private async getMacroInfo(transport: CodecTransport<Uint8Array>, index: number): Promise<MacroHeader> {
    const cache = await this.ensureMeowFsCache(transport);
    if (index >= cache.macros.length) {
      return { valid: 0, id: index, actionCount: 0, dataSize: 0, name: '' };
    }
    const m = cache.macros[index];
    return {
      valid: 1,
      id: index,
      actionCount: m.actionCount,
      dataSize: m.actionCount * CH552_MEOWFS_ACTION_SIZE,
      name: '',
    };
  }

  private async getMacroData(transport: CodecTransport<Uint8Array>, index: number): Promise<MacroData> {
    const cache = await this.ensureMeowFsCache(transport);
    if (index >= cache.macros.length) {
      return {
        header: { valid: 0, id: index, actionCount: 0, dataSize: 0, name: '' },
        actions: [{ type: MacroActionType.END, param: 0 }],
      };
    }
    const m = cache.macros[index];
    const actions: MacroAction[] = m.actions.map((a) => ({ ...a }));
    actions.push({ type: MacroActionType.END, param: 0 });
    return {
      header: {
        valid: 1,
        id: index,
        actionCount: m.actionCount,
        dataSize: m.actionCount * CH552_MEOWFS_ACTION_SIZE,
        name: '',
      },
      actions,
    };
  }

  private async setMacroData(
    transport: CodecTransport<Uint8Array>,
    index: number,
    macro: MacroData,
  ): Promise<void> {
    const cache = await this.ensureMeowFsCache(transport);
    const macros = cache.macros.map((m) => ({
      actionCount: m.actionCount,
      actions: m.actions.map((a) => ({ ...a })),
    }));

    // 序列化新宏 (不含 END)
    const actionsNoEnd = macro.actions.filter((a) => a.type !== MacroActionType.END);
    if (actionsNoEnd.length > CH552_MEOWFS_MAX_ACTIONS) {
      throw new Error(`动作数 ${actionsNoEnd.length} 超过上限 ${CH552_MEOWFS_MAX_ACTIONS}`);
    }

    const newEntry: MeowFsMacroEntry = {
      actionCount: actionsNoEnd.length,
      actions: actionsNoEnd,
    };

    if (index < macros.length) {
      macros[index] = newEntry; // 替换
    } else if (index === macros.length) {
      macros.push(newEntry); // 追加
    } else {
      throw new Error(`无效的宏索引 ${index}`);
    }

    // 序列化并检查空间
    const buf = this.serializeMacros(macros);
    if (buf.length > cache.fsTotal) {
      throw new Error(`宏数据总计 ${buf.length} 字节，超过 MeowFS 容量 ${cache.fsTotal} 字节`);
    }

    // 擦除 → 全量写回
    await this.eraseAllFs(transport);
    await this.writeFsChunked(transport, 0, buf);
    this.meowfsCache = null;
  }

  private async deleteMacro(transport: CodecTransport<Uint8Array>, index: number): Promise<void> {
    const cache = await this.ensureMeowFsCache(transport);
    if (index >= cache.macros.length) {
      throw new Error(`宏索引 ${index} 不存在`);
    }

    await this.sendMeowFsCmd(transport, Ch552MacroSub.DELETE, new Uint8Array([index]));
    this.meowfsCache = null;
  }

  private async readWithRetry<T>(label: string, task: () => Promise<T>): Promise<T> {
    let lastError: unknown;
    for (let attempt = 0; attempt < CH552_READ_RETRY_COUNT; attempt++) {
      try {
        return await task();
      } catch (error) {
        lastError = error;
        if (attempt + 1 >= CH552_READ_RETRY_COUNT) {
          break;
        }
        // 首次失败多等一会儿，让设备 USB 栈稳定（尤其是首次连接场景）
        await new Promise((resolve) => setTimeout(resolve, attempt === 0 ? 500 : 80));
      }
    }

    if (lastError instanceof Error) {
      throw lastError;
    }
    throw new Error(label);
  }
}
