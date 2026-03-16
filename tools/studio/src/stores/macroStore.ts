/**
 * BinaryKeyboard 宏状态管理
 *
 * 使用 MacroCard 模型：每张卡片 = 1 个动作 + 后向延时(ms)
 * 序列化时自动拆分 >2550ms 的延时为多个 DELAY 动作
 */

import { defineStore } from "pinia";
import { ref, computed } from "vue";
import { hidService } from "@/services/HidService";
import { showToast } from "@/services/toastService";
import type {
  MacroOverview,
  MacroData,
  MacroAction,
  MacroHeader,
} from "@/types/protocol";
import {
  MacroActionType,
  MACRO_SLOTS,
  MACRO_MAX_ACTIONS,
  MACRO_MAX_DATA_SIZE,
  MACRO_NAME_MAX_BYTES,
} from "@/types/protocol";
import { truncateUtf8ByBytes } from "@/utils/utf8";

/** 单张宏卡片：动作 + 后向延时 */
export interface MacroCard {
  action: MacroAction;
  delayMs: number;
}

/** 单个 DELAY 动作上限: param(uint8) * 10ms = 2550ms */
const DELAY_ACTION_MAX_MS = 2550;

// ============================================================================
// 卡片 ↔ 扁平动作列表 转换
// ============================================================================

/** 扁平 MacroAction[] → MacroCard[] */
function parseActions(actions: MacroAction[]): MacroCard[] {
  const cards: MacroCard[] = [];
  let i = 0;
  while (i < actions.length) {
    const a = actions[i];
    if (a.type === MacroActionType.END) break;

    // 跳过孤立的 DELAY（不应该出现，但防御性处理）
    if (a.type === MacroActionType.DELAY) {
      // 如果上一张卡片存在，把延时累加上去
      if (cards.length > 0) {
        cards[cards.length - 1].delayMs += a.param * 10;
      }
      i++;
      continue;
    }

    const card: MacroCard = { action: { ...a }, delayMs: 0 };
    i++;

    // 收集紧跟的 DELAY 动作，累加为 delayMs
    while (i < actions.length && actions[i].type === MacroActionType.DELAY) {
      card.delayMs += actions[i].param * 10;
      i++;
    }

    cards.push(card);
  }
  return cards;
}

/** MacroCard[] → 扁平 MacroAction[]（含末尾 END） */
function flattenCards(cards: MacroCard[]): MacroAction[] {
  const actions: MacroAction[] = [];
  for (const card of cards) {
    actions.push({ ...card.action });
    // 拆分延时为多个 DELAY 动作
    let remaining = card.delayMs;
    while (remaining > 0) {
      const chunk = Math.min(remaining, DELAY_ACTION_MAX_MS);
      actions.push({
        type: MacroActionType.DELAY,
        param: Math.round(chunk / 10),
      });
      remaining -= chunk;
    }
  }
  actions.push({ type: MacroActionType.END, param: 0 });
  return actions;
}

/** 计算卡片列表序列化后的总动作数和字节数 */
function calcFlatSize(cards: MacroCard[]): {
  actionCount: number;
  dataSize: number;
} {
  let count = 0;
  for (const card of cards) {
    count++; // 动作本身
    if (card.delayMs > 0) {
      count += Math.ceil(card.delayMs / DELAY_ACTION_MAX_MS);
    }
  }
  count++; // END
  return { actionCount: count, dataSize: count * 2 };
}

export const useMacroStore = defineStore("macro", () => {
  // ========================================
  // 状态
  // ========================================

  /** 槽位概览 */
  const overview = ref<MacroOverview | null>(null);

  /** 已加载的宏数据缓存 */
  const loadedMacros = ref<Map<number, MacroData>>(new Map());
  /** 宏头部缓存（用于槽位名称等轻量信息） */
  const macroHeaders = ref<Map<number, MacroHeader>>(new Map());

  /** 正在编辑的槽位 (-1 表示未编辑) */
  const editingSlot = ref(-1);

  /** 编辑中的卡片列表 */
  const editingCards = ref<MacroCard[]>([]);

  /** 编辑中的宏名称 */
  const editingName = ref("");

  /** 加载状态 */
  const isLoading = ref(false);

  /** 保存状态 */
  const isSaving = ref(false);

  // ========================================
  // 计算属性
  // ========================================

  /** 是否正在编辑 */
  const isEditing = computed(() => editingSlot.value >= 0);

  /** 已使用的槽位数 */
  const usedCount = computed(() => overview.value?.usedCount ?? 0);

  /** 总槽位数 */
  const totalSlots = computed(() => overview.value?.totalSlots ?? MACRO_SLOTS);

  /** 各槽位是否有效 */
  const slotValid = computed(
    () => overview.value?.slotValid ?? new Array(MACRO_SLOTS).fill(false),
  );

  /** 序列化后的动作数和数据大小 */
  const editingFlatSize = computed(() => calcFlatSize(editingCards.value));

  /** 编辑中的数据大小 (字节) */
  const editingDataSize = computed(() => editingFlatSize.value.dataSize);

  /** 编辑中的总动作数 */
  const editingActionCount = computed(() => editingFlatSize.value.actionCount);

  /** 编辑中的数据是否超限 */
  const editingOverLimit = computed(
    () =>
      editingActionCount.value > MACRO_MAX_ACTIONS ||
      editingDataSize.value > MACRO_MAX_DATA_SIZE,
  );

  // ========================================
  // 方法
  // ========================================

  /** 刷新槽位概览 */
  async function refreshOverview(): Promise<void> {
    try {
      const nextOverview = await hidService.getMacroOverview();
      overview.value = nextOverview;
      const nextHeaders = new Map<number, MacroHeader>();

      for (let slot = 0; slot < nextOverview.slotValid.length; slot++) {
        if (!nextOverview.slotValid[slot]) {
          loadedMacros.value.delete(slot);
          continue;
        }

        const cachedData = loadedMacros.value.get(slot);
        if (cachedData) {
          nextHeaders.set(slot, cachedData.header);
          continue;
        }

        try {
          const header = await hidService.getMacroInfo(slot);
          nextHeaders.set(slot, header);
        } catch {
          const cachedHeader = macroHeaders.value.get(slot);
          if (cachedHeader) {
            nextHeaders.set(slot, cachedHeader);
          }
        }
      }

      macroHeaders.value = nextHeaders;
    } catch (error) {
      overview.value = null;
      throw error;
    }
  }

  /** 加载指定槽位的宏数据 */
  async function loadMacro(slot: number): Promise<MacroData> {
    const cached = loadedMacros.value.get(slot);
    if (cached) return cached;

    const data = await hidService.getMacroData(slot);
    loadedMacros.value.set(slot, data);
    macroHeaders.value.set(slot, data.header);
    return data;
  }

  /** 保存宏到设备 */
  async function saveMacro(): Promise<void> {
    if (editingSlot.value < 0) return;

    const safeName = truncateUtf8ByBytes(editingName.value, MACRO_NAME_MAX_BYTES);
    if (safeName !== editingName.value) {
      editingName.value = safeName;
    }
    if (editingOverLimit.value) {
      showToast("error", "数据超限", "宏动作数量或数据大小超过限制");
      return;
    }

    isSaving.value = true;
    try {
      const actions = flattenCards(editingCards.value);
      const dataSize = actions.length * 2;

      const macro: MacroData = {
        header: {
          valid: 1,
          id: editingSlot.value,
          actionCount: actions.length,
          dataSize,
          name: safeName,
        },
        actions,
      };

      await hidService.setMacroData(editingSlot.value, macro);
      await hidService.saveConfig();

      loadedMacros.value.set(editingSlot.value, macro);
      macroHeaders.value.set(editingSlot.value, macro.header);
      await refreshOverview();

      showToast("success", "保存成功", `宏 ${editingSlot.value} 已写入设备`);
    } catch (error) {
      showToast(
        "error",
        "保存失败",
        error instanceof Error ? error.message : "写入宏数据时发生错误",
      );
      throw error;
    } finally {
      isSaving.value = false;
    }
  }

  /** 删除指定槽位的宏 */
  async function deleteMacro(slot: number): Promise<void> {
    isLoading.value = true;
    try {
      await hidService.deleteMacro(slot);
      await hidService.saveConfig();

      loadedMacros.value.delete(slot);
      macroHeaders.value.delete(slot);
      await refreshOverview();

      if (editingSlot.value === slot) {
        cancelEditing();
      }

      showToast("success", "删除成功", `宏 ${slot} 已删除`);
    } catch (error) {
      showToast(
        "error",
        "删除失败",
        error instanceof Error ? error.message : "删除宏时发生错误",
      );
      throw error;
    } finally {
      isLoading.value = false;
    }
  }

  /** 开始编辑指定槽位 */
  async function startEditing(slot: number): Promise<void> {
    isLoading.value = true;
    try {
      if (slotValid.value[slot]) {
        const data = await loadMacro(slot);
        editingSlot.value = slot;
        editingName.value = data.header.name;
        editingCards.value = parseActions(data.actions);
      } else {
        editingSlot.value = slot;
        editingName.value = "";
        editingCards.value = [];
      }
    } catch (error) {
      showToast(
        "error",
        "加载失败",
        error instanceof Error ? error.message : "读取宏数据时发生错误",
      );
    } finally {
      isLoading.value = false;
    }
  }

  /** 取消编辑 */
  function cancelEditing(): void {
    editingSlot.value = -1;
    editingCards.value = [];
    editingName.value = "";
  }

  /** 重置所有状态 (断开连接时调用) */
  function reset(): void {
    overview.value = null;
    loadedMacros.value = new Map();
    macroHeaders.value = new Map();
    cancelEditing();
    isLoading.value = false;
    isSaving.value = false;
  }

  /** 清除指定槽位的缓存 */
  function invalidateCache(slot?: number): void {
    if (slot !== undefined) {
      loadedMacros.value.delete(slot);
      macroHeaders.value.delete(slot);
    } else {
      loadedMacros.value = new Map();
      macroHeaders.value = new Map();
    }
  }

  function getSlotDisplayName(slot: number): string {
    const header = macroHeaders.value.get(slot);
    return header?.name || `宏 ${slot + 1}`;
  }

  return {
    // 状态
    overview,
    loadedMacros,
    macroHeaders,
    editingSlot,
    editingCards,
    editingName,
    isLoading,
    isSaving,

    // 计算属性
    isEditing,
    usedCount,
    totalSlots,
    slotValid,
    editingDataSize,
    editingActionCount,
    editingOverLimit,

    // 方法
    refreshOverview,
    loadMacro,
    saveMacro,
    deleteMacro,
    startEditing,
    cancelEditing,
    reset,
    invalidateCache,
    getSlotDisplayName,
  };
});
