import {
  ActionType,
  CH552_CAPABILITIES,
  CH552_FRAME_SIZE,
  CH552_MACRO_SLOTS,
  CH552_MACRO_SLOT_SIZE,
  CH552_MACRO_MAX_ACTIONS,
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
  MACRO = 0x40,
}

/** CH552G 宏 HID 子命令 */
enum Ch552MacroSub {
  INFO = 0,
  READ = 1,
  ERASE = 2,
  WRITE = 3,
}

/** CH552G 宏头部字节数 (valid + action_count) */
const CH552_MACRO_HEADER_SIZE = 2;
const CH552_MACRO_VALID_MARKER = 0xaa;
/** 单次 READ 最大字节数 (CustomBuf 限制) */
const CH552_MACRO_READ_CHUNK = 26;
/** 单次 WRITE 最大字节数 */
const CH552_MACRO_WRITE_CHUNK = 22;

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
  private meta: Ch552MetaInfo | null = null;
  private metaReadSupported: boolean | null = null;

  resetState(): void {
    this.version = 0;
    this.deviceType = 0;
    this.maxLayers = 1;
    this.currentLayer = 0;
    this.meta = null;
    this.metaReadSupported = null;
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

  getOptionalOperations(transport: CodecTransport<Uint8Array>): HidOptionalOperations {
    return {
      getRgbConfig: () => this.getRgbConfig(transport),
      setRgbConfig: (config) => this.setRgbConfig(transport, config),
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
    if (data[0] !== Ch552Command.READ_CONFIG) {
      throw new Error(`CH552G 返回了未知响应 ${this.hexByte(data[0] ?? 0)}`);
    }

    const version = data[1] ?? 0;
    const deviceType = data[2] ?? 0;
    const blob: Ch552ConfigBlob = version >= FW_VERSION_MULTILAYER
      ? {
          version,
          deviceType,
          currentLayer: data[3] ?? 0,
          maxLayers: data[4] ?? 4,
          payload: data,
        }
      : {
          version,
          deviceType,
          currentLayer: 0,
          maxLayers: 1,
          payload: data,
        };

    this.syncState(blob);
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

    return {
      enabled: (data[1] ?? 1) !== 0,
      mode: data[2] ?? 0,
      brightness: data[3] ?? 0,
      speed: data[4] ?? 128,
      colorR: data[5] ?? 255,
      colorG: data[6] ?? 255,
      colorB: data[7] ?? 255,
      indicatorEnabled: (data[8] ?? 0) !== 0,
      indicatorBrightness: data[9] ?? data[3] ?? 0,
      pressEffect: data[10] ?? PressEffect.NONE,
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
      macroSlots: CH552_MACRO_SLOTS,
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

  private commandName(cmd: number): string {
    switch (cmd) {
      case Ch552Command.READ_CONFIG: return 'CH552_READ_CFG';
      case Ch552Command.WRITE_LAYER: return 'CH552_WRITE_LAYER';
      case Ch552Command.READ_LAYER: return 'CH552_READ_LAYER';
      case Ch552Command.SET_LAYER: return 'CH552_SET_LAYER';
      case Ch552Command.READ_META: return 'CH552_READ_META';
      case Ch552Command.READ_RGB: return 'CH552_READ_RGB';
      case Ch552Command.WRITE_RGB: return 'CH552_WRITE_RGB';
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
        return `写入 RGB: enabled=${frame[1] ?? 0} mode=${frame[2] ?? 0} press=${frame[10] ?? 0}`;
      case Ch552Command.MACRO: {
        const sub = frame[1] ?? 0;
        const names = ['INFO', 'READ', 'ERASE', 'WRITE'];
        return `宏 ${names[sub] ?? sub} slot=${frame[2] ?? 0}`;
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

    if (frame[0] === Ch552Command.MACRO) {
      const sub = frame[1] ?? 0;
      if (sub === Ch552MacroSub.INFO) {
        return `宏概览: slots=${frame[3] ?? 0} size=${frame[4] ?? 0} valid=[${frame[5] ?? 0},${frame[6] ?? 0}]`;
      }
      if (sub === Ch552MacroSub.READ) {
        return `宏数据: len=${frame[3] ?? 0}`;
      }
      return `宏响应 sub=${sub}`;
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
    return this.toDeviceInfo(blob);
  }

  async getSysStatus(transport: CodecTransport<Uint8Array>): Promise<DeviceStatus> {
    if (this.cachedVersion === 0 || this.isMultiLayer) {
      await this.readConfigBlob(transport);
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
  }

  private async readConfigBlob(transport: CodecTransport<Uint8Array>): Promise<Ch552ConfigBlob> {
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

  // ==========================================================================
  // 宏操作 (CH552G: 2 slots × 64B, 无名称, Flash 存储)
  // ==========================================================================

  /**
   * 构建 CH552G 宏命令帧
   *
   * transport reportId=0x04 映射到 Ep1Buffer[0]=4,
   * frame 内容从 Ep1Buffer[1] 开始:
   *   frame[0] → Ep1Buffer[1] = cmd (0x40)
   *   frame[1] → Ep1Buffer[2] = sub
   *   frame[2] → Ep1Buffer[3] = slot
   *   frame[3] → Ep1Buffer[4] = (unused)
   *   frame[4] → Ep1Buffer[5] = offset  (READ/WRITE)
   *   frame[5] → Ep1Buffer[6] = length  (READ/WRITE)
   *   frame[6..] → Ep1Buffer[7..] = data (WRITE)
   */
  private buildMacroFrame(sub: Ch552MacroSub, slot: number, offset?: number, length?: number, data?: Uint8Array): Uint8Array {
    const frame = new Uint8Array(CH552_FRAME_SIZE);
    frame[0] = Ch552Command.MACRO;
    frame[1] = sub;
    frame[2] = slot;
    // frame[3] unused (Ep1Buffer[4])
    if (offset !== undefined) frame[4] = offset;
    if (length !== undefined) frame[5] = length;
    if (data) frame.set(data, 6);
    return frame;
  }

  private async sendMacroCmd(
    transport: CodecTransport<Uint8Array>,
    sub: Ch552MacroSub,
    slot: number,
    offset?: number,
    length?: number,
    data?: Uint8Array,
  ): Promise<Uint8Array> {
    const frame = this.buildMacroFrame(sub, slot, offset, length, data);
    return this.readWithRetry('CH552G 宏命令超时', () =>
      transport.sendAndWait(frame, {
        timeout: 2000,
        timeoutLabel: 'CH552G 宏命令超时',
      }),
    );
  }

  private async getMacroOverview(transport: CodecTransport<Uint8Array>): Promise<MacroOverview> {
    const resp = await this.sendMacroCmd(transport, Ch552MacroSub.INFO, 0);
    // CustomBuf: [0]=0x40, [1]=sub, [2]=0, [3]=slotCount, [4]=slotSize, [5]=valid0, [6]=valid1
    const totalSlots = resp[3] ?? CH552_MACRO_SLOTS;
    const slotValid: boolean[] = [];
    let usedCount = 0;
    for (let i = 0; i < totalSlots; i++) {
      const valid = (resp[5 + i] ?? 0) !== 0;
      slotValid.push(valid);
      if (valid) usedCount++;
    }
    return { totalSlots, usedCount, slotValid };
  }

  private async getMacroInfo(transport: CodecTransport<Uint8Array>, slot: number): Promise<MacroHeader> {
    if (slot >= CH552_MACRO_SLOTS) {
      throw new Error(`CH552G 宏槽位 ${slot} 超出范围 (最大 ${CH552_MACRO_SLOTS - 1})`);
    }
    // 读取槽位头部 2 字节: [valid, actionCount]
    const resp = await this.sendMacroCmd(transport, Ch552MacroSub.READ, slot, 0, CH552_MACRO_HEADER_SIZE);
    // CustomBuf: [0]=0x40, [1]=1, [2]=0, [3]=len, [4..]=data
    const len = resp[3] ?? 0;
    const valid = len >= 2 && resp[4] === CH552_MACRO_VALID_MARKER ? 1 : 0;
    const actionCount = valid ? (resp[5] ?? 0) : 0;
    const dataSize = actionCount * 2;
    return { valid, id: slot, actionCount, dataSize, name: '' };
  }

  private async getMacroData(transport: CodecTransport<Uint8Array>, slot: number): Promise<MacroData> {
    const header = await this.getMacroInfo(transport, slot);
    if (!header.valid || header.actionCount === 0) {
      return { header, actions: [{ type: MacroActionType.END, param: 0 }] };
    }

    // 读取动作数据（跳过 2 字节头部）
    const totalBytes = header.actionCount * 2;
    const raw = new Uint8Array(totalBytes);
    let readOffset = 0;

    while (readOffset < totalBytes) {
      const chunkSize = Math.min(CH552_MACRO_READ_CHUNK, totalBytes - readOffset);
      const flashOffset = CH552_MACRO_HEADER_SIZE + readOffset;
      const resp = await this.sendMacroCmd(transport, Ch552MacroSub.READ, slot, flashOffset, chunkSize);
      const readLen = resp[3] ?? 0;
      for (let i = 0; i < readLen && readOffset + i < totalBytes; i++) {
        raw[readOffset + i] = resp[4 + i] ?? 0;
      }
      readOffset += readLen || chunkSize;
    }

    const actions: MacroAction[] = [];
    for (let i = 0; i + 1 < raw.length; i += 2) {
      const type = raw[i] as MacroActionType;
      const param = raw[i + 1];
      actions.push({ type, param });
      if (type === MacroActionType.END) break;
    }
    if (actions.length === 0 || actions[actions.length - 1].type !== MacroActionType.END) {
      actions.push({ type: MacroActionType.END, param: 0 });
    }

    return { header, actions };
  }

  private async setMacroData(
    transport: CodecTransport<Uint8Array>,
    slot: number,
    macro: MacroData,
  ): Promise<void> {
    if (slot >= CH552_MACRO_SLOTS) {
      throw new Error(`CH552G 宏槽位 ${slot} 超出范围`);
    }

    // 序列化动作 (不含 END — 固件用 actionCount 控制)
    const actionsNoEnd = macro.actions.filter((a) => a.type !== MacroActionType.END);
    if (actionsNoEnd.length > CH552_MACRO_MAX_ACTIONS) {
      throw new Error(`动作数 ${actionsNoEnd.length} 超过 CH552G 上限 ${CH552_MACRO_MAX_ACTIONS}`);
    }

    // 构建完整槽位数据: [valid, count, ...actions]
    const slotData = new Uint8Array(CH552_MACRO_HEADER_SIZE + actionsNoEnd.length * 2);
    slotData[0] = CH552_MACRO_VALID_MARKER;
    slotData[1] = actionsNoEnd.length;
    for (let i = 0; i < actionsNoEnd.length; i++) {
      slotData[CH552_MACRO_HEADER_SIZE + i * 2] = actionsNoEnd[i].type;
      slotData[CH552_MACRO_HEADER_SIZE + i * 2 + 1] = actionsNoEnd[i].param;
    }

    // 先擦除
    await this.sendMacroCmd(transport, Ch552MacroSub.ERASE, slot);
    await new Promise((resolve) => setTimeout(resolve, 30));

    // 分块写入 (firmware 每次最多写 22 字节)
    let writeOffset = 0;
    while (writeOffset < slotData.length) {
      const chunkSize = Math.min(CH552_MACRO_WRITE_CHUNK, slotData.length - writeOffset);
      await this.sendMacroCmd(
        transport,
        Ch552MacroSub.WRITE,
        slot,
        writeOffset,
        chunkSize,
        slotData.slice(writeOffset, writeOffset + chunkSize),
      );
      await new Promise((resolve) => setTimeout(resolve, 20));
      writeOffset += chunkSize;
    }
  }

  private async deleteMacro(transport: CodecTransport<Uint8Array>, slot: number): Promise<void> {
    if (slot >= CH552_MACRO_SLOTS) {
      throw new Error(`CH552G 宏槽位 ${slot} 超出范围`);
    }
    await this.sendMacroCmd(transport, Ch552MacroSub.ERASE, slot);
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
        await new Promise((resolve) => setTimeout(resolve, 80));
      }
    }

    if (lastError instanceof Error) {
      throw lastError;
    }
    throw new Error(label);
  }
}
