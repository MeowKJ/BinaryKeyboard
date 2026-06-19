<template>
  <div class="panel actions-panel">
    <h3 class="panel-title">
      <i class="pi pi-cog"></i>
      操作
    </h3>
    <div class="action-buttons">
      <Button v-if="showResetButton" label="恢复出厂" icon="pi pi-refresh" severity="danger" outlined @click="$emit('reset')"
        class="action-btn btn-danger-outline" />
    </div>
  </div>
</template>

<script setup lang="ts">
defineProps<{
  showResetButton: boolean;
}>();

defineEmits<{
  (e: 'reset'): void;
}>();
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

@media (min-width: 1440px) {
  .action-buttons {
    display: flex;
  }
}
</style>
