import {
  ActionType,
  CH552_CAPABILITIES,
  CH552_FRAME_SIZE,
  DeviceProtocol,
  KeyboardType,
  KeyboardTypeInfo,
  MouseButton,
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

enum Ch552Command {
  READ_CONFIG = 0x01,
  WRITE_LAYER = 0x02,
  READ_LAYER = 0x03,
  SET_LAYER = 0x04,
  READ_META = 0x05,
}

enum LegacyCh552KeyType {
  INVALID = 0x00,
  KEYBOARD = 0x01,
  MEDIA = 0x02,
  MOUSE = 0x03,
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
  protocolMajor: number;
  protocolMinor: number;
  storageMajor: number;
  storageMinor: number;
}

export class Ch552Codec implements DeviceCodec<Uint8Array> {
  readonly protocol = DeviceProtocol.CH552;
  readonly protocolLabel = 'CH552G USB';
  readonly chipFamily = FIRMWARE_VERSION_META.CH552G.chipFamily;
  readonly protocolFamily = FIRMWARE_VERSION_META.CH552G.protocolFamily;
  readonly protocolVersionMajor = FIRMWARE_VERSION_META.CH552G.protocolVersion.major;
  readonly protocolVersionMinor = FIRMWARE_VERSION_META.CH552G.protocolVersion.minor;
  readonly storageVersionMajor = FIRMWARE_VERSION_META.CH552G.storageVersion.major;
  readonly storageVersionMinor = FIRMWARE_VERSION_META.CH552G.storageVersion.minor;

  private version = 0;
  private deviceType = 0;
  private maxLayers = 1;
  private currentLayer = 0;
  private meta: Ch552MetaInfo | null = null;
  private metaReadSupported: boolean | null = null;

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

  getOptionalOperations(_transport: CodecTransport<Uint8Array>): HidOptionalOperations {
    return {};
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

  buildSetLayerFrame(layerIndex: number): Uint8Array {
    const frame = new Uint8Array(CH552_FRAME_SIZE);
    frame[0] = Ch552Command.SET_LAYER;
    frame[1] = layerIndex;
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
      protocolMajor: data[4] ?? this.protocolVersionMajor,
      protocolMinor: data[5] ?? this.protocolVersionMinor,
      storageMajor: data[6] ?? this.storageVersionMajor,
      storageMinor: data[7] ?? this.storageVersionMinor,
    };
    this.meta = meta;
    this.metaReadSupported = true;
    return meta;
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
      protocolFamily: this.protocolFamily,
      protocolVersionMajor: meta?.protocolMajor ?? this.protocolVersionMajor,
      protocolVersionMinor: meta?.protocolMinor ?? this.protocolVersionMinor,
      storageVersionMajor: meta?.storageMajor ?? this.storageVersionMajor,
      storageVersionMinor: meta?.storageMinor ?? this.storageVersionMinor,
      maxLayers: blob.maxLayers,
      maxKeys: CH552_SLOT_COUNT,
      macroSlots: 0,
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
    if (version >= FW_VERSION_MULTILAYER) {
      return CH552_CAPABILITIES;
    }
    return createDeviceCapabilities({
      multiLayer: false,
      layerKeyActions: false,
      rgb: false,
      fnKeys: false,
      macroActions: false,
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
      return `FW ${frame[1] ?? 0}.${frame[2] ?? 0}.${frame[3] ?? 0}  Proto ${frame[4] ?? 0}.${frame[5] ?? 0}  Storage ${frame[6] ?? 0}.${frame[7] ?? 0}`;
    }

    return `cmd=${this.hexByte(frame[0] ?? 0)}`;
  }

  private versionToSemver(versionByte: number): { major: number; minor: number; patch: number } {
    return {
      major: FIRMWARE_VERSION_META.CH552G.firmwareVersion.major,
      minor: FIRMWARE_VERSION_META.CH552G.firmwareVersion.minor,
      patch: versionByte & 0xff,
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
    const data = await transport.sendAndWait(this.buildReadConfigFrame(), {
      timeout: 1500,
      timeoutLabel: 'CH552G 配置读取超时',
    });
    return this.parseConfigBlob(data);
  }

  private async readLayerData(
    transport: CodecTransport<Uint8Array>,
    layerIndex: number,
  ): Promise<{ layerIndex: number; keys: KeyAction[] }> {
    const data = await transport.sendAndWait(this.buildReadLayerFrame(layerIndex), {
      timeout: 1500,
      timeoutLabel: 'CH552G 层读取超时',
    });
    return this.parseLayerData(data);
  }

  private async readMetaInfo(transport: CodecTransport<Uint8Array>): Promise<Ch552MetaInfo> {
    const data = await transport.sendAndWait(this.buildReadMetaFrame(), {
      timeout: 1500,
      timeoutLabel: 'CH552G 元信息读取超时',
    });
    return this.parseMetaInfo(data);
  }
}
