<script setup lang="ts">
import { computed, onMounted, ref, watch } from 'vue';
import { listConfigItems } from '@/services/configLibraryService';
import type { ConfigLibraryItem } from '@/types/configBackup';
import StudioSidebarEntry from '@/components/StudioSidebarEntry.vue';

const props = defineProps<{
  refreshKey?: number;
}>();

const emit = defineEmits<{
  (e: 'open'): void;
}>();

const configs = ref<ConfigLibraryItem[]>([]);

const latestName = computed(() => configs.value[0]?.name ?? '暂无配置');
const configCountLabel = computed(() => `${configs.value.length} 套配置`);

async function refreshCount() {
  configs.value = await listConfigItems();
}

onMounted(refreshCount);
watch(() => props.refreshKey, refreshCount);
</script>

<template>
  <StudioSidebarEntry
    class="config-library-entry"
    title="配置库"
    :meta="`${configCountLabel} · ${latestName}`"
    icon="pi pi-objects-column"
    aria-label="打开配置库"
    action-title="打开配置库"
    action-icon="pi pi-folder-open"
    @open="emit('open')"
  />
</template>
