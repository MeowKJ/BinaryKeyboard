<script setup lang="ts">
import { computed } from 'vue';
import CatEmoji from '@/components/CatEmoji.vue';
import { useTheme } from '@/composables/useTheme';

const { themeId } = useTheme();

// 荷叶配置：大小/位置/旋转/缺口角度各不相同
const lilypads = [
  { size: 180, top: '-30px', right: '5%', rotate: 25, notch: 30 },
  { size: 120, bottom: '8%', left: '8%', rotate: -20, notch: 55 },
  { size: 80, top: '48%', right: '8%', rotate: 110, notch: 10 },
  { size: 150, top: '15%', right: '60%', rotate: -45, notch: 75 },
  { size: 65, bottom: '30%', right: '20%', rotate: 150, notch: 40 },
  { size: 100, top: '68%', left: '35%', rotate: 200, notch: 65 },
  { size: 50, top: '35%', right: '35%', rotate: -80, notch: 20 },
];

// 水波纹：每次切换到 frog 时重新生成随机值
let rippleSeed = 0;
function makeRipples() {
  rippleSeed++;
  return Array.from({ length: 6 }, (_, i) => ({
    id: `rip-${rippleSeed}-${i}`,
    size: 120 + Math.random() * 250,
    x: 5 + Math.random() * 90,
    y: 5 + Math.random() * 90,
    dur: 4 + Math.random() * 6,
    delay: i * 1.2 + Math.random() * 3,
  }));
}

const ripples = computed(() => {
  // 依赖 themeId，当切换到 frog 时会重新计算
  if (themeId.value === 'frog') return makeRipples();
  return [];
});
</script>

<template>
  <div class="keyboard-decoration">
    <!-- 默认主题：干净无装饰 -->
    <template v-if="themeId === 'default'">
      <!-- 空 -->
    </template>

    <!-- 猫咪主题：猫咪元素背景 -->
    <template v-else-if="themeId === 'neko'">
      <div class="deco-circle deco-1"></div>
      <div class="deco-circle deco-2"></div>
      <div class="deco-circle deco-3"></div>
      <span class="paw-walk paw-w1">🐾</span>
      <span class="paw-walk paw-w2">🐾</span>
      <span class="paw-walk paw-w3">🐾</span>
      <span class="paw-walk paw-w4">🐾</span>
      <span class="paw-walk paw-w5">🐾</span>
      <span class="paw-walk paw-w6">🐾</span>
      <span class="paw-walk paw-w7">🐾</span>
      <span class="paw-walk paw-w8">🐾</span>
      <span class="paw-walk paw-w9">🐾</span>
      <div class="deco-star deco-star-1">✨</div>
      <div class="deco-star deco-star-2">✨</div>
      <span class="deco-emoji deco-icon-1"><CatEmoji type="grinning-animated" /></span>
      <span class="deco-emoji deco-icon-2"><CatEmoji type="grinning-eyes-animated" /></span>
      <span class="deco-emoji deco-icon-3"><CatEmoji type="heart-eyes-animated" /></span>
      <span class="deco-mini deco-yarn">🧶</span>
      <span class="deco-mini deco-star-3">⭐</span>
    </template>

    <!-- 青蛙主题：荷叶 + 水波纹 + 🐸 -->
    <template v-else-if="themeId === 'frog'">
      <!-- 水波纹层（随机位置） -->
      <div
        v-for="r in ripples"
        :key="r.id"
        class="water-ripple"
        :style="{
          width: r.size + 'px',
          height: r.size + 'px',
          left: r.x + '%',
          top: r.y + '%',
          animationDuration: r.dur + 's',
          animationDelay: r.delay + 's',
        }"
      ></div>

      <!-- 荷叶 -->
      <div
        v-for="(lp, i) in lilypads"
        :key="i"
        class="lilypad"
        :style="{
          width: lp.size + 'px',
          height: lp.size + 'px',
          top: lp.top ?? 'auto',
          right: lp.right ?? 'auto',
          left: lp.left ?? 'auto',
          bottom: lp.bottom ?? 'auto',
          '--lp-rotate': lp.rotate + 'deg',
          '--lp-notch': lp.notch + 'deg',
          animationDelay: (i * 1.3) + 's',
          animationDuration: (7 + i * 1.5) + 's',
        } as any"
      ></div>

      <!-- 青蛙坐在荷叶上（动画版） -->
      <span class="frog-deco frog-1"><CatEmoji type="frog-animated" /></span>
      <span class="frog-deco frog-2"><CatEmoji type="frog-animated" /></span>
      <div class="deco-star deco-star-1">✨</div>
      <div class="deco-star deco-star-2">✨</div>
    </template>

    <!-- 琉璃主题：简洁星星 -->
    <template v-else-if="themeId === 'liuli'">
      <div class="deco-star deco-star-1">✨</div>
      <div class="deco-star deco-star-2">✨</div>
      <div class="deco-circle deco-1"></div>
      <div class="deco-circle deco-2"></div>
    </template>

    <!-- 安哥拉兔主题：简洁 -->
    <template v-else-if="themeId === 'angora'">
      <div class="deco-star deco-star-1">✨</div>
      <div class="deco-star deco-star-2">✨</div>
    </template>
  </div>
</template>

<style scoped>
.keyboard-decoration {
  position: absolute;
  inset: 0;
  pointer-events: none;
  overflow: visible; /* 允许荷叶超出 */
}

/* === 圆形装饰 === */
.deco-circle {
  position: absolute;
  border-radius: 50%;
  opacity: 0.1;
  background: var(--c-accent);
}

.deco-1 { width: 300px; height: 300px; top: -100px; right: -50px; animation: float 8s ease-in-out infinite; }
.deco-2 { width: 180px; height: 180px; top: 60%; left: 0; animation: float 6s ease-in-out infinite reverse; }
.deco-3 { width: 120px; height: 120px; top: 40%; right: 3%; animation: float 10s ease-in-out infinite; }

/* === 猫爪脚印 === */
.paw-walk { position: absolute; font-size: 1rem; opacity: 0; }
.paw-w1 { bottom: 20%; left: 14%; --paw-angle: 22deg; animation: pawStep 18s ease-in-out infinite 0s; }
.paw-w2 { bottom: 24%; left: 18%; --paw-angle: 28deg; animation: pawStep 18s ease-in-out infinite 1.2s; }
.paw-w3 { bottom: 29%; left: 23%; --paw-angle: 38deg; animation: pawStep 18s ease-in-out infinite 2.4s; }
.paw-w4 { bottom: 33%; left: 28%; --paw-angle: 52deg; animation: pawStep 18s ease-in-out infinite 3.6s; }
.paw-w5 { bottom: 35%; left: 34%; --paw-angle: 72deg; animation: pawStep 18s ease-in-out infinite 4.8s; }
.paw-w6 { bottom: 37%; left: 40%; --paw-angle: 58deg; animation: pawStep 18s ease-in-out infinite 6s; }
.paw-w7 { bottom: 42%; left: 45%; --paw-angle: 40deg; animation: pawStep 18s ease-in-out infinite 7.2s; }
.paw-w8 { bottom: 48%; left: 50%; --paw-angle: 28deg; animation: pawStep 18s ease-in-out infinite 8.4s; }
.paw-w9 { bottom: 54%; left: 55%; --paw-angle: 22deg; animation: pawStep 18s ease-in-out infinite 9.6s; }

@keyframes pawStep {
  0% { opacity: 0; transform: rotate(var(--paw-angle)) scale(0.5); }
  5% { opacity: 0.32; transform: rotate(var(--paw-angle)) scale(1); }
  18% { opacity: 0.28; transform: rotate(var(--paw-angle)) scale(1); }
  28% { opacity: 0; transform: rotate(var(--paw-angle)) scale(0.8); }
  100% { opacity: 0; transform: rotate(var(--paw-angle)) scale(0.8); }
}

/* === 星星闪烁 === */
.deco-star { position: absolute; font-size: 1.5rem; opacity: 0.2; animation: twinkle 2s ease-in-out infinite; }
.deco-star-1 { top: 25%; right: 20%; }
.deco-star-2 { bottom: 30%; left: 15%; animation-delay: 1s; }

/* === Emoji 装饰 === */
.deco-emoji { position: absolute; font-size: 3rem; opacity: 0.25; user-select: none; pointer-events: none; }
.deco-icon-1 { top: 8%; left: 3%; font-size: 3.5rem; animation: emojiFloat 4s ease-in-out infinite; }
.deco-icon-2 { bottom: 12%; right: 6%; font-size: 3rem; animation: emojiFloat 5s ease-in-out infinite 1s; }
.deco-icon-3 { top: 50%; right: 5%; font-size: 2.5rem; opacity: 0.18; animation: emojiFloat 6s ease-in-out infinite 0.5s; }

.deco-emoji :deep(.cat-emoji) { width: 1em; height: 1em; }

.deco-mini { position: absolute; opacity: 0.2; user-select: none; pointer-events: none; }
.deco-yarn { font-size: 1.3rem; top: 65%; left: 3%; animation: yarnRoll 5s ease-in-out infinite; }
.deco-star-3 { font-size: 1.2rem; top: 30%; right: 5%; animation: twinkle 2s ease-in-out infinite 0.3s; }

/* ============================================
   水波纹 — 同心圆扩散效果
============================================ */
.water-ripple {
  position: absolute;
  border-radius: 50%;
  border: 1px solid rgba(100, 200, 255, 0.08);
  pointer-events: none;
  opacity: 0;
  animation: rippleExpand ease-out infinite;
  transform: translate(-50%, -50%);
}

@keyframes rippleExpand {
  0% {
    transform: translate(-50%, -50%) scale(0.3);
    opacity: 0;
    border-width: 2px;
    border-color: rgba(100, 200, 255, 0.18);
  }
  15% {
    opacity: 0.6;
  }
  100% {
    transform: translate(-50%, -50%) scale(2.5);
    opacity: 0;
    border-width: 1px;
    border-color: rgba(100, 200, 255, 0);
  }
}

/* ============================================
   荷叶 — 纯圆形 + conic-gradient 小缺口
============================================ */
.lilypad {
  position: absolute;
  border-radius: 50%;
  background: radial-gradient(
    circle at 45% 40%,
    rgba(34, 180, 80, 0.22) 0%,
    rgba(22, 140, 60, 0.16) 50%,
    rgba(16, 100, 45, 0.08) 80%,
    transparent 100%
  );
  transform: rotate(var(--lp-rotate, 0deg));
  animation: lilyFloat ease-in-out infinite;
  /* 圆形 + 窄缺口（约15度弧） */
  -webkit-mask-image: conic-gradient(
    from var(--lp-notch, 45deg) at 50% 50%,
    transparent 0deg 12deg,
    black 14deg 360deg
  );
  mask-image: conic-gradient(
    from var(--lp-notch, 45deg) at 50% 50%,
    transparent 0deg 12deg,
    black 14deg 360deg
  );
}

/* 青蛙装饰 */
.frog-deco {
  position: absolute;
  font-size: 2.8rem;
  opacity: 0.7;
  user-select: none;
  pointer-events: none;
  filter: drop-shadow(0 2px 8px rgba(0, 0, 0, 0.35));
}

.frog-deco :deep(.cat-emoji) { width: 1em; height: 1em; }

.frog-1 {
  top: -10px;
  right: 8%;
  font-size: 2.5rem;
  animation: frogBob 3s ease-in-out infinite;
}

.frog-2 {
  bottom: 12%;
  left: 10%;
  font-size: 2rem;
  animation: frogBob 4s ease-in-out infinite 1.5s;
}

@keyframes frogBob {
  0%, 100% { transform: translateY(0) rotate(-3deg); }
  50% { transform: translateY(-8px) rotate(3deg); }
}

@keyframes lilyFloat {
  0%, 100% { transform: rotate(var(--lp-rotate, 0deg)) translateY(0); }
  50% { transform: rotate(var(--lp-rotate, 0deg)) translateY(-8px); }
}

/* === Keyframes === */
@keyframes emojiFloat {
  0%, 100% { transform: translateY(0) rotate(-5deg); }
  50% { transform: translateY(-15px) rotate(5deg); }
}

@keyframes yarnRoll {
  0%, 100% { transform: rotate(0deg); }
  50% { transform: rotate(20deg) translateX(5px); }
}

@keyframes twinkle {
  0%, 100% { opacity: 0.2; transform: scale(1); }
  50% { opacity: 0.4; transform: scale(1.2); }
}

@keyframes float {
  0%, 100% { transform: translateY(0); }
  50% { transform: translateY(-15px); }
}
</style>
