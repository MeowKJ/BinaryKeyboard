<script setup lang="ts">
import { computed } from 'vue';
import catFace from '@/assets/emoji/cat-face.png';
import grinningCat from '@/assets/emoji/grinning-cat.png';
import grinningCatEyes from '@/assets/emoji/grinning-cat-eyes.png';

const props = withDefaults(defineProps<{
  type?: 'cat' | 'grinning' | 'grinning-eyes'
}>(), {
  type: 'cat'
});

const src = computed(() => {
  switch (props.type) {
    case 'grinning': return grinningCat;
    case 'grinning-eyes': return grinningCatEyes;
    default: return catFace;
  }
});
</script>

<template>
  <img :src="src" class="cat-emoji" alt="" draggable="false" />
</template>

<style scoped>
.cat-emoji {
  width: 1em;
  height: 1em;
  vertical-align: -0.1em;
  display: inline-block;
  pointer-events: none;
}
</style>
