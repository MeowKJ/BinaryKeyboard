import { ref } from "vue";
import { defineStore } from "pinia";
import { hidService } from "@/services/HidService";
import { useDeviceStore } from "@/stores/deviceStore";
import { useMacroStore } from "@/stores/macroStore";
import {
  MacroActionType,
  MACRO_NAME_MAX_BYTES,
  type MacroData,
} from "@/types/protocol";
import {
  ASSET_FORMAT_VERSION,
  MACRO_PACK_FORMAT,
  cloneAsset,
  type ApplyProgress,
  type AssetSourceDevice,
  type MacroAsset,
  type MacroImportOptions,
  type MacroPackDocument,
} from "@/types/studioAssets";
import { truncateUtf8ByBytes } from "@/utils/utf8";

const VALID_MACRO_ACTION_TYPES = new Set(
  Object.values(MacroActionType).filter((value): value is number => typeof value === "number"),
);

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === "object" && value !== null;
}

function isByte(value: unknown): value is number {
  return typeof value === "number" && Number.isInteger(value) && value >= 0 && value <= 0xff;
}

function sourceFromCurrentDevice(): AssetSourceDevice {
  const deviceStore = useDeviceStore();
  const info = deviceStore.deviceInfo;
  if (!info) {
    throw new Error("设备未连接");
  }
  return {
    protocol: info.protocol,
    protocolLabel: info.protocolLabel,
    keyboardType: info.keyboardType,
    keyboardTypeName: deviceStore.keyboardTypeName,
    actualKeyCount: info.actualKeyCount,
    maxLayers: info.maxLayers,
    macroSlots: info.macroSlots,
    fnKeyCount: info.fnKeyCount,
    firmwareVersion: deviceStore.firmwareVersion,
    capabilities: {
      multiLayer: info.capabilities.multiLayer,
      layerKeyActions: info.capabilities.layerKeyActions,
      rgb: info.capabilities.rgb,
      fnKeys: info.capabilities.fnKeys,
      macroActions: info.capabilities.macroActions,
      osMode: info.capabilities.osMode,
      wheelClickAction: info.capabilities.wheelClickAction,
    },
  };
}

function validateMacroPack(value: unknown): MacroPackDocument {
  if (!isRecord(value) || value.format !== MACRO_PACK_FORMAT || value.version !== ASSET_FORMAT_VERSION) {
    throw new Error("宏包格式不正确");
  }
  if (!Array.isArray(value.macros)) {
    throw new Error("宏包缺少宏数据");
  }
  value.macros.forEach((macro, macroIndex) => {
    if (!isRecord(macro) || typeof macro.name !== "string" || !Number.isInteger(macro.sourceSlot)) {
      throw new Error(`宏 ${macroIndex + 1} 结构无效`);
    }
    if (!Array.isArray(macro.actions)) {
      throw new Error(`宏 ${macroIndex + 1} 缺少动作数据`);
    }
    macro.actions.forEach((action, actionIndex) => {
      if (!isRecord(action) || !VALID_MACRO_ACTION_TYPES.has(action.type as number) || !isByte(action.param)) {
        throw new Error(`宏 ${macroIndex + 1} 动作 ${actionIndex + 1} 参数无效`);
      }
    });
  });
  return value as unknown as MacroPackDocument;
}

export const useStudioAssetStore = defineStore("studioAsset", () => {
  const isLoading = ref(false);
  const progress = ref<ApplyProgress>({
    active: false,
    title: "",
    current: 0,
    total: 0,
    detail: "",
  });

  function setProgress(title: string, current: number, total: number, detail: string): void {
    progress.value = { active: true, title, current, total, detail };
  }

  async function exportMacroPack(slot?: number): Promise<string> {
    const macroStore = useMacroStore();
    const sourceDevice = sourceFromCurrentDevice();
    await macroStore.refreshOverview();
    const slots = slot === undefined
      ? macroStore.slotValid.map((valid, index) => ({ valid, index })).filter((item) => item.valid).map((item) => item.index)
      : [slot];
    const macros: MacroAsset[] = [];
    for (const macroSlot of slots) {
      const data = await macroStore.loadMacro(macroSlot);
      if (!data.header.valid && slot !== undefined) {
        throw new Error("这个宏槽是空的");
      }
      if (!data.header.valid) continue;
      macros.push({
        name: data.header.name || `宏 ${macroSlot + 1}`,
        sourceSlot: macroSlot,
        actions: cloneAsset(data.actions),
      });
    }
    const pack: MacroPackDocument = {
      format: MACRO_PACK_FORMAT,
      version: ASSET_FORMAT_VERSION,
      name: slot === undefined ? `${sourceDevice.keyboardTypeName} 宏包` : macros[0]?.name ?? "宏包",
      createdAt: new Date().toISOString(),
      sourceDevice,
      macros,
    };
    return JSON.stringify(pack, null, 2);
  }

  async function importMacroPackJson(json: string, options: MacroImportOptions): Promise<number> {
    const macroStore = useMacroStore();
    const pack = validateMacroPack(JSON.parse(json) as unknown);
    if (!pack.macros.length) {
      throw new Error("宏包里没有宏");
    }
    await macroStore.refreshOverview();
    const startSlot = options.strategy === "overwrite"
      ? (options.targetSlot ?? 0)
      : options.strategy === "append"
        ? macroStore.usedCount
        : 0;

    const macrosToImport = options.single ? pack.macros.slice(0, 1) : pack.macros;
    let imported = 0;
    progress.value.active = true;
    try {
      for (let i = 0; i < macrosToImport.length; i++) {
        const macro = macrosToImport[i];
        const targetSlot = options.strategy === "original"
          ? macro.sourceSlot
          : startSlot + i;
        setProgress(options.single ? "导入宏" : "导入宏包", i + 1, macrosToImport.length, `写入宏 ${targetSlot + 1}`);
        const name = macroStore.maxNameBytes > 0
          ? truncateUtf8ByBytes(macro.name, macroStore.maxNameBytes || MACRO_NAME_MAX_BYTES)
          : "";
        const actions = macro.actions.some((action) => action.type === MacroActionType.END)
          ? cloneAsset(macro.actions)
          : [...cloneAsset(macro.actions), { type: MacroActionType.END, param: 0 }];
        const data: MacroData = {
          header: {
            valid: 1,
            id: targetSlot,
            actionCount: actions.filter((action) => action.type !== MacroActionType.END).length,
            dataSize: actions.filter((action) => action.type !== MacroActionType.END).length * 2,
            name,
          },
          actions,
        };
        await hidService.setMacroData(targetSlot, data);
        imported++;
      }
      macroStore.invalidateCache();
      await macroStore.refreshOverview();
      return imported;
    } finally {
      progress.value.active = false;
    }
  }

  return {
    isLoading,
    progress,
    exportMacroPack,
    importMacroPackJson,
  };
});
