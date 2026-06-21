import { beforeEach, describe, expect, it, vi } from "vitest";
import { createPinia, setActivePinia } from "pinia";
import { useDeviceStore } from "@/stores/deviceStore";
import { useStudioAssetStore } from "@/stores/studioAssetStore";
import {
  createDeviceCapabilities,
  createEmptyKeymap,
  createDefaultRgbConfig,
  DeviceProtocol,
  KeyboardType,
  OsMode,
  type DeviceInfo,
} from "@/types/protocol";
import {
  ASSET_FORMAT_VERSION,
  CONFIG_PROFILE_FORMAT,
  type ConfigProfileDocument,
} from "@/types/studioAssets";

const memoryProfiles = new Map<string, ConfigProfileDocument>();

vi.mock("@/services/studioAssetDb", () => ({
  listConfigProfiles: vi.fn(async () => Array.from(memoryProfiles.values())),
  putConfigProfile: vi.fn(async (profile: ConfigProfileDocument) => {
    memoryProfiles.set(profile.id, profile);
  }),
  deleteConfigProfile: vi.fn(async (id: string) => {
    memoryProfiles.delete(id);
  }),
}));

function fakeDeviceInfo(protocol: DeviceProtocol, keyboardType: KeyboardType): DeviceInfo {
  const isCh592 = protocol === DeviceProtocol.CH592;
  const keyboardInfo = {
    [KeyboardType.BASIC]: { keys: 4, layers: 4 },
    [KeyboardType.FIVE_KEYS]: { keys: 5, layers: 5 },
    [KeyboardType.KNOB]: { keys: 7, layers: 4 },
  }[keyboardType];

  return {
    vendorId: isCh592 ? 0x413d : 0x1209,
    productId: isCh592 ? 0x2107 : 0xc55d,
    chipFamily: isCh592 ? "CH592F" : "CH552G",
    versionMajor: 1,
    versionMinor: 2,
    versionPatch: 3,
    maxLayers: keyboardInfo.layers,
    maxKeys: 8,
    macroSlots: 8,
    keyboardType,
    actualKeyCount: keyboardInfo.keys,
    fnKeyCount: isCh592 ? 1 : 0,
    protocol,
    protocolLabel: isCh592 ? "CH592F HID" : "CH552G USB",
    capabilities: createDeviceCapabilities({
      fnKeys: isCh592,
      osMode: true,
      layerKeyActions: isCh592,
      battery: isCh592,
      logs: isCh592,
      wireless: isCh592,
      explicitSave: isCh592,
      iap: isCh592,
    }),
  };
}

function setFakeDevice(protocol: DeviceProtocol, keyboardType: KeyboardType): void {
  const deviceStore = useDeviceStore();
  const keymap = createEmptyKeymap();
  const info = fakeDeviceInfo(protocol, keyboardType);
  keymap.numLayers = info.maxLayers;
  keymap.currentLayer = 0;
  keymap.defaultLayer = 0;

  deviceStore.deviceInfo = info;
  deviceStore.deviceStatus = {
    workMode: 0,
    connectionState: 2,
    currentLayer: 0,
    batteryLevel: 100,
    isCharging: false,
  };
  deviceStore.keymap = keymap;
  deviceStore.keymapOriginal = JSON.parse(JSON.stringify(keymap));
  deviceStore.rgbConfig = createDefaultRgbConfig();
  deviceStore.osModeConfig = { mode: OsMode.MAC };
}

function fakeProfile(protocol: DeviceProtocol, keyboardType: KeyboardType): ConfigProfileDocument {
  const source = fakeDeviceInfo(protocol, keyboardType);
  const keymap = createEmptyKeymap();
  keymap.numLayers = source.maxLayers;
  keymap.layers[0].keys[0] = { type: 5, modifier: 0, param1: 2, param2: 0 };

  return {
    format: CONFIG_PROFILE_FORMAT,
    version: ASSET_FORMAT_VERSION,
    id: "fake-profile",
    name: "假数据配置",
    createdAt: "2026-06-21T00:00:00.000Z",
    updatedAt: "2026-06-21T00:00:00.000Z",
    sourceDevice: {
      protocol,
      protocolLabel: source.protocolLabel,
      keyboardType,
      keyboardTypeName: keyboardType === KeyboardType.FIVE_KEYS ? "五键款" : "基础款",
      actualKeyCount: source.actualKeyCount,
      maxLayers: source.maxLayers,
      macroSlots: source.macroSlots,
      fnKeyCount: source.fnKeyCount,
      firmwareVersion: "1.2.3",
      capabilities: source.capabilities,
    },
    config: {
      keymap,
      rgb: createDefaultRgbConfig(),
      osMode: { mode: OsMode.MAC },
    },
    macroRefs: [{ layer: 0, key: 0, slot: 2 }],
  };
}

describe("studio asset store fake data", () => {
  beforeEach(() => {
    memoryProfiles.clear();
    setActivePinia(createPinia());
  });

  it("saves current device config into an IndexedDB-backed fake profile list", async () => {
    setFakeDevice(DeviceProtocol.CH552, KeyboardType.FIVE_KEYS);
    const assetStore = useStudioAssetStore();

    const profile = await assetStore.saveCurrentProfile("假数据快照");

    expect(profile.name).toBe("假数据快照");
    expect(profile.sourceDevice.protocol).toBe(DeviceProtocol.CH552);
    expect(assetStore.profileCount).toBe(1);
    expect(JSON.parse(assetStore.exportProfile(profile.id)).format).toBe(CONFIG_PROFILE_FORMAT);
  });

  it("accepts CH552 profile plans for CH592 devices and warns about macro references", () => {
    setFakeDevice(DeviceProtocol.CH592, KeyboardType.FIVE_KEYS);
    const assetStore = useStudioAssetStore();
    const profile = fakeProfile(DeviceProtocol.CH552, KeyboardType.FIVE_KEYS);

    const plan = assetStore.buildApplyPlan(profile);

    expect(plan.some((item) => item.severity === "error")).toBe(false);
    expect(plan).toEqual(
      expect.arrayContaining([
        expect.objectContaining({ section: "协议", severity: "ok" }),
        expect.objectContaining({ section: "宏引用", severity: "warn" }),
      ]),
    );
  });

  it("rejects CH592 profile plans for CH552 devices", () => {
    setFakeDevice(DeviceProtocol.CH552, KeyboardType.FIVE_KEYS);
    const assetStore = useStudioAssetStore();
    const profile = fakeProfile(DeviceProtocol.CH592, KeyboardType.FIVE_KEYS);

    const plan = assetStore.buildApplyPlan(profile);

    expect(plan).toEqual([
      expect.objectContaining({
        section: "协议",
        severity: "error",
        message: "无线款配置不能导入到有线款",
      }),
    ]);
  });

  it("imports fake profile JSON into the local profile library", async () => {
    setFakeDevice(DeviceProtocol.CH592, KeyboardType.FIVE_KEYS);
    const assetStore = useStudioAssetStore();
    const profile = fakeProfile(DeviceProtocol.CH552, KeyboardType.FIVE_KEYS);

    const count = await assetStore.importProfileJson(JSON.stringify(profile));

    expect(count).toBe(1);
    expect(assetStore.profiles).toHaveLength(1);
    expect(assetStore.profiles[0].sourceDevice.protocol).toBe(DeviceProtocol.CH552);
  });
});
