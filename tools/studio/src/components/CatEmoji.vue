<script setup lang="ts">
import { computed } from 'vue';
import catFace from '@/assets/emoji/cat-face.png';
import grinningCat from '@/assets/emoji/grinning-cat.png';
import grinningCatEyes from '@/assets/emoji/grinning-cat-eyes.png';
import grinningCatAnimated from '@/assets/emoji/grinning_cat_animated.png';
import grinningCatEyesAnimated from '@/assets/emoji/grinning_cat_with_smiling_eyes_animated.png';
import heartEyesAnimated from '@/assets/emoji/smiling_cat_with_heart-eyes_animated.png';
import cryingAnimated from '@/assets/emoji/crying_cat_animated.png';
import wearyAnimated from '@/assets/emoji/weary_cat_animated.png';
import pooAnimated from '@/assets/emoji/pile_of_poo_animated.png';
import catFace3d from '@/assets/emoji/cat_face_3d.png';
import catFaceAnimated from '@/assets/emoji/cat_face_animated.png';
import rocket3d from '@/assets/emoji/rocket_3d.png';
import rocketAnimated from '@/assets/emoji/rocket_animated.png';
import hourglassDone3d from '@/assets/emoji/hourglass_done_3d.png';
import hourglassDoneAnimated from '@/assets/emoji/hourglass_done_animated.png';
import hourglassNotDone3d from '@/assets/emoji/hourglass_not_done_3d.png';
import hourglassNotDoneAnimated from '@/assets/emoji/hourglass_not_done_animated.png';

export type CatEmojiType =
  | 'cat'
  | 'grinning'
  | 'grinning-eyes'
  | 'grinning-animated'
  | 'grinning-eyes-animated'
  | 'heart-eyes-animated'
  | 'crying-animated'
  | 'weary-animated'
  | 'poo-animated'
  | 'cat-face-3d'
  | 'cat-face-animated'
  | 'rocket-3d'
  | 'rocket-animated'
  | 'hourglass-done-3d'
  | 'hourglass-done-animated'
  | 'hourglass-not-done-3d'
  | 'hourglass-not-done-animated';

const props = withDefaults(defineProps<{
  type?: CatEmojiType
}>(), {
  type: 'cat'
});

const src = computed(() => {
  switch (props.type) {
    case 'grinning': return grinningCat;
    case 'grinning-eyes': return grinningCatEyes;
    case 'grinning-animated': return grinningCatAnimated;
    case 'grinning-eyes-animated': return grinningCatEyesAnimated;
    case 'heart-eyes-animated': return heartEyesAnimated;
    case 'crying-animated': return cryingAnimated;
    case 'weary-animated': return wearyAnimated;
    case 'poo-animated': return pooAnimated;
    case 'cat-face-3d': return catFace3d;
    case 'cat-face-animated': return catFaceAnimated;
    case 'rocket-3d': return rocket3d;
    case 'rocket-animated': return rocketAnimated;
    case 'hourglass-done-3d': return hourglassDone3d;
    case 'hourglass-done-animated': return hourglassDoneAnimated;
    case 'hourglass-not-done-3d': return hourglassNotDone3d;
    case 'hourglass-not-done-animated': return hourglassNotDoneAnimated;
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
