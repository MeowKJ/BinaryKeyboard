<template>
  <div class="panel actions-panel">
    <h3 class="panel-title">
      <i class="pi pi-cog"></i>
      操作
    </h3>
    <div class="action-buttons">
      <Button :label="saveLabel" icon="pi pi-save" :disabled="!deviceStore.hasChanges" @click="$emit('save')"
        class="action-btn btn-primary" />
      <Button label="放弃更改" icon="pi pi-undo" severity="secondary" :disabled="!deviceStore.hasChanges"
        @click="$emit('discard')" class="action-btn btn-secondary" />
      <Divider v-if="showResetButton" />
      <Button v-if="showResetButton" label="恢复出厂" icon="pi pi-refresh" severity="danger" outlined @click="$emit('reset')"
        class="action-btn btn-danger-outline" />
    </div>
  </div>
</template>

<script setup lang="ts">
import { useDeviceStore } from '@/stores/deviceStore';

defineProps<{
  showResetButton: boolean;
  saveLabel: string;
}>();

defineEmits<{
  (e: 'save'): void;
  (e: 'discard'): void;
  (e: 'reset'): void;
}>();

const deviceStore = useDeviceStore();
</script>

<style scoped>
.action-buttons {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.action-btn {
  width: 100%;
  justify-content: center;
}
</style>
