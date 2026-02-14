<template>
  <Teleport to="body">
    <div v-if="visible" class="intro-overlay" ref="overlayRef"
      :style="{ background: bgColor }">
      <!-- 背景粒子 -->
      <canvas ref="particleCanvas" class="particle-canvas"></canvas>

      <!-- 中央舞台 -->
      <div class="intro-stage">
        <!-- 主 SVG Logo - 纯色线条描边 -->
        <svg class="logo-svg" viewBox="0 0 1000 100" xmlns="http://www.w3.org/2000/svg">
          <!-- 底层：描边动画 -->
          <text ref="textMain" class="logo-text-stroke"
            x="500" y="74" text-anchor="middle"
            font-family="'Quicksand', sans-serif" font-size="82" font-weight="700"
            fill="none" :stroke="strokeColor" stroke-width="1.2"
            stroke-linejoin="round" stroke-linecap="round">
            BinaryKeyboard
          </text>
          <!-- 上层：与背景同色填充，遮盖描边内部三角形伪影 -->
          <text class="logo-text-mask"
            x="500" y="74" text-anchor="middle"
            font-family="'Quicksand', sans-serif" font-size="82" font-weight="700"
            :fill="bgColor">
            BinaryKeyboard
          </text>
        </svg>

        <!-- 通讯状态 -->
        <div class="connecting-hint" ref="connectingHint">
          <div class="connecting-dots">
            <span class="dot"></span>
            <span class="dot"></span>
            <span class="dot"></span>
          </div>
          <span class="connecting-text" :style="{ color: textColor }">通讯中</span>
        </div>
      </div>
    </div>
  </Teleport>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted, nextTick } from 'vue';
import gsap from 'gsap';

const emit = defineEmits<{ done: [] }>();

const visible = ref(true);
const overlayRef = ref<HTMLElement>();
const textMain = ref<SVGTextElement>();
const connectingHint = ref<HTMLElement>();
const particleCanvas = ref<HTMLCanvasElement>();

let animFrameId: number | null = null;

// 主题检测（挂载时读取一次，动画期间不会切换）
const isDark = ref(true);
const bgColor = ref('#0a0a0f');
const strokeColor = ref('rgba(255,255,255,0.85)');
const textColor = ref('rgba(255,255,255,0.4)');
const lineAlpha = ref(0.06);

function detectTheme() {
  isDark.value = document.documentElement.getAttribute('data-theme') !== 'light';
  if (isDark.value) {
    bgColor.value = '#0a0a0f';
    strokeColor.value = 'rgba(255,255,255,0.85)';
    textColor.value = 'rgba(255,255,255,0.4)';
    lineAlpha.value = 0.06;
  } else {
    bgColor.value = '#f0f2f5';
    strokeColor.value = 'rgba(30,30,60,0.8)';
    textColor.value = 'rgba(30,30,60,0.4)';
    lineAlpha.value = 0.12;
  }
}

// 粒子系统
function initParticles(canvas: HTMLCanvasElement) {
  const ctx = canvas.getContext('2d')!;
  canvas.width = window.innerWidth;
  canvas.height = window.innerHeight;

  const dark = isDark.value;
  const linkAlpha = lineAlpha.value;

  const particles: { x: number; y: number; vx: number; vy: number; size: number; alpha: number; color: string }[] = [];

  for (let i = 0; i < 50; i++) {
    particles.push({
      x: Math.random() * canvas.width,
      y: Math.random() * canvas.height,
      vx: (Math.random() - 0.5) * 0.3,
      vy: (Math.random() - 0.5) * 0.3,
      size: Math.random() * 1.5 + 0.5,
      alpha: Math.random() * 0.3 + 0.05,
      color: Math.random() > 0.5 ? '#8b5cf6' : '#06b6d4',
    });
  }

  function draw() {
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    for (let i = 0; i < particles.length; i++) {
      for (let j = i + 1; j < particles.length; j++) {
        const dx = particles[i].x - particles[j].x;
        const dy = particles[i].y - particles[j].y;
        const dist = Math.sqrt(dx * dx + dy * dy);
        if (dist < 120) {
          ctx.beginPath();
          const a = linkAlpha * (1 - dist / 120);
          ctx.strokeStyle = dark
            ? `rgba(139, 92, 246, ${a})`
            : `rgba(100, 60, 200, ${a})`;
          ctx.lineWidth = 0.5;
          ctx.moveTo(particles[i].x, particles[i].y);
          ctx.lineTo(particles[j].x, particles[j].y);
          ctx.stroke();
        }
      }
    }

    for (const p of particles) {
      p.x += p.vx;
      p.y += p.vy;
      if (p.x < 0 || p.x > canvas.width) p.vx *= -1;
      if (p.y < 0 || p.y > canvas.height) p.vy *= -1;

      ctx.beginPath();
      ctx.arc(p.x, p.y, p.size, 0, Math.PI * 2);
      ctx.globalAlpha = p.alpha;
      ctx.fillStyle = p.color;
      ctx.fill();
      ctx.globalAlpha = 1;
    }

    animFrameId = requestAnimationFrame(draw);
  }

  draw();
}

// SVG 描边初始化
function setupStroke(el: SVGTextElement) {
  const len = el.getComputedTextLength();
  el.style.strokeDasharray = `${len}`;
  el.style.strokeDashoffset = `${len}`;
}

onMounted(async () => {
  detectTheme();
  await nextTick();

  if (particleCanvas.value) {
    initParticles(particleCanvas.value);
  }

  if (textMain.value) setupStroke(textMain.value);

  const tl = gsap.timeline();

  // 1. "通讯中" 先淡入
  tl.fromTo(connectingHint.value!, {
    opacity: 0,
    y: 10,
  }, {
    opacity: 1,
    y: 0,
    duration: 0.4,
    ease: 'power2.out',
  }, 0);

  // 2. 线条描边超慢绘制（power2.in = 越画越快，结尾干脆）
  tl.to(textMain.value!, {
    strokeDashoffset: 0,
    duration: 5,
    ease: 'power2.in',
  }, 0.3);

  // 3. 淡出完成即进入主界面（不等时间线结束）
  tl.to([connectingHint.value!, overlayRef.value!], {
    opacity: 0,
    duration: 0.6,
    ease: 'power2.in',
    onComplete: () => {
      tl.kill();
      visible.value = false;
      emit('done');
    },
  }, 3.8);
});

onUnmounted(() => {
  if (animFrameId) cancelAnimationFrame(animFrameId);
});
</script>

<style scoped>
.intro-overlay {
  position: fixed;
  inset: 0;
  z-index: 99999;
  display: flex;
  align-items: center;
  justify-content: center;
  overflow: hidden;
}

.particle-canvas {
  position: absolute;
  inset: 0;
  width: 100%;
  height: 100%;
  pointer-events: none;
}

.intro-stage {
  position: relative;
  z-index: 2;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 2rem;
}

/* SVG Logo */
.logo-svg {
  width: 95vw;
  height: auto;
}

.logo-text-stroke {
  will-change: stroke-dashoffset;
}

/* 通讯状态 */
.connecting-hint {
  display: flex;
  align-items: center;
  gap: 0.6rem;
  opacity: 0;
}

.connecting-text {
  font-family: 'Nunito', 'Inter', sans-serif;
  font-size: 0.85rem;
  font-weight: 500;
  letter-spacing: 0.15em;
}

.connecting-dots {
  display: flex;
  gap: 4px;
}

.dot {
  width: 4px;
  height: 4px;
  border-radius: 50%;
  background: rgba(139, 92, 246, 0.6);
  animation: dotPulse 1.4s ease-in-out infinite;
}

.dot:nth-child(2) {
  animation-delay: 0.2s;
}

.dot:nth-child(3) {
  animation-delay: 0.4s;
}

@keyframes dotPulse {
  0%, 80%, 100% {
    opacity: 0.3;
    transform: scale(0.8);
  }
  40% {
    opacity: 1;
    transform: scale(1.2);
  }
}
</style>
