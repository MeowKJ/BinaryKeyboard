<template>
  <button 
    class="key-button" 
    :class="[shapeClass, { selected }]"
    @click="emit('click')"
  >
    <span class="key-label">{{ displayLabel }}</span>
    <span v-if="actionTypeLabel" class="key-type">{{ actionTypeLabel }}</span>
  </button>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import { ActionType, type KeyAction } from '@/types/protocol';
import { getKeycodeName } from '@/utils/keycodes';
import { getConsumerName } from '@/utils/consumer';

const props = defineProps<{
  index: number;
  action: KeyAction;
  selected: boolean;
  shape: string;
  label?: string;
}>();

const emit = defineEmits<{
  click: [];
}>();

const shapeClass = computed(() => `shape-${props.shape}`);

const displayLabel = computed(() => {
  if (props.label) return props.label;

  const action = props.action;
  if (!action) return `${props.index + 1}`;

  switch (action.type) {
    case ActionType.NONE:
      return '—';
    case ActionType.KEYBOARD:
      return getKeycodeName(action.param1, action.modifier) || `0x${action.param1.toString(16).toUpperCase()}`;
    case ActionType.MOUSE_BTN:
      return getMouseButtonName(action.param1);
    case ActionType.MOUSE_WHEEL:
      return getWheelName(action.param1);
    case ActionType.CONSUMER:
      const code = action.param1 | (action.param2 << 8);
      return getConsumerName(code) || `媒体`;
    case ActionType.MACRO:
      return `M${action.param1 + 1}`;
    case ActionType.LAYER:
      return `L${action.param1 + 1}`;
    default:
      return `${props.index + 1}`;
  }
});

const actionTypeLabel = computed(() => {
  const action = props.action;
  if (!action || action.type === ActionType.NONE) return null;

  switch (action.type) {
    case ActionType.KEYBOARD: return '键盘';
    case ActionType.MOUSE_BTN: return '鼠标';
    case ActionType.MOUSE_WHEEL: return '滚轮';
    case ActionType.CONSUMER: return '媒体';
    case ActionType.MACRO: return '宏';
    case ActionType.LAYER: return '层';
    default: return null;
  }
});

function getMouseButtonName(btn: number): string {
  if (btn & 0x01) return '左键';
  if (btn & 0x02) return '右键';
  if (btn & 0x04) return '中键';
  if (btn & 0x08) return '后退';
  if (btn & 0x10) return '前进';
  return '鼠标';
}

function getWheelName(dir: number): string {
  switch (dir) {
    case 1: return '↑';
    case 2: return '↓';
    case 3: return '●';
    default: return '滚轮';
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
  background: var(--app-surface-hover);
  border: 2px solid var(--app-border);
  border-radius: 12px;
  cursor: pointer;
  transition: all 0.15s ease;
  font-family: inherit;
  color: var(--app-text);
}

.key-button:hover {
  background: var(--app-accent-soft);
  border-color: var(--app-accent);
  transform: translateY(-2px);
}

.key-button.selected {
  background: var(--app-accent-soft);
  border-color: var(--app-accent);
  box-shadow: 0 0 20px rgba(124, 92, 255, 0.3);
}

.key-label {
  font-size: 1rem;
  font-weight: 600;
  max-width: 90%;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.key-type {
  position: absolute;
  bottom: 4px;
  font-size: 0.6rem;
  color: var(--app-text-muted);
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

/* 形状 */
.shape-square {
  width: 80px;
  height: 80px;
}

.shape-tall {
  width: 80px;
  height: 172px;
  grid-row: span 2;
}

.shape-wide {
  width: 172px;
  height: 80px;
  grid-column: span 2;
}

.shape-circle {
  width: 80px;
  height: 80px;
  border-radius: 50%;
}

/* 旋钮样式 */
.shape-encoder-cw,
.shape-encoder-ccw {
  width: 50px;
  height: 50px;
  border-radius: 8px;
  font-size: 1.5rem;
}

.shape-encoder-press {
  width: 60px;
  height: 60px;
  border-radius: 50%;
}
</style>
