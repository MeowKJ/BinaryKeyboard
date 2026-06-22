import type {
  DeviceCapabilities,
  DeviceProtocol,
  KeyboardType,
  MacroAction,
} from "@/types/protocol";

export const MACRO_PACK_FORMAT = "meowkeyboard-macro-pack";
export const ASSET_FORMAT_VERSION = 1;

export interface AssetSourceDevice {
  protocol: DeviceProtocol;
  protocolLabel: string;
  keyboardType: KeyboardType;
  keyboardTypeName: string;
  actualKeyCount: number;
  maxLayers: number;
  macroSlots: number;
  fnKeyCount: number;
  firmwareVersion: string;
  capabilities: Partial<DeviceCapabilities>;
}

export interface MacroAsset {
  name: string;
  sourceSlot: number;
  actions: MacroAction[];
}

export interface MacroPackDocument {
  format: typeof MACRO_PACK_FORMAT;
  version: typeof ASSET_FORMAT_VERSION;
  name: string;
  createdAt: string;
  sourceDevice: AssetSourceDevice;
  macros: MacroAsset[];
}

export interface ApplyProgress {
  active: boolean;
  title: string;
  current: number;
  total: number;
  detail: string;
}

export interface MacroImportOptions {
  strategy: "append" | "original" | "overwrite";
  targetSlot?: number;
  single?: boolean;
}

export function cloneAsset<T>(value: T): T {
  return JSON.parse(JSON.stringify(value)) as T;
}
