<script setup lang="ts">
import { computed, ref, watch } from 'vue';
import { lastToastSeverity } from '@/services/toastService';

import idleImg from '@/assets/emoji/grinning_cat_with_smiling_eyes_animated.png';
import successImg from '@/assets/emoji/smiling_cat_with_heart-eyes_animated.png';
import errorImg from '@/assets/emoji/crying_cat_animated.png';
import warnImg from '@/assets/emoji/weary_cat_animated.png';
import infoImg from '@/assets/emoji/grinning_cat_animated.png';

const props = defineProps<{
  loading?: boolean;
}>();

const bounce = ref(false);

const currentImg = computed(() => {
  if (props.loading) return idleImg;
  switch (lastToastSeverity.value) {
    case 'success': return successImg;
    case 'error': return errorImg;
    case 'warn': return warnImg;
    case 'info': return infoImg;
    default: return idleImg;
  }
});

watch(lastToastSeverity, (v) => {
  if (v && !props.loading) {
    bounce.value = true;
    setTimeout(() => { bounce.value = false; }, 500);
  }
});
</script>

<template>
  <div class="cat-assistant" :class="{ bounce, loading }">
    <img :src="currentImg" alt="猫咪助手" draggable="false" />
    <span v-if="loading" class="cat-loading-text">加载中...</span>
  </div>
</template>

<style scoped>
.cat-assistant {
  position: fixed;
  bottom: 52px;
  right: 24px;
  width: 64px;
  height: 64px;
  z-index: 100;
  cursor: default;
  transition: transform 0.3s ease;
  filter: drop-shadow(0 2px 8px rgba(0, 0, 0, 0.22));
}

.cat-assistant img {
  width: 100%;
  height: 100%;
  object-fit: contain;
  pointer-events: none;
}

.cat-assistant.bounce {
  animation: cat-bounce 0.5s ease;
}

.cat-assistant.loading {
  animation: cat-breathe 1.5s ease-in-out infinite;
}

.cat-loading-text {
  position: absolute;
  bottom: -18px;
  left: 50%;
  transform: translateX(-50%);
  font-size: 0.65rem;
  font-weight: 600;
  color: var(--c-text-muted);
  white-space: nowrap;
  animation: loadingPulse 1.5s ease-in-out infinite;
}

@keyframes cat-bounce {
  0%, 100% { transform: scale(1); }
  30% { transform: scale(1.3); }
  60% { transform: scale(0.9); }
}

@keyframes cat-breathe {
  0%, 100% { transform: scale(1); }
  50% { transform: scale(1.12); }
}

@keyframes loadingPulse {
  0%, 100% { opacity: 0.4; }
  50% { opacity: 1; }
}
</style>
