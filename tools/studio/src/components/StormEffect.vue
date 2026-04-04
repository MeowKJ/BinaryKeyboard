<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue';

/**
 * 风暴主题全屏效果
 *
 * 天气由实际时间确定性计算（刷新不影响状态）
 * 每个天气阶段约 30 分钟，随机权重转移
 * 月亮独立 10 分钟弧形周期
 */

const canvasRef = ref<HTMLCanvasElement | null>(null);
const flashRef = ref<HTMLDivElement | null>(null);
const loaded = ref(false);

// ── 确定性伪随机 (mulberry32) ──
function mulberry32(seed: number) {
  return () => {
    seed |= 0;
    seed = (seed + 0x6D2B79F5) | 0;
    let t = Math.imul(seed ^ (seed >>> 15), 1 | seed);
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

// ── 天气阶段 ──
type WeatherPhase = 'clear' | 'cloudy' | 'lightning' | 'rain';

const phase = ref<WeatherPhase>('clear');
const phaseProgress = ref(0);

const ALL_PHASES: WeatherPhase[] = ['clear', 'cloudy', 'lightning', 'rain'];

// 30 分钟基础 ±30% 随机
const PHASE_BASE_MS = 30 * 60 * 1000;

const PHASE_TRANSITIONS: Record<WeatherPhase, { next: WeatherPhase; weight: number }[]> = {
  clear:     [{ next: 'cloudy', weight: 6 }, { next: 'lightning', weight: 1 }, { next: 'rain', weight: 1 }],
  cloudy:    [{ next: 'lightning', weight: 5 }, { next: 'rain', weight: 2 }, { next: 'clear', weight: 2 }],
  lightning: [{ next: 'rain', weight: 5 }, { next: 'cloudy', weight: 3 }, { next: 'clear', weight: 1 }],
  rain:      [{ next: 'lightning', weight: 3 }, { next: 'cloudy', weight: 3 }, { next: 'clear', weight: 3 }],
};

function pickNextWithRng(current: WeatherPhase, rng: () => number): WeatherPhase {
  const transitions = PHASE_TRANSITIONS[current];
  const total = transitions.reduce((s, t) => s + t.weight, 0);
  let roll = rng() * total;
  for (const t of transitions) {
    roll -= t.weight;
    if (roll <= 0) return t.next;
  }
  return transitions[transitions.length - 1].next;
}

function getDurationWithRng(rng: () => number): number {
  return PHASE_BASE_MS * (0.7 + rng() * 0.6);
}

// 根据实际时间计算当前天气状态
// 以每天 00:00 UTC 为起点，用当天日期作 seed
function computeWeatherFromTime(nowMs: number): { phase: WeatherPhase; elapsed: number; duration: number } {
  const dayStart = nowMs - (nowMs % (24 * 60 * 60 * 1000)); // 当天 00:00 UTC
  const daySeed = Math.floor(dayStart / 86400000);
  const rng = mulberry32(daySeed);

  // 初始天气也由 seed 决定
  let current: WeatherPhase = ALL_PHASES[Math.floor(rng() * ALL_PHASES.length)];
  let cursor = dayStart;
  let dur = getDurationWithRng(rng);

  // 向前推进到包含 nowMs 的阶段
  while (cursor + dur < nowMs) {
    cursor += dur;
    current = pickNextWithRng(current, rng);
    dur = getDurationWithRng(rng);
  }

  return { phase: current, elapsed: nowMs - cursor, duration: dur };
}

// 当前阶段结束的绝对时间戳
let phaseEndMs = 0;
let currentPhaseDuration = 0;

// ── 阶段主题色 HSL [h, s, l] ──
const PHASE_ACCENT: Record<WeatherPhase, [number, number, number]> = {
  clear: [220, 25, 82],
  cloudy: [215, 15, 62],
  lightning: [220, 10, 38],
  rain: [270, 30, 32],
};

let currentHsl: [number, number, number] = [...PHASE_ACCENT.clear];

function lerpHsl(a: [number, number, number], b: [number, number, number], t: number): [number, number, number] {
  return [a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t];
}

function applyAccentColors(h: number, s: number, l: number) {
  const root = document.documentElement;
  root.style.setProperty('--c-accent', `hsl(${h}, ${s}%, ${l}%)`);
  root.style.setProperty('--c-accent-light', `hsl(${h}, ${s + 5}%, ${Math.min(l + 12, 95)}%)`);
  root.style.setProperty('--c-accent-soft', `hsla(${h}, ${s}%, ${l}%, 0.15)`);
  root.style.setProperty('--c-accent-gradient', `linear-gradient(135deg, hsl(${h}, ${s + 5}%, ${Math.min(l + 12, 95)}%) 0%, hsl(${h}, ${s}%, ${l}%) 100%)`);
}

// ── 月亮弧形运动 ──
const MOON_CYCLE_MS = 10 * 60 * 1000; // 10 分钟一个弧

const moonStyle = computed(() => {
  const moonT = (Date.now() % MOON_CYCLE_MS) / MOON_CYCLE_MS;
  const x = 110 - moonT * 120;
  const yBase = 28;
  const arcHeight = 14;
  const centered = (moonT - 0.5) * 2;
  const yFinal = yBase - arcHeight * (1 - centered * centered);
  return { left: `${x}%`, top: `${yFinal}%` };
});

const moonCloudOpacity = computed(() => {
  switch (phase.value) {
    case 'clear': return 0;
    case 'cloudy': return 0.6;
    case 'lightning': return 0.8;
    case 'rain': return 1;
  }
});

// ── 雨滴 ──
interface RainDrop {
  x: number; y: number; len: number; speed: number; opacity: number;
}

const RAIN_COUNT = 280;
const drops: RainDrop[] = [];

function initDrops(w: number, h: number) {
  drops.length = 0;
  for (let i = 0; i < RAIN_COUNT; i++) drops.push(createDrop(w, h, true));
}

function createDrop(w: number, h: number, randomY = false): RainDrop {
  return {
    x: Math.random() * (w + 200) - 100,
    y: randomY ? Math.random() * h : -Math.random() * 60,
    len: 20 + Math.random() * 32,
    speed: 16 + Math.random() * 12,
    opacity: 0.12 + Math.random() * 0.22,
  };
}

// ── 树叶 ──
interface Leaf {
  x: number; y: number; size: number;
  rotation: number; rotSpeed: number;
  speedX: number; speedY: number;
  wobblePhase: number; wobbleSpeed: number;
  opacity: number; hue: number;
}

const LEAF_COUNT = 18;
const leaves: Leaf[] = [];

function initLeaves(w: number, h: number) {
  leaves.length = 0;
  for (let i = 0; i < LEAF_COUNT; i++) leaves.push(createLeaf(w, h, true));
}

function createLeaf(w: number, h: number, randomPos = false): Leaf {
  return {
    x: randomPos ? Math.random() * w : -20 - Math.random() * 100,
    y: randomPos ? Math.random() * h : Math.random() * h * 0.8,
    size: 4 + Math.random() * 6,
    rotation: Math.random() * Math.PI * 2,
    rotSpeed: (Math.random() - 0.5) * 0.15,
    speedX: 2 + Math.random() * 3,
    speedY: 0.5 + Math.random() * 1.5,
    wobblePhase: Math.random() * Math.PI * 2,
    wobbleSpeed: 0.02 + Math.random() * 0.03,
    opacity: 0.3 + Math.random() * 0.4,
    hue: 30 + Math.random() * 80,
  };
}

function drawLeaf(ctx: CanvasRenderingContext2D, leaf: Leaf) {
  ctx.save();
  ctx.translate(leaf.x, leaf.y);
  ctx.rotate(leaf.rotation);
  ctx.globalAlpha = leaf.opacity;
  ctx.beginPath();
  ctx.ellipse(0, 0, leaf.size, leaf.size * 0.45, 0, 0, Math.PI * 2);
  ctx.fillStyle = `hsl(${leaf.hue}, 40%, 35%)`;
  ctx.fill();
  ctx.beginPath();
  ctx.moveTo(-leaf.size * 0.8, 0);
  ctx.lineTo(leaf.size * 0.8, 0);
  ctx.strokeStyle = `hsla(${leaf.hue}, 30%, 25%, 0.5)`;
  ctx.lineWidth = 0.5;
  ctx.stroke();
  ctx.restore();
}

// ── 风 ──
let windAngle = 0.15;
let windStrength = 1;
let windTargetAngle = 0.15;
let windTargetStrength = 1;
let windChangeTimer = 0;

function updateWind(dt: number) {
  windChangeTimer -= dt;
  if (windChangeTimer <= 0) {
    const p = phase.value;
    if (p === 'cloudy') {
      windTargetAngle = (Math.random() - 0.3) * 0.7;
      windTargetStrength = 2.0 + Math.random() * 2.0;
      windChangeTimer = 1200 + Math.random() * 800;
    } else if (p === 'lightning') {
      if (Math.random() > 0.6) {
        windTargetAngle = (Math.random() - 0.4) * 0.5;
        windTargetStrength = 1.5 + Math.random() * 2.0;
      } else {
        windTargetAngle = (Math.random() - 0.5) * 0.2;
        windTargetStrength = 0.4 + Math.random() * 0.6;
      }
      windChangeTimer = 2000 + Math.random() * 2000;
    } else if (p === 'rain') {
      windTargetAngle = (Math.random() - 0.35) * 0.5;
      windTargetStrength = 1.0 + Math.random() * 1.5;
      windChangeTimer = 1500 + Math.random() * 1500;
    } else {
      windTargetAngle = (Math.random() - 0.5) * 0.1;
      windTargetStrength = 0.2 + Math.random() * 0.3;
      windChangeTimer = 3000 + Math.random() * 2000;
    }
  }
  const lerpRate = Math.min(0.002 * dt, 1);
  windAngle += (windTargetAngle - windAngle) * lerpRate;
  windStrength += (windTargetStrength - windStrength) * lerpRate;
}

// ── 闪电 ──
let flashTimeout: ReturnType<typeof setTimeout> | null = null;

function triggerFlash() {
  if (!flashRef.value) return;
  const el = flashRef.value;
  const roll = Math.random();

  if (roll < 0.12) {
    el.style.opacity = '1';
    setTimeout(() => { el.style.opacity = '0.15'; }, 80);
    setTimeout(() => { el.style.opacity = '0.9'; }, 150);
    setTimeout(() => { el.style.opacity = '0'; }, 250);
    setTimeout(() => { el.style.opacity = '0.7'; }, 400);
    setTimeout(() => { el.style.opacity = '0.1'; }, 480);
    setTimeout(() => { el.style.opacity = '0.5'; }, 550);
    setTimeout(() => { el.style.opacity = '0'; }, 650);
  } else if (roll < 0.4) {
    el.style.opacity = '0.8';
    setTimeout(() => { el.style.opacity = '0'; }, 70);
    setTimeout(() => { el.style.opacity = '0.6'; }, 140);
    setTimeout(() => { el.style.opacity = '0'; }, 220);
    if (Math.random() > 0.3) {
      setTimeout(() => { el.style.opacity = '0.4'; }, 380);
      setTimeout(() => { el.style.opacity = '0'; }, 450);
    }
  } else if (roll < 0.75) {
    el.style.opacity = '0.5';
    setTimeout(() => { el.style.opacity = '0'; }, 60);
    setTimeout(() => { el.style.opacity = '0.3'; }, 120);
    setTimeout(() => { el.style.opacity = '0'; }, 200);
  } else {
    el.style.opacity = '0.2';
    setTimeout(() => { el.style.opacity = '0'; }, 100);
  }
}

function scheduleFlashes() {
  clearFlashSchedule();
  const fire = () => {
    if (phase.value === 'lightning' || phase.value === 'rain') {
      triggerFlash();
      const base = phase.value === 'lightning' ? 800 : 2000;
      const range = phase.value === 'lightning' ? 2500 : 3500;
      flashTimeout = setTimeout(fire, base + Math.random() * range);
    } else {
      if (phase.value === 'cloudy' && Math.random() < 0.3) {
        if (flashRef.value) {
          flashRef.value.style.opacity = '0.12';
          setTimeout(() => { if (flashRef.value) flashRef.value.style.opacity = '0'; }, 120);
        }
      }
      flashTimeout = setTimeout(fire, 3000 + Math.random() * 4000);
    }
  };
  flashTimeout = setTimeout(fire, 500 + Math.random() * 1500);
}

function clearFlashSchedule() {
  if (flashTimeout) { clearTimeout(flashTimeout); flashTimeout = null; }
}

// ── 动画循环 ──
let raf = 0;
let lastTime = 0;
let currentRainIntensity = 0;
let currentLeafIntensity = 0;

function getRainTarget(): number {
  switch (phase.value) {
    case 'clear': return 0;
    case 'cloudy': return 0;
    case 'lightning': return 0.3;
    case 'rain': return 1;
  }
}

function getLeafTarget(): number {
  switch (phase.value) {
    case 'clear': return 0;
    case 'cloudy': return 1;
    case 'lightning': return 0.15;
    case 'rain': return 0.05;
  }
}

function syncWeather() {
  const now = Date.now();
  const state = computeWeatherFromTime(now);
  phase.value = state.phase;
  currentPhaseDuration = state.duration;
  phaseEndMs = now - state.elapsed + state.duration;
  phaseProgress.value = state.elapsed / state.duration;
  // 立即跳到对应主题色
  currentHsl = [...PHASE_ACCENT[state.phase]];
  applyAccentColors(currentHsl[0], currentHsl[1], currentHsl[2]);
}

function tick(time: number) {
  if (!lastTime) lastTime = time;
  const dt = Math.min(time - lastTime, 50);
  lastTime = time;

  // 天气阶段推进
  const now = Date.now();
  if (now >= phaseEndMs) {
    syncWeather();
  } else {
    phaseProgress.value = Math.min(1 - (phaseEndMs - now) / currentPhaseDuration, 1);
  }

  // 主题色平滑过渡
  const targetHsl = PHASE_ACCENT[phase.value];
  const colorLerp = Math.min(0.003 * dt, 1);
  currentHsl = lerpHsl(currentHsl, targetHsl, colorLerp);
  applyAccentColors(currentHsl[0], currentHsl[1], currentHsl[2]);

  // 风
  updateWind(dt);

  // 强度平滑过渡
  const intensityLerp = Math.min(0.002 * dt, 1);
  currentRainIntensity += (getRainTarget() - currentRainIntensity) * intensityLerp;
  currentLeafIntensity += (getLeafTarget() - currentLeafIntensity) * intensityLerp;

  // ── Canvas ──
  const canvas = canvasRef.value;
  if (!canvas) { raf = requestAnimationFrame(tick); return; }
  const ctx = canvas.getContext('2d');
  if (!ctx) { raf = requestAnimationFrame(tick); return; }

  const w = canvas.width;
  const h = canvas.height;
  ctx.clearRect(0, 0, w, h);

  // 树叶
  if (currentLeafIntensity > 0.01) {
    const visibleLeaves = Math.floor(LEAF_COUNT * currentLeafIntensity);
    for (let i = 0; i < visibleLeaves; i++) {
      const lf = leaves[i];
      const wobble = Math.sin(lf.wobblePhase) * 2;
      lf.x += (lf.speedX * windStrength + windAngle * 60) * (dt / 16);
      lf.y += (lf.speedY + wobble * 0.5) * (dt / 16);
      lf.rotation += lf.rotSpeed * windStrength * (dt / 16);
      lf.wobblePhase += lf.wobbleSpeed * dt;

      if (lf.x > w + 30 || lf.x < -30 || lf.y > h + 30) {
        Object.assign(lf, createLeaf(w, h, false));
        if (Math.random() > 0.3) {
          lf.x = -10 - Math.random() * 60;
          lf.y = Math.random() * h;
        } else {
          lf.x = Math.random() * w;
          lf.y = -10 - Math.random() * 30;
        }
      }

      drawLeaf(ctx, lf);
    }
  }

  // 雨丝
  if (currentRainIntensity > 0.01) {
    const visibleCount = Math.floor(RAIN_COUNT * currentRainIntensity);
    const dxWind = Math.sin(windAngle) * windStrength;
    const dyWind = Math.cos(windAngle);

    ctx.strokeStyle = '#a0b8dc';
    ctx.lineCap = 'round';
    ctx.lineWidth = 1;

    for (let i = 0; i < visibleCount; i++) {
      const d = drops[i];
      d.y += d.speed * dyWind * (dt / 16);
      d.x += d.speed * dxWind * (dt / 16);

      if (d.y > h + 10 || d.x > w + 100 || d.x < -100) {
        Object.assign(d, createDrop(w, h, false));
      }

      ctx.globalAlpha = d.opacity * currentRainIntensity;
      ctx.beginPath();
      ctx.moveTo(d.x, d.y);
      ctx.lineTo(d.x + dxWind * d.len, d.y + dyWind * d.len);
      ctx.stroke();
    }
    ctx.globalAlpha = 1;
  }

  raf = requestAnimationFrame(tick);
}

// ── 尺寸 ──
function resize() {
  const canvas = canvasRef.value;
  if (!canvas) return;
  const dpr = window.devicePixelRatio || 1;
  canvas.width = window.innerWidth * dpr;
  canvas.height = window.innerHeight * dpr;
  canvas.style.width = window.innerWidth + 'px';
  canvas.style.height = window.innerHeight + 'px';
  const ctx = canvas.getContext('2d');
  if (ctx) ctx.scale(dpr, dpr);
  initDrops(window.innerWidth, window.innerHeight);
  initLeaves(window.innerWidth, window.innerHeight);
}

onMounted(() => {
  resize();
  window.addEventListener('resize', resize);
  lastTime = 0;
  currentRainIntensity = 0;
  currentLeafIntensity = 0;
  windAngle = 0.15;
  windStrength = 0.3;
  windTargetAngle = 0.1;
  windTargetStrength = 0.3;
  windChangeTimer = 0;
  syncWeather();
  raf = requestAnimationFrame(tick);
  scheduleFlashes();
  requestAnimationFrame(() => { loaded.value = true; });
});

onUnmounted(() => {
  cancelAnimationFrame(raf);
  clearFlashSchedule();
  window.removeEventListener('resize', resize);
});
</script>

<template>
  <div class="storm-effect" :class="{ loaded }">
    <div class="storm-sky">
      <div class="storm-moon" :style="moonStyle">
        <div class="moon-glow"></div>
        <div class="moon-body"></div>
        <div class="moon-cloud-cover" :style="{ opacity: moonCloudOpacity }"></div>
      </div>

      <div class="storm-clouds" :class="phase">
        <div class="cloud cloud-1"></div>
        <div class="cloud cloud-2"></div>
        <div class="cloud cloud-3"></div>
      </div>
    </div>

    <div ref="flashRef" class="storm-flash"></div>
    <canvas ref="canvasRef" class="storm-rain-canvas"></canvas>
  </div>
</template>

<style scoped>
.storm-effect {
  position: fixed;
  inset: 0;
  z-index: -1;
  opacity: 0;
  transition: opacity 1.5s ease;
  pointer-events: none;
}

.storm-effect.loaded {
  opacity: 1;
}

.storm-sky {
  position: absolute;
  inset: 0;
  background:
    radial-gradient(ellipse at 40% 20%, rgba(50, 60, 120, 0.12) 0%, transparent 50%),
    radial-gradient(ellipse at 70% 60%, rgba(40, 50, 110, 0.08) 0%, transparent 50%),
    linear-gradient(180deg, #060810 0%, #0c1020 35%, #101830 65%, #080c18 100%);
}

.storm-moon {
  position: absolute;
  width: 56px;
  height: 56px;
  transform: translate(-50%, -50%);
}

.moon-body {
  position: relative;
  width: 100%;
  height: 100%;
  border-radius: 50%;
  background: radial-gradient(circle at 38% 36%, #f5eed8 0%, #e0d4a8 55%, #c8b880 100%);
  box-shadow: 0 0 24px rgba(245, 238, 216, 0.35), 0 0 60px rgba(245, 238, 216, 0.1);
  z-index: 1;
}

.moon-glow {
  position: absolute;
  inset: -40px;
  border-radius: 50%;
  background: radial-gradient(circle, rgba(245, 238, 216, 0.12) 0%, transparent 70%);
  z-index: 0;
}

.moon-cloud-cover {
  position: absolute;
  inset: -50px;
  border-radius: 50%;
  background: radial-gradient(circle, rgba(12, 15, 30, 1) 0%, rgba(12, 15, 30, 0.9) 40%, rgba(12, 15, 30, 0.5) 65%, transparent 80%);
  transition: opacity 2s ease;
  z-index: 2;
}

.storm-clouds {
  position: absolute;
  inset: 0;
  transition: opacity 2.5s ease;
}

.cloud {
  position: absolute;
  border-radius: 50%;
  background: radial-gradient(ellipse, rgba(30, 35, 60, 0.9) 0%, rgba(20, 25, 45, 0.6) 50%, transparent 70%);
  filter: blur(30px);
}

.cloud-1 {
  width: 500px; height: 160px; top: 2%; right: 5%;
  animation: cloudDrift1 25s ease-in-out infinite;
}
.cloud-2 {
  width: 400px; height: 130px; top: 6%; right: 15%;
  animation: cloudDrift2 30s ease-in-out infinite 5s;
  opacity: 0.8;
}
.cloud-3 {
  width: 350px; height: 120px; top: 0%; left: 20%;
  animation: cloudDrift3 22s ease-in-out infinite 8s;
  opacity: 0.6;
}

.storm-clouds.clear { opacity: 0.15; }
.storm-clouds.cloudy { opacity: 0.9; }
.storm-clouds.lightning { opacity: 1; }
.storm-clouds.lightning .cloud {
  background: radial-gradient(ellipse, rgba(50, 55, 90, 0.95) 0%, rgba(35, 40, 70, 0.7) 50%, transparent 70%);
}
.storm-clouds.rain { opacity: 1; }
.storm-clouds.rain .cloud {
  background: radial-gradient(ellipse, rgba(25, 28, 50, 0.95) 0%, rgba(18, 22, 42, 0.7) 50%, transparent 70%);
}

@keyframes cloudDrift1 {
  0%, 100% { transform: translateX(0); }
  50% { transform: translateX(-40px); }
}
@keyframes cloudDrift2 {
  0%, 100% { transform: translateX(0) translateY(0); }
  50% { transform: translateX(30px) translateY(-8px); }
}
@keyframes cloudDrift3 {
  0%, 100% { transform: translateX(0); }
  50% { transform: translateX(-25px) translateY(5px); }
}

.storm-flash {
  position: fixed;
  inset: 0;
  background: radial-gradient(ellipse at 50% 20%, rgba(200, 210, 255, 0.8) 0%, rgba(150, 170, 240, 0.3) 40%, transparent 70%);
  opacity: 0;
  pointer-events: none;
  z-index: 1;
  transition: opacity 0.05s linear;
}

.storm-rain-canvas {
  position: fixed;
  inset: 0;
  pointer-events: none;
  z-index: 1;
}
</style>
