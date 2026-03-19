import { useTerminalStore } from '@/stores/terminalStore';
import type { DeviceProtocol } from '@/types/protocol';
import type { DeviceCodec, CodecCommandOptions, CodecTransport } from './codecTypes';
import type { HidAdapter, HidOptionalOperations } from './types';

export abstract class BaseHidAdapter<TResponse> implements HidAdapter {
  readonly protocol: DeviceProtocol;
  readonly filters: HIDDeviceFilter[];
  readonly optional: HidOptionalOperations;

  protected readonly codec: DeviceCodec<TResponse>;
  protected device: HIDDevice | null = null;
  private responsePromise: { resolve: (data: TResponse) => void; reject: (err: Error) => void } | null = null;
  private responseTimeout: number | null = null;
  private readonly transport: CodecTransport<TResponse>;
  private readonly inputReportHandler = this.handleInputReport.bind(this);

  /** 命令串行队列：保证同一时刻只有一个 sendAndWait 在等待响应 */
  private sendQueue: Promise<unknown> = Promise.resolve();

  protected constructor(codec: DeviceCodec<TResponse>, filters: HIDDeviceFilter[]) {
    this.codec = codec;
    this.protocol = codec.protocol;
    this.filters = filters;
    this.transport = {
      sendAndWait: (frame, options) => this.sendAndWait(frame, options),
      sendNoWait: (frame, options) => this.sendNoWait(frame, options),
    };
    this.optional = codec.getOptionalOperations(this.transport);
  }

  abstract matches(device: HIDDevice): boolean;
  protected abstract get commandReportId(): number;
  protected abstract get responseReportId(): number;
  protected abstract mapResponseData(event: HIDInputReportEvent): TResponse;
  protected abstract responseFrameBytes(event: HIDInputReportEvent): Uint8Array;

  async connect(device: HIDDevice): Promise<boolean> {
    try {
      if (this.device && this.device !== device) {
        this.device.removeEventListener('inputreport', this.inputReportHandler);
      }
      this.codec.resetState?.();
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
    this.codec.resetState?.();
    this.clearPendingResponse();
  }

  getDevice(): HIDDevice | null {
    return this.device;
  }

  isConnected(): boolean {
    return this.device !== null && this.device.opened;
  }

  async getSysInfo() {
    return this.codec.getSysInfo(this.transport);
  }

  async getSysStatus() {
    return this.codec.getSysStatus(this.transport);
  }

  async getFullKeymap() {
    return this.codec.getFullKeymap(this.transport);
  }

  async setFullKeymap(config: Parameters<DeviceCodec<TResponse>['setFullKeymap']>[1]) {
    await this.codec.setFullKeymap(this.transport, config);
  }

  protected addTerminalEntry(entry: Parameters<ReturnType<typeof useTerminalStore>['addEntry']>[0]): void {
    try {
      const terminalStore = useTerminalStore();
      terminalStore.addEntry(entry);
    } catch {
      // terminal may not be initialized
    }
  }

  protected sendAndWait(frame: Uint8Array, options: CodecCommandOptions = {}): Promise<TResponse> {
    // 所有命令通过串行队列发送，避免并发覆盖 responsePromise
    const task = this.sendQueue.then(() => this.sendAndWaitInternal(frame, options));
    // 无论成功失败都推进队列，不让错误阻塞后续命令
    this.sendQueue = task.catch(() => {});
    return task;
  }

  private async sendAndWaitInternal(frame: Uint8Array, options: CodecCommandOptions): Promise<TResponse> {
    if (!this.device || !this.device.opened) {
      throw new Error('设备未连接');
    }

    const timeout = options.timeout ?? 3000;
    const timeoutLabel = options.timeoutLabel ?? '命令响应超时';
    const responsePromise = new Promise<TResponse>((resolve, reject) => {
      this.responsePromise = { resolve, reject };
      this.responseTimeout = window.setTimeout(() => {
        this.responseTimeout = null;
        this.responsePromise = null;
        reject(new Error(timeoutLabel));
      }, timeout);
    });

    this.addTerminalEntry(this.codec.describeOutgoingFrame(frame));

    try {
      await this.device.sendReport(this.commandReportId, frame);
    } catch (error) {
      this.clearPendingResponse();
      throw error instanceof Error ? error : new Error('发送 HID 报告失败');
    }

    return responsePromise;
  }

  protected async sendNoWait(frame: Uint8Array, _options?: CodecCommandOptions): Promise<void> {
    if (!this.device || !this.device.opened) {
      throw new Error('设备未连接');
    }

    this.addTerminalEntry(this.codec.describeOutgoingFrame(frame));
    await this.device.sendReport(this.commandReportId, frame);
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

  private handleInputReport(event: HIDInputReportEvent): void {
    if (this.responseReportId !== 0 && event.reportId !== this.responseReportId) return;

    const frame = this.responseFrameBytes(event);
    const packet = this.codec.parseIncomingPacket(frame);
    this.addTerminalEntry(packet.entry);

    if (packet.kind !== 'response' || !this.responsePromise) {
      return;
    }

    const promise = this.responsePromise;
    this.clearPendingResponse();
    promise.resolve(packet.response ?? this.mapResponseData(event));
  }
}
