/**
 * BinaryKeyboard 调试终端状态管理
 * 记录所有 HID 双向通讯数据 + 固件推送日志
 */

import { defineStore } from 'pinia';
import { ref, computed } from 'vue';

// ============================================================================
// 类型定义
// ============================================================================

export type LogDirection = 'send' | 'receive' | 'device';
export type LogLevel = 'info' | 'success' | 'warning' | 'error';

/** 固件日志类别 */
export type LogCategory =
  | 'key' | 'fn' | 'layer' | 'mode'
  | 'ble' | 'rgb' | 'system';

/** 命令组分类 (用于发光色) */
export type CmdGroup =
  | 'system' | 'config' | 'keymap' | 'rgb'
  | 'macro' | 'fnkey' | 'battery' | 'log' | 'error' | 'unknown';

/** 发光颜色映射 */
export const GLOW_COLORS: Record<CmdGroup, string> = {
  system: '#22d3ee',   // 青色
  config: '#fbbf24',   // 琥珀
  keymap: '#a78bfa',   // 紫色
  rgb: '#f472b6',      // 粉色
  macro: '#fb923c',    // 橙色
  fnkey: '#60a5fa',    // 蓝色
  battery: '#4ade80',  // 绿色
  log: '#38bdf8',      // 天蓝
  error: '#fb7185',    // 红色
  unknown: '#94a3b8',  // 灰色
};

/** 固件日志子类别发光色 */
export const LOG_CATEGORY_COLORS: Record<LogCategory, string> = {
  key: '#a78bfa',      // 紫色
  fn: '#60a5fa',       // 蓝色
  layer: '#fbbf24',    // 琥珀
  mode: '#22d3ee',     // 青色
  ble: '#38bdf8',      // 天蓝
  rgb: '#f472b6',      // 粉色
  system: '#4ade80',   // 绿色
};

/** 根据 CMD hex 获取命令组 */
export function getCmdGroup(cmdHex: string, level: LogLevel): CmdGroup {
  if (level === 'error') return 'error';
  const cmd = parseInt(cmdHex, 16);
  if (cmd >= 0x01 && cmd <= 0x0f) return 'system';
  if (cmd >= 0x10 && cmd <= 0x1f) return 'config';
  if (cmd >= 0x20 && cmd <= 0x2f) return 'keymap';
  if (cmd >= 0x30 && cmd <= 0x3f) return 'rgb';
  if (cmd >= 0x40 && cmd <= 0x4f) return 'macro';
  if (cmd >= 0x50 && cmd <= 0x5f) return 'fnkey';
  if (cmd >= 0x60 && cmd <= 0x6f) return 'battery';
  if (cmd >= 0x70 && cmd <= 0x7f) return 'log';
  return 'unknown';
}

export interface TerminalEntry {
  id: number;
  timestamp: number;
  direction: LogDirection;
  level: LogLevel;
  /** 命令名称 */
  command: string;
  /** 命令码 hex */
  cmdHex: string;
  /** SUB 字段 */
  sub: number;
  /** 数据长度 */
  dataLen: number;
  /** 原始帧 hex dump */
  rawHex: string;
  /** 人类可读的解析结果 */
  parsed: string;
  /** 状态码 (仅响应) */
  statusCode?: string;
  /** 耗时 ms (仅响应) */
  duration?: number;
  /** 固件日志类别 */
  category?: LogCategory;
  /** 发光色 (CSS color) */
  glowColor: string;
}

export type FilterMode = 'all' | 'send' | 'receive' | 'device';

// ============================================================================
// 常量
// ============================================================================

const MAX_ENTRIES = 2000;

// ============================================================================
// Store
// ============================================================================

/** 类别 key → category mask bit */
const CATEGORY_MASK_MAP: Record<LogCategory, number> = {
  key:    0x01,
  fn:     0x02,
  layer:  0x04,
  mode:   0x08,
  ble:    0x10,
  rgb:    0x20,
  system: 0x40,
};

export const useTerminalStore = defineStore('terminal', () => {
  // 状态
  const entries = ref<TerminalEntry[]>([]);
  const isOpen = ref(false);
  const filter = ref<FilterMode>('all');
  const autoScroll = ref(true);
  const panelHeight = ref(280);
  /** 当前展开详情的 entry id (-1 = 无) */
  const expandedId = ref(-1);
  /** UI 端类别过滤掩码 (0x7F = 全部) */
  const categoryMask = ref(0x7F);

  let nextId = 0;
  const pendingTimestamps = new Map<number, number>();

  // 计算属性
  const filteredEntries = computed(() => {
    let list = entries.value;
    // 方向过滤
    if (filter.value !== 'all') {
      list = list.filter(e => e.direction === filter.value);
    }
    // 类别过滤 (仅对 device 方向的日志条目)
    if (categoryMask.value !== 0x7F) {
      list = list.filter(e => {
        if (!e.category) return true; // 非日志条目不过滤
        const bit = CATEGORY_MASK_MAP[e.category];
        return bit ? (categoryMask.value & bit) !== 0 : true;
      });
    }
    return list;
  });

  const sendCount = computed(() => entries.value.filter(e => e.direction === 'send').length);
  const receiveCount = computed(() => entries.value.filter(e => e.direction === 'receive').length);
  const deviceCount = computed(() => entries.value.filter(e => e.direction === 'device').length);
  const errorCount = computed(() => entries.value.filter(e => e.level === 'error').length);

  // 方法
  function addEntry(entry: Omit<TerminalEntry, 'id' | 'timestamp' | 'glowColor'> & { glowColor?: string }) {
    const now = Date.now();
    const group = getCmdGroup(entry.cmdHex, entry.level);
    const glowColor = entry.glowColor
      || (entry.category ? LOG_CATEGORY_COLORS[entry.category] : GLOW_COLORS[group]);

    const fullEntry: TerminalEntry = {
      ...entry,
      id: nextId++,
      timestamp: now,
      glowColor,
    };

    if (entry.direction === 'send') {
      const cmdCode = parseInt(entry.cmdHex, 16);
      pendingTimestamps.set(cmdCode, now);
    }

    if (entry.direction === 'receive') {
      const cmdCode = parseInt(entry.cmdHex, 16);
      const sendTime = pendingTimestamps.get(cmdCode);
      if (sendTime) {
        fullEntry.duration = now - sendTime;
        pendingTimestamps.delete(cmdCode);
      }
    }

    entries.value.push(fullEntry);

    if (entries.value.length > MAX_ENTRIES) {
      entries.value.splice(0, entries.value.length - MAX_ENTRIES);
    }
  }

  function clear() {
    entries.value = [];
    pendingTimestamps.clear();
    expandedId.value = -1;
  }

  function toggle() {
    isOpen.value = !isOpen.value;
  }

  function setFilter(mode: FilterMode) {
    filter.value = mode;
  }

  function toggleExpand(id: number) {
    expandedId.value = expandedId.value === id ? -1 : id;
  }

  function toggleCategory(mask: number) {
    categoryMask.value ^= mask;
    if (categoryMask.value === 0) categoryMask.value = 0x7F; // 不允许全空
  }

  return {
    entries,
    isOpen,
    filter,
    autoScroll,
    panelHeight,
    expandedId,
    categoryMask,
    filteredEntries,
    sendCount,
    receiveCount,
    deviceCount,
    errorCount,
    addEntry,
    clear,
    toggle,
    setFilter,
    toggleExpand,
    toggleCategory,
  };
});
