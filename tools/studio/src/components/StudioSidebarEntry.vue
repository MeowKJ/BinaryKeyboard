<script setup lang="ts">
withDefaults(defineProps<{
  title: string;
  meta: string;
  icon: string;
  ariaLabel?: string;
  actionTitle?: string;
  actionIcon?: string;
}>(), {
  actionIcon: 'pi pi-arrow-right',
});

const emit = defineEmits<{
  (e: 'open'): void;
}>();
</script>

<template>
  <div
    class="panel sidebar-entry"
    role="button"
    tabindex="0"
    :aria-label="ariaLabel ?? title"
    @click="emit('open')"
    @keydown.enter.prevent="emit('open')"
    @keydown.space.prevent="emit('open')"
  >
    <div class="entry-icon" aria-hidden="true">
      <i :class="icon"></i>
    </div>

    <div class="entry-copy">
      <strong>{{ title }}</strong>
      <code>{{ meta }}</code>
    </div>

    <slot name="action">
      <button type="button" class="entry-open" :title="actionTitle ?? title" @click.stop="emit('open')">
        <i :class="actionIcon"></i>
      </button>
    </slot>
  </div>
</template>
