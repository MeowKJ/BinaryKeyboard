import { beforeEach, describe, expect, it, vi } from "vitest";
import { createPinia, setActivePinia } from "pinia";
import { useStudioAssetStore } from "@/stores/studioAssetStore";
import {
  createDeviceCapabilities,
  DeviceProtocol,
  KeyboardType,
  MacroActionType,
  type DeviceInfo,
  type MacroData,
} from "@/types/protocol";
import { MACRO_PACK_FORMAT } from "@/types/studioAssets";

const mocks = vi.hoisted(() => {
  const deviceStore = {
    deviceInfo: null as DeviceInfo | null,
    keyboardTypeName: "五键款",
    firmwareVersion: "1.2.3",
  };
  const macroStore = {
    slotValid: [true, false, true],
    usedCount: 1,
    maxNameBytes: 16,
    refreshOverview: vi.fn(async () => {}),
    loadMacro: vi.fn(),
    invalidateCache: vi.fn(),
  };
  const hidService = {
    setMacroData: vi.fn(async () => {}),
  };
  return { deviceStore, macroStore, hidService };
});

vi.mock("@/stores/deviceStore", () => ({
  useDeviceStore: () => mocks.deviceStore,
}));

vi.mock("@/stores/macroStore", () => ({
  useMacroStore: () => mocks.macroStore,
}));

vi.mock("@/services/HidService", () => ({
  hidService: mocks.hidService,
}));

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
    capabilities: createDeviceCapabilities({ macroActions: true, fnKeys: true }),
  };
}

function macroData(slot: number, name: string): MacroData {
  return {
    header: {
      valid: 1,
      id: slot,
      actionCount: 1,
      dataSize: 2,
      name,
    },
    actions: [
      { type: MacroActionType.KEY_DOWN, param: 4 },
      { type: MacroActionType.END, param: 0 },
    ],
  };
}

describe("studio asset store macro packs", () => {
  beforeEach(() => {
    setActivePinia(createPinia());
    mocks.deviceStore.deviceInfo = fakeDeviceInfo();
    mocks.deviceStore.keyboardTypeName = "五键款";
    mocks.deviceStore.firmwareVersion = "1.2.3";
    mocks.macroStore.slotValid = [true, false, true];
    mocks.macroStore.usedCount = 1;
    mocks.macroStore.maxNameBytes = 16;
    mocks.macroStore.refreshOverview.mockClear();
    mocks.macroStore.invalidateCache.mockClear();
    mocks.macroStore.loadMacro.mockReset();
    mocks.macroStore.loadMacro.mockImplementation(async (slot: number) => macroData(slot, `宏 ${slot + 1}`));
    mocks.hidService.setMacroData.mockClear();
  });

  it("exports only valid macro slots", async () => {
    const assetStore = useStudioAssetStore();

    const pack = JSON.parse(await assetStore.exportMacroPack()) as { format: string; macros: Array<{ sourceSlot: number }> };

    expect(pack.format).toBe(MACRO_PACK_FORMAT);
    expect(pack.macros.map((macro) => macro.sourceSlot)).toEqual([0, 2]);
    expect(mocks.macroStore.loadMacro).toHaveBeenCalledTimes(2);
  });

  it("imports macro packs after the current used count when appending", async () => {
    const assetStore = useStudioAssetStore();
    const pack = {
      format: MACRO_PACK_FORMAT,
      version: 1,
      name: "导入宏包",
      createdAt: "2026-06-22T00:00:00.000Z",
      sourceDevice: {},
      macros: [
        {
          name: "测试宏",
          sourceSlot: 0,
          actions: [{ type: MacroActionType.KEY_DOWN, param: 4 }],
        },
      ],
    };

    const count = await assetStore.importMacroPackJson(JSON.stringify(pack), { strategy: "append" });

    expect(count).toBe(1);
    expect(mocks.hidService.setMacroData).toHaveBeenCalledWith(
      1,
      expect.objectContaining({
        header: expect.objectContaining({ id: 1, name: "测试宏", actionCount: 1 }),
        actions: expect.arrayContaining([{ type: MacroActionType.END, param: 0 }]),
      }),
    );
    expect(mocks.macroStore.invalidateCache).toHaveBeenCalledOnce();
  });

  it("imports only the first macro when single import is requested", async () => {
    const assetStore = useStudioAssetStore();
    const pack = {
      format: MACRO_PACK_FORMAT,
      version: 1,
      name: "单宏导入",
      createdAt: "2026-06-22T00:00:00.000Z",
      sourceDevice: {},
      macros: [
        {
          name: "第一个",
          sourceSlot: 0,
          actions: [{ type: MacroActionType.KEY_DOWN, param: 4 }],
        },
        {
          name: "第二个",
          sourceSlot: 1,
          actions: [{ type: MacroActionType.KEY_DOWN, param: 5 }],
        },
      ],
    };

    const count = await assetStore.importMacroPackJson(JSON.stringify(pack), {
      strategy: "overwrite",
      targetSlot: 2,
      single: true,
    });

    expect(count).toBe(1);
    expect(mocks.hidService.setMacroData).toHaveBeenCalledOnce();
    expect(mocks.hidService.setMacroData).toHaveBeenCalledWith(
      2,
      expect.objectContaining({
        header: expect.objectContaining({ id: 2, name: "第一个" }),
      }),
    );
  });

  it("rejects macro packs with invalid actions", async () => {
    const assetStore = useStudioAssetStore();
    const pack = {
      format: MACRO_PACK_FORMAT,
      version: 1,
      name: "坏宏包",
      createdAt: "2026-06-22T00:00:00.000Z",
      sourceDevice: {},
      macros: [
        {
          name: "坏宏",
          sourceSlot: 0,
          actions: [{ type: 999, param: 4 }],
        },
      ],
    };

    await expect(assetStore.importMacroPackJson(JSON.stringify(pack), { strategy: "append" })).rejects.toThrow(/动作 1/);
    expect(mocks.hidService.setMacroData).not.toHaveBeenCalled();
  });
});
