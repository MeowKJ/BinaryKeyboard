import { computed, ref } from "vue";
import { defineStore } from "pinia";
import { hidService } from "@/services/HidService";
import {
  deleteConfigProfile,
  listConfigProfiles,
  putConfigProfile,
} from "@/services/studioAssetDb";
import { useDeviceStore } from "@/stores/deviceStore";
import { useMacroStore } from "@/stores/macroStore";
import {
  ActionType,
  DeviceProtocol,
  MacroActionType,
  MACRO_NAME_MAX_BYTES,
  type KeyAction,
  type KeymapConfig,
  type MacroData,
} from "@/types/protocol";
import {
  ASSET_FORMAT_VERSION,
  CONFIG_PROFILE_FORMAT,
  MACRO_PACK_FORMAT,
  cloneAsset,
  type ApplyPlanItem,
  type ApplyProgress,
  type AssetSourceDevice,
  type ConfigProfileDocument,
  type MacroAsset,
  type MacroImportOptions,
  type MacroPackDocument,
  type MacroReference,
} from "@/types/studioAssets";
import { truncateUtf8ByBytes } from "@/utils/utf8";

function createId(): string {
  if (typeof crypto !== "undefined" && "randomUUID" in crypto) {
    return crypto.randomUUID();
  }
  return `asset-${Date.now()}-${Math.random().toString(16).slice(2)}`;
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === "object" && value !== null;
}

function collectMacroRefs(keymap: KeymapConfig): MacroReference[] {
  const refs: MacroReference[] = [];
  keymap.layers.forEach((layer, layerIndex) => {
    layer.keys.forEach((key, keyIndex) => {
      if (key.type === ActionType.MACRO) {
        refs.push({ layer: layerIndex, key: keyIndex, slot: key.param1 });
      }
    });
  });
  return refs;
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

function validateProfileDoc(value: unknown): ConfigProfileDocument {
  if (!isRecord(value) || value.format !== CONFIG_PROFILE_FORMAT || value.version !== ASSET_FORMAT_VERSION) {
    throw new Error("配置档案格式不正确");
  }
  if (!isRecord(value.config) || !isRecord(value.config.keymap)) {
    throw new Error("配置档案缺少键位数据");
  }
  return value as unknown as ConfigProfileDocument;
}

function validateMacroPack(value: unknown): MacroPackDocument {
  if (!isRecord(value) || value.format !== MACRO_PACK_FORMAT || value.version !== ASSET_FORMAT_VERSION) {
    throw new Error("宏包格式不正确");
  }
  if (!Array.isArray(value.macros)) {
    throw new Error("宏包缺少宏数据");
  }
  return value as unknown as MacroPackDocument;
}

function sanitizeKeyAction(action: KeyAction): KeyAction {
  const deviceStore = useDeviceStore();
  if (action.type === ActionType.LAYER && !deviceStore.supportsLayerKeyActions) {
    return { type: ActionType.NONE, modifier: 0, param1: 0, param2: 0 };
  }
  if (action.type === ActionType.MACRO && !deviceStore.supportsMacroActions) {
    return { type: ActionType.NONE, modifier: 0, param1: 0, param2: 0 };
  }
  return { ...action };
}

function adaptKeymapForCurrentDevice(source: KeymapConfig): KeymapConfig {
  const deviceStore = useDeviceStore();
  const target = cloneAsset(deviceStore.keymap);
  const maxLayers = Math.min(target.numLayers, source.numLayers, deviceStore.deviceInfo?.maxLayers || target.numLayers);
  const keyCount = deviceStore.actualKeyCount;

  target.numLayers = maxLayers;
  target.currentLayer = Math.min(source.currentLayer, Math.max(0, maxLayers - 1));
  target.defaultLayer = Math.min(source.defaultLayer, Math.max(0, maxLayers - 1));

  for (let layer = 0; layer < maxLayers; layer++) {
    for (let key = 0; key < keyCount; key++) {
      const sourceAction = source.layers[layer]?.keys[key];
      if (sourceAction) {
        target.layers[layer].keys[key] = sanitizeKeyAction(sourceAction);
      }
    }
  }
  return target;
}

export const useStudioAssetStore = defineStore("studioAsset", () => {
  const profiles = ref<ConfigProfileDocument[]>([]);
  const isLoading = ref(false);
  const progress = ref<ApplyProgress>({
    active: false,
    title: "",
    current: 0,
    total: 0,
    detail: "",
  });

  const profileCount = computed(() => profiles.value.length);

  async function refreshProfiles(): Promise<void> {
    profiles.value = await listConfigProfiles();
  }

  async function saveCurrentProfile(name?: string): Promise<ConfigProfileDocument> {
    const deviceStore = useDeviceStore();
    const sourceDevice = sourceFromCurrentDevice();
    const now = new Date().toISOString();
    const profile: ConfigProfileDocument = {
      format: CONFIG_PROFILE_FORMAT,
      version: ASSET_FORMAT_VERSION,
      id: createId(),
      name: name?.trim() || `${deviceStore.keyboardTypeName} 配置`,
      createdAt: now,
      updatedAt: now,
      sourceDevice,
      config: {
        keymap: cloneAsset(deviceStore.keymap),
        rgb: deviceStore.supportsRgb ? cloneAsset(deviceStore.rgbConfig) : undefined,
        fn: deviceStore.supportsFnKeys ? cloneAsset(deviceStore.fnKeyConfig) : undefined,
        osMode: deviceStore.supportsOsMode ? cloneAsset(deviceStore.osModeConfig) : undefined,
      },
      macroRefs: collectMacroRefs(deviceStore.keymap),
    };
    await putConfigProfile(profile);
    await refreshProfiles();
    return profile;
  }

  async function removeProfile(id: string): Promise<void> {
    await deleteConfigProfile(id);
    await refreshProfiles();
  }

  function getProfile(id: string): ConfigProfileDocument {
    const profile = profiles.value.find((item) => item.id === id);
    if (!profile) throw new Error("配置档案不存在");
    return profile;
  }

  function exportProfile(id: string): string {
    return JSON.stringify(getProfile(id), null, 2);
  }

  function exportAllProfiles(): string {
    return JSON.stringify(
      {
        format: "meowkeyboard-config-profile-library",
        version: ASSET_FORMAT_VERSION,
        exportedAt: new Date().toISOString(),
        profiles: profiles.value,
      },
      null,
      2,
    );
  }

  async function importProfileJson(json: string): Promise<number> {
    const parsed = JSON.parse(json) as unknown;
    const docs = isRecord(parsed) && Array.isArray(parsed.profiles)
      ? parsed.profiles.map(validateProfileDoc)
      : [validateProfileDoc(parsed)];
    for (const doc of docs) {
      const now = new Date().toISOString();
      await putConfigProfile({
        ...cloneAsset(doc),
        id: doc.id || createId(),
        updatedAt: now,
      });
    }
    await refreshProfiles();
    return docs.length;
  }

  function buildApplyPlan(profile: ConfigProfileDocument): ApplyPlanItem[] {
    const deviceStore = useDeviceStore();
    const target = deviceStore.deviceInfo;
    if (!target) return [{ section: "设备", severity: "error", message: "设备未连接" }];

    const items: ApplyPlanItem[] = [];
    if (profile.sourceDevice.protocol === DeviceProtocol.CH592 && target.protocol === DeviceProtocol.CH552) {
      items.push({ section: "协议", severity: "error", message: "无线款配置不能导入到有线款" });
      return items;
    }

    if (profile.sourceDevice.protocol === DeviceProtocol.CH552 && target.protocol === DeviceProtocol.CH592) {
      items.push({ section: "协议", severity: "ok", message: "有线款配置将迁移到无线款" });
    } else if (profile.sourceDevice.protocol === target.protocol) {
      items.push({ section: "协议", severity: "ok", message: "协议匹配" });
    } else {
      items.push({ section: "协议", severity: "error", message: "未知协议组合" });
    }

    const sourceKeys = profile.sourceDevice.actualKeyCount;
    const targetKeys = target.actualKeyCount;
    if (sourceKeys > targetKeys) {
      items.push({ section: "键位", severity: "warn", message: `只应用前 ${targetKeys} 个按键，跳过 ${sourceKeys - targetKeys} 个` });
    } else {
      items.push({ section: "键位", severity: "ok", message: "键位可应用" });
    }

    if (profile.config.rgb && !target.capabilities.rgb) {
      items.push({ section: "RGB", severity: "skip", message: "当前设备不支持 RGB，跳过" });
    } else if (profile.config.rgb) {
      items.push({ section: "RGB", severity: "ok", message: "RGB 可应用" });
    }

    if (profile.config.osMode && !target.capabilities.osMode) {
      items.push({ section: "系统模式", severity: "skip", message: "当前设备不支持 Win/Mac 模式，跳过" });
    } else if (profile.config.osMode) {
      items.push({ section: "系统模式", severity: "ok", message: "系统模式可应用" });
    }

    if (profile.macroRefs.length > 0) {
      items.push({ section: "宏引用", severity: "warn", message: `配置引用 ${profile.macroRefs.length} 个宏槽，请确认宏已单独导入` });
    }
    return items;
  }

  function setProgress(title: string, current: number, total: number, detail: string): void {
    progress.value = { active: true, title, current, total, detail };
  }

  async function applyProfile(id: string): Promise<void> {
    const deviceStore = useDeviceStore();
    const profile = getProfile(id);
    const plan = buildApplyPlan(profile);
    if (plan.some((item) => item.severity === "error")) {
      throw new Error(plan.find((item) => item.severity === "error")?.message ?? "配置不兼容");
    }

    const steps = [
      "写入键位",
      ...(profile.config.rgb && deviceStore.supportsRgb ? ["写入 RGB"] : []),
      ...(profile.config.osMode && deviceStore.supportsOsMode ? ["写入系统模式"] : []),
      "回读确认",
    ];
    let current = 0;
    progress.value.active = true;
    try {
      setProgress("应用配置档案", ++current, steps.length, "写入键位");
      deviceStore.keymap = adaptKeymapForCurrentDevice(profile.config.keymap);
      await deviceStore.saveKeymap();

      if (profile.config.rgb && deviceStore.supportsRgb) {
        setProgress("应用配置档案", ++current, steps.length, "写入 RGB");
        deviceStore.rgbConfig = cloneAsset(profile.config.rgb);
        await deviceStore.saveRgbConfig();
      }

      if (profile.config.osMode && deviceStore.supportsOsMode) {
        setProgress("应用配置档案", ++current, steps.length, "写入系统模式");
        await deviceStore.saveOsMode(profile.config.osMode.mode);
      }

      setProgress("应用配置档案", ++current, steps.length, "回读确认");
      await deviceStore.refreshKeymap();
      if (deviceStore.supportsRgb) await deviceStore.refreshRgbConfig();
      if (deviceStore.supportsOsMode) await deviceStore.refreshOsMode();
    } finally {
      progress.value.active = false;
    }
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

    let imported = 0;
    progress.value.active = true;
    try {
      for (let i = 0; i < pack.macros.length; i++) {
        const macro = pack.macros[i];
        const targetSlot = options.strategy === "original"
          ? macro.sourceSlot
          : startSlot + i;
        setProgress("导入宏包", i + 1, pack.macros.length, `写入宏 ${targetSlot + 1}`);
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
    profiles,
    isLoading,
    progress,
    profileCount,
    refreshProfiles,
    saveCurrentProfile,
    removeProfile,
    exportProfile,
    exportAllProfiles,
    importProfileJson,
    buildApplyPlan,
    applyProfile,
    exportMacroPack,
    importMacroPackJson,
  };
});
