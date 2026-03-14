import type { TerminalEntry } from '@/stores/terminalStore';
import type {
  DeviceInfo,
  DeviceProtocol,
  DeviceStatus,
  KeymapConfig,
} from '@/types/protocol';
import type { HidOptionalOperations } from './types';

export type TerminalEntryDraft =
  Omit<TerminalEntry, 'id' | 'timestamp' | 'glowColor'> & { glowColor?: string };

export interface CodecCommandOptions {
  timeout?: number;
  timeoutLabel?: string;
}

export interface CodecTransport<TResponse> {
  sendAndWait(frame: Uint8Array, options?: CodecCommandOptions): Promise<TResponse>;
  sendNoWait(frame: Uint8Array, options?: CodecCommandOptions): Promise<void>;
}

export interface CodecInboundPacket<TResponse> {
  kind: 'response' | 'event';
  entry: TerminalEntryDraft;
  response?: TResponse;
}

export interface DeviceCodec<TResponse> {
  readonly protocol: DeviceProtocol;
  readonly protocolLabel: string;

  resetState?(): void;
  getOptionalOperations(transport: CodecTransport<TResponse>): HidOptionalOperations;
  getSysInfo(transport: CodecTransport<TResponse>): Promise<DeviceInfo>;
  getSysStatus(transport: CodecTransport<TResponse>): Promise<DeviceStatus>;
  getFullKeymap(transport: CodecTransport<TResponse>): Promise<KeymapConfig>;
  setFullKeymap(transport: CodecTransport<TResponse>, config: KeymapConfig): Promise<void>;
  describeOutgoingFrame(frame: Uint8Array): TerminalEntryDraft;
  parseIncomingPacket(frame: Uint8Array): CodecInboundPacket<TResponse>;
}
