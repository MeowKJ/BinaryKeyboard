<script setup lang="ts">
import { onMounted, onUnmounted, ref } from 'vue';

const props = withDefaults(defineProps<{
  type?: 'sakura' | 'stars';
  count?: number;
}>(), {
  type: 'sakura',
  count: 20,
});

const container = ref<HTMLElement>();

interface Particle {
  el: HTMLElement;
  x: number;
  y: number;
  size: number;
  speed: number;
  wobble: number;
  wobbleSpeed: number;
  rotation: number;
  rotationSpeed: number;
  opacity: number;
}

let particles: Particle[] = [];
let animId = 0;

function createParticle(i: number): Particle {
  const el = document.createElement('div');
  el.className = `particle particle-${props.type}`;
  el.innerHTML = props.type === 'sakura' ? '🌸' : '✦';

  const size = 10 + Math.random() * 14;
  el.style.fontSize = `${size}px`;
  el.style.position = 'absolute';
  el.style.pointerEvents = 'none';
  el.style.userSelect = 'none';
  el.style.willChange = 'transform, opacity';

  return {
    el,
    x: Math.random() * 100,
    y: -10 - Math.random() * 30,
    size,
    speed: 0.15 + Math.random() * 0.25,
    wobble: Math.random() * Math.PI * 2,
    wobbleSpeed: 0.01 + Math.random() * 0.02,
    rotation: Math.random() * 360,
    rotationSpeed: (Math.random() - 0.5) * 2,
    opacity: 0.3 + Math.random() * 0.4,
  };
}

function animate() {
  for (const p of particles) {
    p.y += p.speed;
    p.wobble += p.wobbleSpeed;
    p.rotation += p.rotationSpeed;

    const wx = Math.sin(p.wobble) * 30;

    if (p.y > 110) {
      p.y = -10;
      p.x = Math.random() * 100;
    }

    p.el.style.left = `calc(${p.x}% + ${wx}px)`;
    p.el.style.top = `${p.y}%`;
    p.el.style.opacity = String(p.opacity);
    p.el.style.transform = `rotate(${p.rotation}deg)`;
  }
  animId = requestAnimationFrame(animate);
}

onMounted(() => {
  if (!container.value) return;
  for (let i = 0; i < props.count; i++) {
    const p = createParticle(i);
    p.y = Math.random() * 100; // 初始随机分布
    container.value.appendChild(p.el);
    particles.push(p);
  }
  animate();
});

onUnmounted(() => {
  cancelAnimationFrame(animId);
  particles = [];
});
</script>

<template>
  <div ref="container" class="falling-effect"></div>
</template>

<style scoped>
.falling-effect {
  position: fixed;
  inset: 0;
  pointer-events: none;
  z-index: 0;
  overflow: hidden;
}
</style>
