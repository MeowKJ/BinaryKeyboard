import {
  CH592_CAPABILITIES,
  Command,
  DeviceProtocol,
  FRAME_SIZE,
  MAX_FN_KEYS,
  MAX_KEYS,
  ResponseCode,
  createEmptyAction,
  createEmptyKeymap,
  type DeviceInfo,
  type DeviceStatus,
  type FnKeyConfig,
  type FnKeyEntry,
  type KeyAction,
  type LayerConfig,
  type LogConfig,
  type KeymapConfig,
  type RgbConfig,
} from '@/types/protocol';
import { FIRMWARE_VERSION_META } from '@/generated/versionConfig';
import { parseLogFrame, parseReceiveFrame, parseSendFrame } from '@/utils/protocolParser';
import type { BatteryInfo, HidOptionalOperations } from '../../common/types';
import type {
  CodecInboundPacket,
  CodecTransport,
  DeviceCodec,
  TerminalEntryDraft,
} from '../../common/codecTypes';

const RESP_HEADER_SIZE = 3;

export class Ch592Codec implements DeviceCodec<DataView> {
  readonly protocol = DeviceProtocol.CH592;
  readonly protocolLabel = 'CH592F HID';
  readonly capabilities = CH592_CAPABILITIES;
  readonly chipFamily = FIRMWARE_VERSION_META.CH592F.chipFamily;
  readonly protocolFamily = FIRMWARE_VERSION_META.CH592F.protocolFamily;
  readonly protocolVersionMajor = FIRMWARE_VERSION_META.CH592F.protocolVersion.major;
  readonly protocolVersionMinor = FIRMWARE_VERSION_META.CH592F.protocolVersion.minor;
  readonly storageVersionMajor = FIRMWARE_VERSION_META.CH592F.storageVersion.major;
  readonly storageVersionMinor = FIRMWARE_VERSION_META.CH592F.storageVersion.minor;

  getOptionalOperations(transport: CodecTransport<DataView>): HidOptionalOperations {
    return {
      getRgbConfig: () => this.getRgbConfig(transport),
      setRgbConfig: (config) => this.setRgbConfig(transport, config),
      getFnKeyConfig: () => this.getFnKeyConfig(transport),
      setFnKeyConfig: (config) => this.setFnKeyConfig(transport, config),
      saveConfig: () => this.runOkCommand(transport, Command.CFG_SAVE, 'CFG_SAVE'),
      loadConfig: () => this.runOkCommand(transport, Command.CFG_LOAD, 'CFG_LOAD'),
      resetConfig: () => this.runOkCommand(transport, Command.CFG_RESET, 'CFG_RESET'),
      getBattery: () => this.getBattery(transport),
      getLogConfig: () => this.getLogConfig(transport),
      setLogConfig: (config) => this.setLogConfig(transport, config),
    };
  }

  buildCommandFrame(cmd: Command, sub = 0, data: Uint8Array = new Uint8Array(0)): Uint8Array {
    if (data.length > FRAME_SIZE - 3) {
      throw new Error(`命令数据过长: ${data.length} > ${FRAME_SIZE - 3}`);
    }

    const frame = new Uint8Array(FRAME_SIZE);
    frame[0] = cmd;
    frame[1] = sub;
    frame[2] = data.length;
    frame.set(data, 3);
    return frame;
  }

  describeOutgoingFrame(frame: Uint8Array): TerminalEntryDraft {
    const parsed = parseSendFrame(frame);
    return {
      direction: 'send',
      level: 'info',
      command: parsed.command,
      cmdHex: parsed.cmdHex,
      sub: parsed.sub,
      dataLen: parsed.dataLen,
      rawHex: parsed.rawHex,
      parsed: parsed.parsed,
    };
  }

  parseIncomingPacket(frame: Uint8Array): CodecInboundPacket<DataView> {
    if (frame[0] === Command.LOG) {
      const parsed = parseLogFrame(frame);
      return {
        kind: 'event',
        entry: {
          direction: 'device',
          level: 'info',
          command: parsed.command,
          cmdHex: parsed.cmdHex,
          sub: parsed.sub,
          dataLen: parsed.dataLen,
          rawHex: parsed.rawHex,
          parsed: parsed.parsed,
          category: parsed.category,
        },
      };
    }

    const parsed = parseReceiveFrame(frame);
    return {
      kind: 'response',
      entry: {
        direction: 'receive',
        level: parsed.isError ? 'error' : 'success',
        command: parsed.command,
        cmdHex: parsed.cmdHex,
        sub: parsed.sub,
        dataLen: parsed.dataLen,
        rawHex: parsed.rawHex,
        parsed: parsed.parsed,
        statusCode: parsed.statusCode,
      },
      response: new DataView(frame.buffer, frame.byteOffset, frame.byteLength),
    };
  }

  parseSysInfo(resp: DataView): DeviceInfo {
    const d = this.expectOk(resp, 'SYS_INFO');
    return {
      vendorId: (resp.getUint8(d + 1) << 8) | resp.getUint8(d + 2),
      productId: (resp.getUint8(d + 3) << 8) | resp.getUint8(d + 4),
      chipFamily: this.chipFamily,
      versionMajor: resp.getUint8(d + 5),
      versionMinor: resp.getUint8(d + 6),
      versionPatch: resp.getUint8(d + 7),
      protocolFamily: this.protocolFamily,
      protocolVersionMajor: resp.byteLength >= d + 15 ? resp.getUint8(d + 14) : this.protocolVersionMajor,
      protocolVersionMinor: resp.byteLength >= d + 16 ? resp.getUint8(d + 15) : this.protocolVersionMinor,
      storageVersionMajor: resp.byteLength >= d + 17 ? resp.getUint8(d + 16) : this.storageVersionMajor,
      storageVersionMinor: resp.byteLength >= d + 18 ? resp.getUint8(d + 17) : this.storageVersionMinor,
      maxLayers: resp.getUint8(d + 8),
      maxKeys: resp.getUint8(d + 9),
      macroSlots: resp.getUint8(d + 10),
      keyboardType: resp.getUint8(d + 11),
      actualKeyCount: resp.getUint8(d + 12),
      fnKeyCount: resp.getUint8(d + 13),
      protocol: this.protocol,
      protocolLabel: this.protocolLabel,
      capabilities: this.capabilities,
    };
  }

  parseSysStatus(resp: DataView): DeviceStatus {
    const d = this.expectOk(resp, 'SYS_STATUS');
    return {
      workMode: resp.getUint8(d + 1),
      connectionState: resp.getUint8(d + 2),
      currentLayer: resp.getUint8(d + 3),
      batteryLevel: resp.getUint8(d + 4),
      isCharging: resp.getUint8(d + 5) !== 0,
    };
  }

  parseKeymap(resp: DataView): { numLayers: number; currentLayer: number; defaultLayer: number; layer: LayerConfig } {
    const d = this.expectOk(resp, 'KEYMAP_GET');
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

  buildSetKeymapPayload(numLayers: number, defaultLayer: number, layer: LayerConfig): Uint8Array {
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

    return data;
  }

  parseRgbConfig(resp: DataView): RgbConfig {
    const d = this.expectOk(resp, 'RGB_GET');
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

  buildSetRgbPayload(config: RgbConfig): Uint8Array {
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
    return data;
  }

  parseFnKeyConfig(resp: DataView): FnKeyConfig {
    const d = this.expectOk(resp, 'FNKEY_GET');
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

  buildSetFnKeyPayload(config: FnKeyConfig): Uint8Array {
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
    return data;
  }

  parseBatteryInfo(resp: DataView): BatteryInfo {
    const d = this.expectOk(resp, 'BATTERY');
    return {
      level: resp.getUint8(d + 1),
      isCharging: resp.getUint8(d + 2) !== 0,
      voltage: resp.getUint16(d + 3, true) / 1000,
    };
  }

  parseLogConfig(resp: DataView): LogConfig {
    const d = this.expectOk(resp, 'LOG_GET');
    return {
      enabled: resp.getUint8(d + 1) !== 0,
    };
  }

  buildSetLogConfigPayload(config: LogConfig): Uint8Array {
    return new Uint8Array([config.enabled ? 1 : 0]);
  }

  expectOk(resp: DataView, commandName: string): number {
    const status = resp.getUint8(RESP_HEADER_SIZE);
    if (status !== ResponseCode.OK) {
      throw new Error(`${commandName} 失败: 0x${status.toString(16)}`);
    }
    return RESP_HEADER_SIZE;
  }

  buildFullKeymap(firstLayer: { numLayers: number; currentLayer: number; defaultLayer: number; layer: LayerConfig }, layers: LayerConfig[]): KeymapConfig {
    return {
      numLayers: firstLayer.numLayers,
      currentLayer: firstLayer.currentLayer,
      defaultLayer: firstLayer.defaultLayer,
      layers,
    };
  }

  async getSysInfo(transport: CodecTransport<DataView>): Promise<DeviceInfo> {
    const resp = await this.sendCommand(transport, Command.SYS_INFO);
    return this.parseSysInfo(resp);
  }

  async getSysStatus(transport: CodecTransport<DataView>): Promise<DeviceStatus> {
    const resp = await this.sendCommand(transport, Command.SYS_STATUS);
    return this.parseSysStatus(resp);
  }

  async getFullKeymap(transport: CodecTransport<DataView>): Promise<KeymapConfig> {
    const first = await this.getKeymap(transport, 0);
    const layers = createEmptyKeymap().layers;
    layers[0] = first.layer;

    for (let i = 1; i < first.numLayers; i++) {
      const result = await this.getKeymap(transport, i);
      layers[i] = result.layer;
    }

    return this.buildFullKeymap(first, layers);
  }

  async setFullKeymap(transport: CodecTransport<DataView>, config: KeymapConfig): Promise<void> {
    for (let i = 0; i < config.numLayers; i++) {
      const data = this.buildSetKeymapPayload(config.numLayers, config.defaultLayer, config.layers[i]);
      const resp = await this.sendCommand(transport, Command.KEYMAP_SET, i, data);
      this.expectOk(resp, 'KEYMAP_SET');
    }
  }

  private async sendCommand(
    transport: CodecTransport<DataView>,
    cmd: Command,
    sub = 0,
    data: Uint8Array = new Uint8Array(0),
    timeout = 3000,
  ): Promise<DataView> {
    return transport.sendAndWait(this.buildCommandFrame(cmd, sub, data), {
      timeout,
      timeoutLabel: '命令响应超时',
    });
  }

  private async getKeymap(
    transport: CodecTransport<DataView>,
    layerIndex: number,
  ): Promise<{ numLayers: number; currentLayer: number; defaultLayer: number; layer: KeymapConfig['layers'][number] }> {
    const resp = await this.sendCommand(transport, Command.KEYMAP_GET, layerIndex);
    return this.parseKeymap(resp);
  }

  private async getRgbConfig(transport: CodecTransport<DataView>): Promise<RgbConfig> {
    const resp = await this.sendCommand(transport, Command.RGB_GET);
    return this.parseRgbConfig(resp);
  }

  private async setRgbConfig(transport: CodecTransport<DataView>, config: RgbConfig): Promise<void> {
    const resp = await this.sendCommand(transport, Command.RGB_SET, 0, this.buildSetRgbPayload(config));
    this.expectOk(resp, 'RGB_SET');
  }

  private async getFnKeyConfig(transport: CodecTransport<DataView>): Promise<FnKeyConfig> {
    const resp = await this.sendCommand(transport, Command.FNKEY_GET);
    return this.parseFnKeyConfig(resp);
  }

  private async setFnKeyConfig(transport: CodecTransport<DataView>, config: FnKeyConfig): Promise<void> {
    const resp = await this.sendCommand(transport, Command.FNKEY_SET, 0, this.buildSetFnKeyPayload(config));
    this.expectOk(resp, 'FNKEY_SET');
  }

  private async getBattery(transport: CodecTransport<DataView>): Promise<BatteryInfo> {
    const resp = await this.sendCommand(transport, Command.BATTERY);
    return this.parseBatteryInfo(resp);
  }

  private async getLogConfig(transport: CodecTransport<DataView>): Promise<LogConfig> {
    const resp = await this.sendCommand(transport, Command.LOG_GET);
    return this.parseLogConfig(resp);
  }

  private async setLogConfig(transport: CodecTransport<DataView>, config: LogConfig): Promise<void> {
    const resp = await this.sendCommand(transport, Command.LOG_SET, 0, this.buildSetLogConfigPayload(config));
    this.expectOk(resp, 'LOG_SET');
  }

  private async runOkCommand(
    transport: CodecTransport<DataView>,
    cmd: Command,
    commandName: string,
  ): Promise<void> {
    const resp = await this.sendCommand(transport, cmd);
    this.expectOk(resp, commandName);
  }
}
