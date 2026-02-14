<template>
  <button 
    class="key-button" 
    :class="[sizeClass, typeClass, { selected, 'has-action': hasAction, disabled }]"
    :style="gridStyle"
    :disabled="disabled"
    @click="!disabled && emit('click')"
  >
    <span class="key-label">{{ displayLabel }}</span>
    <span v-if="actionBadge" class="key-badge" :class="badgeClass">{{ actionBadge }}</span>
  </button>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import type { KeyDef, KeySize } from '@/config/layouts';
import { ActionType, type KeyAction } from '@/types/protocol';
import { getKeycodeName } from '@/utils/keycodes';
import { getConsumerName } from '@/utils/consumer';

const props = defineProps<{
  /** 按键定义 */
  keyDef: KeyDef;
  /** 按键动作 */
  action: KeyAction;
  /** 是否选中 */
  selected: boolean;
  /** 是否禁用 */
  disabled?: boolean;
}>();

const emit = defineEmits<{
  click: [];
}>();

/** 网格定位样式 */
const gridStyle = computed(() => {
  const k = props.keyDef;
  const rowSpan = k.size === '2u-v' ? 2 : 1;
  const colSpan = k.size === '2u-h' ? 2 : 1;
  
  return {
    gridRow: `${Math.floor(k.row) + 1} / span ${rowSpan}`,
    gridColumn: `${k.col + 1} / span ${colSpan}`,
  };
});

/** 尺寸类名 */
const sizeClass = computed(() => `key-${props.keyDef.size}`);

/** 类型类名 (旋钮等特殊按键) */
const typeClass = computed(() => {
  if (props.keyDef.type && props.keyDef.type !== 'normal') {
    return `key-type-${props.keyDef.type}`;
  }
  return '';
});

/** 是否有动作 */
const hasAction = computed(() => {
  return props.action && props.action.type !== ActionType.NONE;
});

/** 显示标签 */
const displayLabel = computed(() => {
  // 自定义标签优先
  if (props.keyDef.label) return props.keyDef.label;
  
  const action = props.action;
  if (!action || action.type === ActionType.NONE) {
    return '—';
  }

  switch (action.type) {
    case ActionType.KEYBOARD:
      return getKeycodeName(action.param1, action.modifier) || `0x${action.param1.toString(16).toUpperCase()}`;
    case ActionType.MOUSE_BTN:
      return getMouseButtonName(action.param1);
    case ActionType.MOUSE_WHEEL:
      return getWheelName(action.param1);
    case ActionType.CONSUMER:
      const code = action.param1 | (action.param2 << 8);
      return getConsumerName(code) || '媒体';
    case ActionType.MACRO:
      return `M${action.param1 + 1}`;
    case ActionType.LAYER:
      return `L${action.param1 + 1}`;
    default:
      return '—';
  }
});

/** 动作类型徽章 */
const actionBadge = computed(() => {
  const action = props.action;
  if (!action || action.type === ActionType.NONE) return null;

  switch (action.type) {
    case ActionType.KEYBOARD: return '⌨';
    case ActionType.MOUSE_BTN: return '🖱';
    case ActionType.MOUSE_WHEEL: return '⚙';
    case ActionType.CONSUMER: return '🎵';
    case ActionType.MACRO: return '📝';
    case ActionType.LAYER: return '📚';
    default: return null;
  }
});

/** 徽章样式类 */
const badgeClass = computed(() => {
  const action = props.action;
  if (!action) return '';
  
  switch (action.type) {
    case ActionType.KEYBOARD: return 'badge-keyboard';
    case ActionType.CONSUMER: return 'badge-media';
    case ActionType.MOUSE_BTN:
    case ActionType.MOUSE_WHEEL: return 'badge-mouse';
    case ActionType.LAYER: return 'badge-layer';
    case ActionType.MACRO: return 'badge-macro';
    default: return '';
  }
});

function getMouseButtonName(btn: number): string {
  if (btn & 0x01) return '左键';
  if (btn & 0x02) return '右键';
  if (btn & 0x04) return '中键';
  if (btn & 0x08) return '后退';
  if (btn & 0x10) return '前进';
  return '🖱';
}

function getWheelName(dir: number): string {
  switch (dir) {
    case 1: return '↑';
    case 2: return '↓';
    case 3: return '●';
    default: return '⚙';
  }
}
</script>

<style scoped>
.key-button {
  position: relative;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 4px;
  
  background: var(--c-key-bg);
  border: 2px solid var(--c-key-border);
  border-radius: var(--radius-md);
  
  cursor: pointer;
  font-family: inherit;
  color: var(--c-text-muted);
  
  transition: all var(--transition-fast);
}

.key-button:hover {
  background: var(--c-bg-hover);
  border-color: var(--c-accent);
  color: var(--c-text-primary);
  transform: translateY(-2px);
  box-shadow: 0 6px 20px var(--c-key-shadow);
}

.key-button.selected {
  background: var(--c-key-active-bg);
  border-color: var(--c-key-active-border);
  color: var(--c-accent);
  box-shadow: 0 0 0 3px var(--c-accent-soft), 0 8px 24px var(--c-key-shadow);
}

.key-button.has-action {
  color: var(--c-text-primary);
}

.key-button.has-action .key-label {
  font-weight: 700;
}

.key-button.disabled {
  cursor: not-allowed;
  opacity: 0.6;
  pointer-events: none;
}

.key-button.disabled:hover {
  transform: none;
  box-shadow: none;
}

/* 标签 */
.key-label {
  font-size: 0.9375rem;
  font-weight: 600;
  max-width: 90%;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  line-height: 1.2;
}

/* 徽章 */
.key-badge {
  position: absolute;
  top: 6px;
  right: 6px;
  font-size: 0.625rem;
  opacity: 0.7;
}

/* ==========================================
   按键尺寸
========================================== */
.key-1u {
  width: var(--key-unit);
  height: var(--key-unit);
}

.key-2u-h {
  width: calc(var(--key-unit) * 2 + var(--key-gap));
  height: var(--key-unit);
}

.key-2u-v {
  width: var(--key-unit);
  height: calc(var(--key-unit) * 2 + var(--key-gap));
}

/* ==========================================
   旋钮编码器按键
========================================== */
.key-type-encoder-cw,
.key-type-encoder-ccw {
  width: 52px;
  height: 52px;
  border-radius: var(--radius-sm);
}

.key-type-encoder-cw .key-label,
.key-type-encoder-ccw .key-label {
  font-size: 1.5rem;
}

.key-type-encoder-press {
  width: 56px;
  height: 56px;
  border-radius: 50%;
}

.key-type-encoder-press .key-label {
  font-size: 1.25rem;
}

/* ==========================================
   徽章颜色
========================================== */
.badge-keyboard { color: var(--c-info); }
.badge-media { color: var(--c-success); }
.badge-mouse { color: var(--c-warning); }
.badge-layer { color: var(--c-accent); }
.badge-macro { color: var(--c-danger); }
</style>
