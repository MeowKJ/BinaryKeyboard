<script setup lang="ts">
import { ref, onMounted } from 'vue';

const props = defineProps<{
  src: string;
}>();

const videoRef = ref<HTMLVideoElement | null>(null);
const loaded = ref(false);

onMounted(() => {
  // Lazy: 只在组件挂载后开始加载视频
  if (videoRef.value) {
    videoRef.value.src = props.src;
    videoRef.value.addEventListener('canplay', () => { loaded.value = true; }, { once: true });
    videoRef.value.load();
  }
});
</script>

<template>
  <div class="video-bg" :class="{ loaded }">
    <video
      ref="videoRef"
      autoplay
      loop
      muted
      playsinline
      preload="none"
      crossorigin="anonymous"
    ></video>
  </div>
</template>

<style scoped>
.video-bg {
  position: fixed;
  inset: 0;
  z-index: -1;
  overflow: hidden;
  opacity: 0;
  transition: opacity 1s ease;
}

.video-bg.loaded {
  opacity: 1;
}

.video-bg video {
  width: 100%;
  height: 100%;
  object-fit: cover;
}
</style>
