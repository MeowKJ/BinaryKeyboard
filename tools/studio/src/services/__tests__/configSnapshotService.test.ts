import { beforeEach, describe, expect, it, vi } from "vitest";
import { createPinia, setActivePinia } from "pinia";
import {
  applyConfigBackupToDevice,
  backupToDownloadName,
  checkConfigCompatibility,
  finalizeImportedBackup,
  parseConfigBackupFile,
  verifyConfigBackupIntegrity,
} from "@/services/configSnapshotService";
import { useDeviceStore } from "@/stores/deviceStore";
import type { ConfigBackupFile } from "@/types/configBackup";
import {
  ActionType,
  createDefaultRgbConfig,
  createDeviceCapabilities,
  createEmptyFnKeyConfig,
  createEmptyKeymap,
  DeviceProtocol,
  FnAction,
  KeyboardType,
  MacroActionType,
  OsMode,
  type DeviceInfo,
  type MacroData,
} from "@/types/protocol";

const hidMocks = vi.hoisted(() => ({
  getSysInfo: vi.fn(),
  getFullKeymap: vi.fn(),
  getRgbConfig: vi.fn(),
  getFnKeyConfig: vi.fn(),
  getOsMode: vi.fn(),
  getLogConfig: vi.fn(),
  getMacroOverview: vi.fn(),
  getMacroData: vi.fn(),
  setFullKeymap: vi.fn(),
  setRgbConfig: vi.fn(),
  setFnKeyConfig: vi.fn(),
  setOsMode: vi.fn(),
  setLogConfig: vi.fn(),
  deleteMacro: vi.fn(),
  setMacroData: vi.fn(),
  saveConfig: vi.fn(),
}));

vi.mock("@/services/HidService", () => ({
  hidService: hidMocks,
}));

function cloneJson<T>(value: T): T {
  return JSON.parse(JSON.stringify(value)) as T;
}

function fakeDeviceInfo(): DeviceInfo {
  return {
    vendorId: 0x413d,
    productId: 0x2107,
    chipFamily: "CH592F",
    versionMajor: 1,
    versionMinor: 2,
    versionPatch: 3,
    maxLayers: 5,
    maxKeys: 8,
    macroSlots: 8,
    keyboardType: KeyboardType.FIVE_KEYS,
    actualKeyCount: 5,
    fnKeyCount: 1,
    protocol: DeviceProtocol.CH592,
    protocolLabel: "CH592F HID",
    capabilities: createDeviceCapabilities(),
  };
}

function setFakeDevice(): DeviceInfo {
  const info = fakeDeviceInfo();
  const deviceStore = useDeviceStore();
  const keymap = createEmptyKeymap();
  keymap.numLayers = info.maxLayers;
  deviceStore.deviceInfo = info;
  deviceStore.deviceStatus = {
    workMode: 0,
    connectionState: 2,
    currentLayer: 0,
    batteryLevel: 100,
    isCharging: false,
  };
  deviceStore.keymap = keymap;
  deviceStore.rgbConfig = createDefaultRgbConfig();
  deviceStore.fnKeyConfig = createEmptyFnKeyConfig();
  deviceStore.osModeConfig = { mode: OsMode.MAC };
  return info;
}

function macroData(slot: number): MacroData {
  return {
    header: {
      valid: 1,
      id: slot,
      actionCount: 1,
      dataSize: 2,
      name: `macro-${slot}`,
    },
    actions: [
      { type: MacroActionType.KEY_DOWN, param: 4 },
      { type: MacroActionType.END, param: 0 },
    ],
  };
}

async function validBackup(): Promise<ConfigBackupFile> {
  const info = fakeDeviceInfo();
  const keymap = createEmptyKeymap();
  keymap.numLayers = info.maxLayers;
  const backup: ConfigBackupFile = {
    schema: "binarykeyboard.config.backup",
    schemaVersion: 1,
    createdAt: "2026-06-22T00:00:00.000Z",
    name: "test backup",
    source: "device",
    device: {
      protocol: info.protocol,
      protocolLabel: info.protocolLabel,
      chipFamily: info.chipFamily,
      keyboardType: info.keyboardType,
      firmwareVersion: "1.2.3",
      maxLayers: info.maxLayers,
      maxKeys: info.maxKeys,
      actualKeyCount: info.actualKeyCount,
      fnKeyCount: info.fnKeyCount,
      macroSlots: info.macroSlots,
    },
    config: {
      keymap,
      rgb: createDefaultRgbConfig(),
      fnKeys: createEmptyFnKeyConfig(),
      osMode: { mode: OsMode.MAC },
      log: { enabled: true },
      macros: [],
    },
    checksum: {
      algorithm: "sha256",
      value: "",
    },
  };
  return finalizeImportedBackup(backup, "device");
}

describe("config snapshot service", () => {
  beforeEach(() => {
    setActivePinia(createPinia());
    vi.clearAllMocks();
    const info = setFakeDevice();
    hidMocks.getSysInfo.mockResolvedValue(info);
    hidMocks.getMacroOverview.mockResolvedValue({
      totalSlots: 8,
      usedCount: 0,
      slotValid: Array.from({ length: 8 }, () => false),
      dynamic: true,
      maxSlots: 8,
      fsTotal: 1024,
      fsFree: 1024,
    });
  });

  it("rejects a tampered backup checksum", async () => {
    const backup = await validBackup();
    const tampered = { ...backup, name: "tampered" };

    await expect(parseConfigBackupFile(JSON.stringify(tampered))).rejects.toThrow();
  });

  it("keeps the config name in exported file names", async () => {
    const backup = await validBackup();

    expect(backupToDownloadName({ ...backup, name: "我的配置:危险/测试" })).toBe("我的配置-危险-测试.binarykeyboard-config.json");
  });

  it("rejects structurally invalid backup fields even when checksum is refreshed", async () => {
    const backup = await validBackup();
    const invalid = cloneJson(backup);
    invalid.config.rgb!.brightness = 300;
    const finalized = await finalizeImportedBackup(invalid, "file");

    await expect(parseConfigBackupFile(JSON.stringify(finalized))).rejects.toThrow();
  });

  it("rechecks backup schema before restoring a local slot", async () => {
    const backup = await validBackup();
    const invalid = {
      ...backup,
      schema: "binarykeyboard.config.unknown",
    } as unknown as ConfigBackupFile;
    invalid.checksum = (await finalizeImportedBackup(invalid, "device")).checksum;

    await expect(verifyConfigBackupIntegrity(invalid)).rejects.toThrow();
  });

  it("rejects non-array macro payloads with a structured validation error", async () => {
    const backup = await validBackup();
    const invalid = cloneJson(backup);
    (invalid.config as unknown as Record<string, unknown>).macros = { slot: 0, data: macroData(0) };
    const finalized = await finalizeImportedBackup(invalid as unknown as ConfigBackupFile, "file");

    await expect(parseConfigBackupFile(JSON.stringify(finalized))).rejects.toThrow();
  });

  it("blocks FN macro references when backup macro data is missing", async () => {
    const backup = await validBackup();
    const withFnMacro = cloneJson(backup);
    withFnMacro.config.fnKeys!.fnKeys[0].clickAction = FnAction.MACRO;
    withFnMacro.config.fnKeys!.fnKeys[0].clickParam = 3;
    withFnMacro.config.macros = [];
    const finalized = await finalizeImportedBackup(withFnMacro, "device");

    const result = checkConfigCompatibility(finalized);

    expect(result.ok).toBe(false);
    expect(result.blocking.length).toBeGreaterThan(0);
  });

  it("blocks sparse macro slots before writing device config", async () => {
    const backup = await validBackup();
    const sparse = cloneJson(backup);
    sparse.config.keymap.layers[0].keys[0] = {
      type: ActionType.MACRO,
      modifier: 0,
      param1: 2,
      param2: 0,
    };
    sparse.config.macros = [{ slot: 2, data: macroData(2) }];
    const finalized = await finalizeImportedBackup(sparse, "device");

    await expect(applyConfigBackupToDevice(finalized)).rejects.toThrow();
    expect(hidMocks.setFullKeymap).not.toHaveBeenCalled();
  });
});
