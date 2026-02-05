<template>
  <div class="keyboard-layout" :class="layoutClass">
    <!-- 基础款 4 键 -->
    <template v-if="keyboardType === KeyboardType.BASIC">
      <KeyButton 
        v-for="(key, i) in displayKeys" 
        :key="i"
        :index="i"
        :action="key"
        :selected="selectedIndex === i"
        :shape="getBasicShape(i)"
        @click="emit('select', i)"
      />
    </template>

    <!-- 五键款 5 键 -->
    <template v-else-if="keyboardType === KeyboardType.FIVE_KEYS">
      <KeyButton 
        v-for="(key, i) in displayKeys" 
        :key="i"
        :index="i"
        :action="key"
        :selected="selectedIndex === i"
        :shape="getFiveKeyShape(i)"
        @click="emit('select', i)"
      />
    </template>

    <!-- 旋钮款 7 虚拟键 -->
    <template v-else-if="keyboardType === KeyboardType.KNOB">
      <div class="knob-layout">
        <div class="knob-main-keys">
          <KeyButton 
            v-for="i in 4" 
            :key="i - 1"
            :index="i - 1"
            :action="keys[i - 1]"
            :selected="selectedIndex === i - 1"
            :shape="getKnobShape(i - 1)"
            @click="emit('select', i - 1)"
          />
        </div>
        <div class="knob-encoder">
          <div class="encoder-ring">
            <KeyButton 
              :index="4"
              :action="keys[4]"
              :selected="selectedIndex === 4"
              shape="encoder-cw"
              label="↻"
              @click="emit('select', 4)"
            />
            <KeyButton 
              :index="6"
              :action="keys[6]"
              :selected="selectedIndex === 6"
              shape="encoder-press"
              label="●"
              @click="emit('select', 6)"
            />
            <KeyButton 
              :index="5"
              :action="keys[5]"
              :selected="selectedIndex === 5"
              shape="encoder-ccw"
              label="↺"
              @click="emit('select', 5)"
            />
          </div>
          <span class="encoder-label">旋钮</span>
        </div>
      </div>
    </template>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import { KeyboardType, type KeyAction, createEmptyAction } from '@/types/protocol';
import KeyButton from './KeyButton.vue';

const props = defineProps<{
  keyboardType: KeyboardType;
  keys: KeyAction[];
  selectedIndex: number;
}>();

const emit = defineEmits<{
  select: [index: number];
}>();

const layoutClass = computed(() => {
  switch (props.keyboardType) {
    case KeyboardType.BASIC: return 'layout-basic';
    case KeyboardType.FIVE_KEYS: return 'layout-five';
    case KeyboardType.KNOB: return 'layout-knob';
    default: return 'layout-basic';
  }
});

const displayKeys = computed(() => {
  const count = props.keyboardType === KeyboardType.FIVE_KEYS ? 5 : 4;
  return props.keys.slice(0, count);
});

function getBasicShape(index: number): string {
  // 基础款布局: 2x2 + 长条
  if (index === 2) return 'tall';
  if (index === 3) return 'wide';
  return 'square';
}

function getFiveKeyShape(index: number): string {
  // 五键款布局: 2x2 + 1
  return 'square';
}

function getKnobShape(index: number): string {
  if (index === 2) return 'tall';
  if (index === 3) return 'wide';
  return 'square';
}
</script>

<style scoped>
.keyboard-layout {
  display: flex;
  justify-content: center;
  align-items: center;
}

/* 基础款布局 */
.layout-basic {
  display: grid;
  grid-template-columns: repeat(3, 80px);
  grid-template-rows: repeat(2, 80px);
  gap: 12px;
}

/* 五键款布局 */
.layout-five {
  display: grid;
  grid-template-columns: repeat(3, 80px);
  grid-template-rows: repeat(2, 80px);
  gap: 12px;
}

/* 旋钮款布局 */
.layout-knob {
  display: flex;
  gap: 2rem;
  align-items: center;
}

.knob-layout {
  display: flex;
  gap: 2rem;
  align-items: center;
}

.knob-main-keys {
  display: grid;
  grid-template-columns: repeat(3, 80px);
  grid-template-rows: repeat(2, 80px);
  gap: 12px;
}

.knob-encoder {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 0.5rem;
}

.encoder-ring {
  display: grid;
  grid-template-columns: 50px 60px 50px;
  grid-template-rows: 50px;
  gap: 4px;
  align-items: center;
}

.encoder-label {
  font-size: 0.8rem;
  color: var(--app-text-muted);
}
</style>
