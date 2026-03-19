<script setup lang="ts">
import { computed, ref, watch, onMounted, onUnmounted } from 'vue';
import { lastToastSeverity } from '@/services/toastService';

import idleImg from '@/assets/emoji/grinning_cat_with_smiling_eyes_animated.png';
import successImg from '@/assets/emoji/smiling_cat_with_heart-eyes_animated.png';
import errorImg from '@/assets/emoji/crying_cat_animated.png';
import warnImg from '@/assets/emoji/weary_cat_animated.png';
import infoImg from '@/assets/emoji/grinning_cat_animated.png';

const props = defineProps<{
  loading?: boolean;
}>();

const emit = defineEmits<{
  (e: 'action', action: string): void;
}>();

const bounce = ref(false);
const menuOpen = ref(false);

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

function toggleMenu() {
  if (props.loading) return;
  menuOpen.value = !menuOpen.value;
}

function onAction(action: string) {
  menuOpen.value = false;
  emit('action', action);
}

function onClickOutside(e: MouseEvent) {
  const el = (e.target as HTMLElement).closest('.cat-assistant');
  if (!el) menuOpen.value = false;
}

onMounted(() => document.addEventListener('click', onClickOutside));
onUnmounted(() => document.removeEventListener('click', onClickOutside));
</script>

<template>
  <div class="cat-assistant" :class="{ bounce, loading }" @click.stop="toggleMenu">
    <img :src="currentImg" alt="猫咪助手" draggable="false" />
    <span v-if="loading" class="cat-loading-text">加载中...</span>

    <transition name="cat-menu">
      <div v-if="menuOpen" class="cat-menu">
        <button class="cat-menu-item" @click.stop="onAction('refresh')">
          <i class="pi pi-sync"></i>
          <span>刷新配置</span>
        </button>
        <button class="cat-menu-item" @click.stop="onAction('theme')">
          <i class="pi pi-palette"></i>
          <span>切换主题</span>
        </button>
        <button class="cat-menu-item" @click.stop="onAction('scrollTop')">
          <i class="pi pi-arrow-up"></i>
          <span>回到顶部</span>
        </button>
        <div class="cat-menu-divider"></div>
        <button class="cat-menu-item danger" @click.stop="onAction('disconnect')">
          <i class="pi pi-power-off"></i>
          <span>断开连接</span>
        </button>
      </div>
    </transition>
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
  cursor: pointer;
  transition: transform 0.3s ease;
  filter: drop-shadow(0 2px 8px rgba(0, 0, 0, 0.22));
}

.cat-assistant:hover {
  transform: scale(1.1);
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
  cursor: default;
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

/* 弹出菜单 */
.cat-menu {
  position: absolute;
  bottom: calc(100% + 8px);
  right: 0;
  min-width: 140px;
  background: var(--c-bg-secondary);
  border: 1px solid var(--c-border);
  border-radius: var(--radius-md, 12px);
  padding: 0.35rem;
  box-shadow: 0 8px 24px rgba(0, 0, 0, 0.25);
}

.cat-menu-item {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  width: 100%;
  padding: 0.5rem 0.65rem;
  font-size: 0.8rem;
  font-weight: 600;
  font-family: inherit;
  color: var(--c-text-secondary);
  background: none;
  border: none;
  border-radius: var(--radius-sm, 8px);
  cursor: pointer;
  transition: background 0.15s ease;
  white-space: nowrap;
}

.cat-menu-item:hover {
  background: var(--c-bg-hover, rgba(255,255,255,0.06));
}

.cat-menu-item i {
  font-size: 0.85rem;
  color: var(--c-accent);
  width: 1.1rem;
  text-align: center;
}

.cat-menu-item.danger {
  color: var(--c-error, #ef4444);
}

.cat-menu-item.danger i {
  color: var(--c-error, #ef4444);
}

.cat-menu-divider {
  height: 1px;
  background: var(--c-border);
  margin: 0.25rem 0.5rem;
}

/* 菜单动画 */
.cat-menu-enter-active {
  transition: all 0.2s cubic-bezier(0.22, 1, 0.36, 1);
}

.cat-menu-leave-active {
  transition: all 0.15s ease;
}

.cat-menu-enter-from,
.cat-menu-leave-to {
  opacity: 0;
  transform: translateY(8px) scale(0.95);
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
