import type {
  DeviceProtocol,
  FnKeyConfig,
  KeymapConfig,
  LogConfig,
  MacroData,
  OsModeConfig,
  RgbConfig,
} from '@/types/protocol';

export const CONFIG_BACKUP_SCHEMA = 'binarykeyboard.config.backup';
export const CONFIG_BACKUP_SCHEMA_VERSION = 1;

export interface ConfigBackupDeviceInfo {
  protocol: DeviceProtocol;
  protocolLabel: string;
  chipFamily: string;
  keyboardType: number;
  firmwareVersion: string;
  maxLayers: number;
  maxKeys: number;
  actualKeyCount: number;
  fnKeyCount: number;
  macroSlots: number;
}

export interface ConfigBackupPayload {
  keymap: KeymapConfig;
  rgb?: RgbConfig;
  fnKeys?: FnKeyConfig;
  osMode?: OsModeConfig;
  log?: LogConfig;
  macros?: ConfigBackupMacroEntry[];
}

export interface ConfigBackupMacroEntry {
  slot: number;
  data: MacroData;
}

export interface ConfigBackupFile {
  schema: typeof CONFIG_BACKUP_SCHEMA;
  schemaVersion: typeof CONFIG_BACKUP_SCHEMA_VERSION;
  createdAt: string;
  name: string;
  source: 'device' | 'file' | 'manual';
  device: ConfigBackupDeviceInfo;
  config: ConfigBackupPayload;
  checksum: {
    algorithm: 'sha256';
    value: string;
  };
}

export interface ConfigLibraryItem {
  id: string;
  index: number;
  name: string;
  createdAt: string;
  updatedAt: string;
  source: 'device' | 'file' | 'manual';
  backup: ConfigBackupFile;
}

export interface ConfigCompatibilityResult {
  ok: boolean;
  blocking: string[];
  warnings: string[];
}
