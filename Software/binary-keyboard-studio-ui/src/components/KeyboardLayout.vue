<template>
  <div class="keyboard-layout">
    <!-- 主键盘区域（包含所有按键，包括旋钮） -->
    <div class="keyboard-grid" :style="gridStyle">
      <template v-for="key in displayKeys" :key="key.index">
        <!-- 旋钮按键：使用特殊的3分区组件 -->
        <EncoderButton v-if="key.type === 'encoder-press' && encoderIndices" :key-def="key" :actions="{
          cw: getAction(encoderIndices.cw),
          press: getAction(encoderIndices.press),
          ccw: getAction(encoderIndices.ccw),
        }" :selected-index="selectedIndex" :encoder-indices="encoderIndices" :disabled="disabled"
          @select="!disabled && emit('select', $event)" />
        <!-- 普通按键 -->
        <KeyButton v-else-if="key.type !== 'encoder-cw' && key.type !== 'encoder-ccw'" :key-def="key"
          :action="getAction(key.index)" :selected="selectedIndex === key.index" :disabled="disabled"
          @click="!disabled && emit('select', key.index)" />
      </template>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import { getLayoutByType, type KeyDef } from '@/config/layouts';
import { type KeyAction, createEmptyAction } from '@/types/protocol';
import KeyButton from './KeyButton.vue';
import EncoderButton from './EncoderButton.vue';

const props = defineProps<{
  keyboardType: number;
  keys: KeyAction[];
  selectedIndex: number;
  disabled?: boolean; // 预览模式下禁用点击
}>();

const emit = defineEmits<{
  select: [index: number];
}>();

/** 获取当前布局 */
const layout = computed(() => getLayoutByType(props.keyboardType));

/** 所有按键（包括旋钮，都集成在主grid中） */
const allKeys = computed(() => {
  return layout.value.keys;
});

/** 显示按键（过滤掉旋钮的 cw 和 ccw，因为它们已经包含在 EncoderButton 中） */
const displayKeys = computed(() => {
  return allKeys.value.filter(key =>
    key.type !== 'encoder-cw' && key.type !== 'encoder-ccw'
  );
});

/** 旋钮的3个按键索引 */
const encoderIndices = computed(() => {
  const encoderPress = allKeys.value.find(k => k.type === 'encoder-press');
  const encoderCw = allKeys.value.find(k => k.type === 'encoder-cw');
  const encoderCcw = allKeys.value.find(k => k.type === 'encoder-ccw');

  if (encoderPress && encoderCw && encoderCcw) {
    return {
      press: encoderPress.index,
      cw: encoderCw.index,
      ccw: encoderCcw.index,
    };
  }
  return null;
});

/** 网格样式 */
const gridStyle = computed(() => {
  const l = layout.value;
  return {
    gridTemplateColumns: `repeat(${l.cols}, var(--key-unit))`,
    gridTemplateRows: `repeat(${l.rows}, var(--key-unit))`,
    gap: 'var(--key-gap)',
  };
});

/** 获取按键动作 */
function getAction(index: number): KeyAction {
  return props.keys[index] || createEmptyAction();
}
</script>

<style scoped>
.keyboard-layout {
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 1.5rem;
  width: 100%;
}

.keyboard-grid {
  display: grid;
  background: var(--c-bg-tertiary);
  border: 1px solid var(--c-border);
  border-radius: var(--radius-xl);
  padding: 1.25rem;
  box-shadow:
    0 4px 20px rgba(0, 0, 0, 0.2),
    inset 0 1px 0 rgba(255, 255, 255, 0.05);
}

/* 响应式 */
@media (max-width: 768px) {
  .keyboard-layout {
    padding: 1rem;
  }

  .keyboard-grid {
    padding: 1rem;
  }
}

@media (max-width: 480px) {
  .keyboard-layout {
    padding: 0.5rem;
  }

  .keyboard-grid {
    padding: 0.75rem;
  }
}
</style>
