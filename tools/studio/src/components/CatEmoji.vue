<script setup lang="ts">
import { computed, ref, watch } from 'vue';
import { getStudioEmoji, type StudioEmojiType } from '@/utils/fluentEmoji';

const props = withDefaults(defineProps<{
  type?: StudioEmojiType;
  alt?: string;
}>(), {
  type: 'cat',
  alt: '',
});

const loadFailed = ref(false);

const emoji = computed(() => getStudioEmoji(props.type));

watch(() => props.type, () => {
  loadFailed.value = false;
});

function onLoadError() {
  loadFailed.value = true;
}
</script>

<template>
  <img
    v-if="!loadFailed"
    :src="emoji.src"
    class="cat-emoji"
    :alt="alt"
    draggable="false"
    decoding="async"
    @error="onLoadError"
  />
  <span
    v-else
    class="cat-emoji cat-emoji-fallback"
    :role="alt ? 'img' : undefined"
    :aria-label="alt || undefined"
    :aria-hidden="alt ? undefined : true"
  >{{ emoji.fallback }}</span>
</template>

<style scoped>
.cat-emoji {
  width: 1em;
  height: 1em;
  vertical-align: -0.1em;
  display: inline-block;
  pointer-events: none;
  object-fit: contain;
}

.cat-emoji-fallback {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  line-height: 1;
}
</style>
