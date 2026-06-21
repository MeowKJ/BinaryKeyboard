import type {
  DeviceCapabilities,
  DeviceProtocol,
  FnKeyConfig,
  KeyboardType,
  KeyAction,
  KeymapConfig,
  MacroAction,
  MacroData,
  OsModeConfig,
  RgbConfig,
} from "@/types/protocol";

export const CONFIG_PROFILE_FORMAT = "meowkeyboard-config-profile";
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

export interface MacroReference {
  layer: number;
  key: number;
  slot: number;
}

export interface ConfigProfileDocument {
  format: typeof CONFIG_PROFILE_FORMAT;
  version: typeof ASSET_FORMAT_VERSION;
  id: string;
  name: string;
  createdAt: string;
  updatedAt: string;
  sourceDevice: AssetSourceDevice;
  config: {
    keymap: KeymapConfig;
    rgb?: RgbConfig;
    fn?: FnKeyConfig;
    osMode?: OsModeConfig;
  };
  macroRefs: MacroReference[];
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

export type ApplySeverity = "ok" | "warn" | "skip" | "error";

export interface ApplyPlanItem {
  section: string;
  severity: ApplySeverity;
  message: string;
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
}

export function cloneAsset<T>(value: T): T {
  return JSON.parse(JSON.stringify(value)) as T;
}
