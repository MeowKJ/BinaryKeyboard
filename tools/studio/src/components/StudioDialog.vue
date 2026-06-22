<script setup lang="ts">
import { computed, useAttrs } from 'vue';
import Dialog from 'primevue/dialog';

defineOptions({ inheritAttrs: false });

const props = withDefaults(defineProps<{
  size?: 'sm' | 'md' | 'lg' | 'immersive';
}>(), {
  size: 'md',
});

const visible = defineModel<boolean>('visible', { default: false });
const attrs = useAttrs();

const dialogClass = computed(() => [
  'studio-dialog',
  `studio-dialog-${props.size}`,
  attrs.class,
]);

const dialogStyle = computed(() => [
  attrs.style,
  { width: `var(--studio-dialog-${props.size})` },
]);

const dialogAttrs = computed(() => {
  const { class: _class, style: _style, ...rest } = attrs;
  return rest;
});
</script>

<template>
  <Dialog
    v-model:visible="visible"
    modal
    dismissable-mask
    v-bind="dialogAttrs"
    :class="dialogClass"
    :style="dialogStyle"
  >
    <template v-if="$slots.header" #header>
      <slot name="header"></slot>
    </template>

    <slot></slot>

    <template v-if="$slots.footer" #footer>
      <slot name="footer"></slot>
    </template>
  </Dialog>
</template>
