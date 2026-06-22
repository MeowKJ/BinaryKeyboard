<script setup lang="ts">
import { computed } from 'vue';
import { useMacroStore } from '@/stores/macroStore';
import StudioSidebarEntry from '@/components/StudioSidebarEntry.vue';

const macroStore = useMacroStore();

const emit = defineEmits<{
  (e: 'edit', slot: number): void;
}>();

const macroCountLabel = computed(() => {
  if (macroStore.usedCount <= 0) return '暂无宏';
  return `${macroStore.usedCount} 个宏`;
});

function openEditor() {
  const firstUsed = macroStore.slotValid.findIndex(Boolean);
  emit('edit', firstUsed >= 0 ? firstUsed : 0);
}
</script>

<template>
  <StudioSidebarEntry
    class="macro-entry"
    title="宏管理"
    :meta="macroCountLabel"
    icon="pi pi-code"
    aria-label="打开宏管理"
    action-title="打开宏编辑器"
    action-icon="pi pi-pen-to-square"
    @open="openEditor"
  />
</template>
