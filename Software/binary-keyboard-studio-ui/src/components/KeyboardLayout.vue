<template>
  <div class="keyboard-layout">
    <!-- 主键盘区域 -->
    <div class="keyboard-grid" :style="gridStyle">
      <KeyButton 
        v-for="key in mainKeys" 
        :key="key.index"
        :key-def="key"
        :action="getAction(key.index)"
        :selected="selectedIndex === key.index"
        @click="emit('select', key.index)"
      />
    </div>

    <!-- 旋钮区域 (如果有) -->
    <div v-if="layout.hasEncoder" class="encoder-section">
      <div class="encoder-container">
        <div class="encoder-ring">
          <KeyButton 
            v-for="key in encoderKeys" 
            :key="key.index"
            :key-def="key"
            :action="getAction(key.index)"
            :selected="selectedIndex === key.index"
            @click="emit('select', key.index)"
          />
        </div>
        <span class="encoder-label">旋钮</span>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import { getLayoutByType, type KeyDef } from '@/config/layouts';
import { type KeyAction, createEmptyAction } from '@/types/protocol';
import KeyButton from './KeyButton.vue';

const props = defineProps<{
  keyboardType: number;
  keys: KeyAction[];
  selectedIndex: number;
}>();

const emit = defineEmits<{
  select: [index: number];
}>();

/** 获取当前布局 */
const layout = computed(() => getLayoutByType(props.keyboardType));

/** 主键盘按键 (非旋钮) */
const mainKeys = computed(() => {
  return layout.value.keys.filter(k => !k.type || k.type === 'normal');
});

/** 旋钮按键 */
const encoderKeys = computed(() => {
  return layout.value.keys.filter(k => k.type && k.type.startsWith('encoder'));
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
  gap: 2.5rem;
  align-items: center;
  padding: 1.5rem;
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

/* 旋钮区域 */
.encoder-section {
  display: flex;
  flex-direction: column;
  align-items: center;
}

.encoder-container {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 0.75rem;
  background: var(--c-bg-tertiary);
  border: 1px solid var(--c-border);
  border-radius: var(--radius-xl);
  padding: 1.25rem;
  box-shadow: 
    0 4px 20px rgba(0, 0, 0, 0.2),
    inset 0 1px 0 rgba(255, 255, 255, 0.05);
}

.encoder-ring {
  display: grid;
  grid-template-columns: 52px 56px 52px;
  grid-template-rows: 52px 52px;
  gap: 6px;
  align-items: center;
  justify-items: center;
}

/* 旋钮布局调整 */
.encoder-ring > :nth-child(1) { grid-area: 1 / 1; }  /* 顺时针 */
.encoder-ring > :nth-child(2) { grid-area: 2 / 1; }  /* 逆时针 */
.encoder-ring > :nth-child(3) { grid-area: 1 / 2 / 3 / 3; }  /* 按下 */

.encoder-label {
  font-size: 0.8rem;
  font-weight: 600;
  color: var(--c-text-muted);
  text-transform: uppercase;
  letter-spacing: 0.5px;
}
</style>
