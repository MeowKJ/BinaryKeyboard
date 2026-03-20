<script setup lang="ts">
import { useMacroStore } from '@/stores/macroStore';

const macroStore = useMacroStore();

const emit = defineEmits<{
  (e: 'edit', slot: number): void;
}>();
</script>

<template>
  <div class="panel macro-panel">
    <h3 class="panel-title">
      <i class="pi pi-code"></i>
      宏管理
    </h3>
    <div class="macro-panel-meta">
      <span>{{ macroStore.usedCount }} 个宏</span>
      <span v-if="macroStore.fsTotalBytes">· 剩余 {{ macroStore.fsFreeBytes }}B / {{ macroStore.fsTotalBytes }}B</span>
    </div>
    <div class="macro-slot-grid">
      <button
        v-for="i in macroStore.maxSlots"
        :key="i"
        class="macro-slot-btn"
        :class="{ 'has-data': macroStore.slotValid[i - 1] }"
        @click="emit('edit', i - 1)"
      >
        <span class="macro-slot-idx">{{ i }}</span>
        <span class="macro-slot-name">{{ macroStore.getSlotDisplayName(i - 1) }}</span>
      </button>
    </div>
  </div>
</template>

<style scoped>
.panel {
  background: var(--c-bg-secondary);
  border: 1px solid var(--c-border);
  border-radius: var(--radius-lg);
  padding: 0.75rem;
}

.panel-title {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  font-size: 0.85rem;
  font-weight: 700;
  margin: 0 0 0.75rem;
  color: var(--c-text-muted);
}

.panel-title i {
  color: var(--c-accent);
  font-size: 0.9rem;
}

.macro-panel-meta {
  display: flex;
  align-items: center;
  gap: 0.35rem;
  margin: -0.35rem 0 0.75rem;
  color: var(--c-text-muted);
  font-size: 0.72rem;
  font-weight: 700;
}

.macro-slot-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 0.375rem;
}

.macro-slot-btn {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  width: 100%;
  padding: 0.5rem 0.625rem;
  font-size: 0.8rem;
  font-family: inherit;
  font-weight: 600;
  border-radius: var(--radius-sm);
  border: 1px solid var(--c-border);
  background: var(--c-bg-tertiary);
  color: var(--c-text-muted);
  cursor: pointer;
  transition: all var(--transition-fast);
  text-align: left;
  overflow: hidden;
}

.macro-slot-btn:hover {
  background: var(--c-bg-hover);
  border-color: var(--c-accent);
  color: var(--c-text-secondary);
}

.macro-slot-btn.has-data {
  border-color: var(--c-accent);
  color: var(--c-accent-light);
}

.macro-slot-idx {
  width: 18px;
  height: 18px;
  border-radius: 50%;
  background: var(--c-bg-primary);
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 0.65rem;
  font-weight: 800;
  flex-shrink: 0;
}

.macro-slot-btn.has-data .macro-slot-idx {
  background: var(--c-accent-soft);
  color: var(--c-accent);
}

.macro-slot-name {
  flex: 1;
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
</style>
