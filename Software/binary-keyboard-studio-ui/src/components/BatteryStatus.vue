<script setup lang="ts">
/**
 * BatteryStatus - 电池动画状态面板
 * SVG 液态波浪填充 + 充电闪电 + 发光效果
 */
import { ref, computed, onMounted, onUnmounted, watch } from 'vue';
import { hidService } from '@/services/HidService';

/* ── 数据 ─────────────────────────────────────────── */

const level = ref(0);
const voltage = ref(0);
const isCharging = ref(false);
const isLoading = ref(false);
const lastUpdate = ref<Date | null>(null);

/** 用于平滑数字跳动的显示值 */
const displayLevel = ref(0);

let pollTimer: ReturnType<typeof setInterval> | null = null;
let animFrame: ReturnType<typeof requestAnimationFrame> | null = null;

async function fetchBattery() {
  try {
    isLoading.value = true;
    const data = await hidService.getBattery();
    level.value = data.level;
    voltage.value = data.voltage;
    isCharging.value = data.isCharging;
    lastUpdate.value = new Date();
  } catch {
    /* 静默失败，保持上次数据 */
  } finally {
    isLoading.value = false;
  }
}

/* 数字跳动动画 */
watch(level, (target) => {
  const start = displayLevel.value;
  const diff = target - start;
  if (diff === 0) return;
  const duration = 800;
  const startTime = performance.now();

  function step(now: number) {
    const elapsed = now - startTime;
    const progress = Math.min(elapsed / duration, 1);
    const ease = 1 - Math.pow(1 - progress, 3); // easeOutCubic
    displayLevel.value = Math.round(start + diff * ease);
    if (progress < 1) {
      animFrame = requestAnimationFrame(step);
    }
  }
  if (animFrame) cancelAnimationFrame(animFrame);
  animFrame = requestAnimationFrame(step);
});

onMounted(() => {
  fetchBattery();
  pollTimer = setInterval(fetchBattery, 10000);
});

onUnmounted(() => {
  if (pollTimer) clearInterval(pollTimer);
  if (animFrame) cancelAnimationFrame(animFrame);
});

/* ── 计算属性 ──────────────────────────────────────── */

const batColor = computed(() => {
  if (isCharging.value) return '#4ade80';
  if (level.value > 60) return '#4ade80';
  if (level.value > 20) return '#fbbf24';
  return '#fb7185';
});

const batColorSoft = computed(() => {
  const c = batColor.value;
  return c + '26'; // ~15% alpha
});

const batColorGlow = computed(() => {
  const c = batColor.value;
  return c + '60'; // ~38% alpha
});

/** SVG 填充高度 (电池内部区域 0-120) */
const fillHeight = computed(() => Math.max(0, Math.min(120, level.value * 1.2)));

/** 填充 Y 坐标 (从底部往上) */
const fillY = computed(() => 150 - fillHeight.value);

/** 波浪路径 (宽 200, 双倍用于平移动画) */
const wavePath1 = computed(() => {
  const y = fillY.value;
  return `M0,${y} Q25,${y - 6} 50,${y} T100,${y} T150,${y} T200,${y} T250,${y} T300,${y} T350,${y} T400,${y} V170 H0 Z`;
});

const wavePath2 = computed(() => {
  const y = fillY.value + 3;
  return `M0,${y} Q25,${y + 5} 50,${y} T100,${y} T150,${y} T200,${y} T250,${y} T300,${y} T350,${y} T400,${y} V170 H0 Z`;
});

const statusText = computed(() => {
  if (isCharging.value) return '充电中';
  if (level.value > 80) return '电量充足';
  if (level.value > 40) return '电量正常';
  if (level.value > 20) return '电量偏低';
  return '电量不足';
});

const statusIcon = computed(() => {
  if (isCharging.value) return 'pi pi-bolt';
  if (level.value > 60) return 'pi pi-check-circle';
  if (level.value > 20) return 'pi pi-exclamation-circle';
  return 'pi pi-times-circle';
});

const timeString = computed(() => {
  if (!lastUpdate.value) return '--:--';
  return lastUpdate.value.toLocaleTimeString('zh-CN', { hour: '2-digit', minute: '2-digit', second: '2-digit' });
});

const cssVars = computed(() => ({
  '--bat-color': batColor.value,
  '--bat-color-soft': batColorSoft.value,
  '--bat-color-glow': batColorGlow.value,
}));
</script>

<template>
  <div class="battery-panel" :style="cssVars" :class="{ charging: isCharging }">
    <h3 class="panel-title">
      <i class="pi pi-bolt"></i>
      电池状态
      <button class="bat-refresh" @click="fetchBattery" :disabled="isLoading"
        :class="{ spinning: isLoading }">
        <i class="pi pi-sync"></i>
      </button>
    </h3>

    <!-- 电池可视化 -->
    <div class="battery-visual">
      <!-- 外发光装饰环 -->
      <div class="battery-glow-ring"></div>

      <svg class="battery-svg" viewBox="0 0 200 180" xmlns="http://www.w3.org/2000/svg">
        <defs>
          <!-- 电池内部裁剪区 -->
          <clipPath id="bat-clip">
            <rect x="30" y="30" width="140" height="120" rx="8" />
          </clipPath>
          <!-- 波浪渐变 -->
          <linearGradient id="wave-grad" x1="0" y1="0" x2="0" y2="1">
            <stop offset="0%" :stop-color="batColor" stop-opacity="0.9" />
            <stop offset="100%" :stop-color="batColor" stop-opacity="0.6" />
          </linearGradient>
          <linearGradient id="wave-grad2" x1="0" y1="0" x2="0" y2="1">
            <stop offset="0%" :stop-color="batColor" stop-opacity="0.5" />
            <stop offset="100%" :stop-color="batColor" stop-opacity="0.3" />
          </linearGradient>
        </defs>

        <!-- 电池正极帽 -->
        <rect x="80" y="18" width="40" height="14" rx="4"
          fill="none" :stroke="batColor" stroke-width="2.5" opacity="0.7" />

        <!-- 电池外框 -->
        <rect x="30" y="30" width="140" height="120" rx="10"
          fill="none" :stroke="batColor" stroke-width="2.5"
          class="battery-outline" />

        <!-- 裁剪组: 液态填充 -->
        <g clip-path="url(#bat-clip)">
          <!-- 底色填充 -->
          <rect x="30" :y="fillY" width="140" :height="fillHeight + 2"
            :fill="batColor" opacity="0.15" class="battery-fill-bg" />

          <!-- 波浪层2 (后) -->
          <path :d="wavePath2" fill="url(#wave-grad2)" class="wave wave-2" />

          <!-- 波浪层1 (前) -->
          <path :d="wavePath1" fill="url(#wave-grad)" class="wave wave-1" />
        </g>

        <!-- 充电闪电 -->
        <g v-if="isCharging" class="charge-bolt-group">
          <path
            d="M108,58 L92,92 H106 L90,128 L118,86 H104 L116,58 Z"
            fill="#fff" fill-opacity="0.9"
            :stroke="batColor" stroke-width="1"
            class="charge-bolt"
          />
        </g>
      </svg>

      <!-- 百分比叠加 -->
      <div class="battery-percent-overlay">
        <span class="percent-number">{{ displayLevel }}</span>
        <span class="percent-sign">%</span>
      </div>
    </div>

    <!-- 数据统计 -->
    <div class="battery-stats">
      <div class="stat-item">
        <span class="stat-label">电压</span>
        <span class="stat-value">{{ voltage.toFixed(1) }}V</span>
      </div>
      <div class="stat-item">
        <span class="stat-label">状态</span>
        <span class="stat-value" :class="{ 'is-charging': isCharging }">
          <i :class="statusIcon" class="stat-icon"></i>
          {{ statusText }}
        </span>
      </div>
      <div class="stat-item">
        <span class="stat-label">刷新</span>
        <span class="stat-value stat-time">{{ timeString }}</span>
      </div>
    </div>
  </div>
</template>

<style scoped>
/* ── 面板容器 ────────────────────────────────────── */
.battery-panel {
  background: var(--c-bg-secondary);
  border: 1px solid var(--c-border-light);
  border-radius: var(--radius-md);
  padding: 0.875rem;
  transition: border-color 0.4s ease, box-shadow 0.4s ease;
}

.battery-panel:hover {
  border-color: var(--bat-color, var(--c-border));
  box-shadow: 0 0 12px var(--bat-color-soft, transparent);
}

.battery-panel.charging {
  animation: chargeGlow 3s ease-in-out infinite;
}

/* ── 标题 ─────────────────────────────────────────── */
.panel-title {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  font-size: 0.85rem;
  font-weight: 600;
  color: var(--c-text-secondary);
  margin-bottom: 0.75rem;
}

.panel-title i:first-child {
  color: var(--bat-color, var(--c-accent));
  text-shadow: 0 0 8px var(--bat-color-glow, transparent);
}

.bat-refresh {
  margin-left: auto;
  background: none;
  border: 1px solid var(--c-border-light);
  border-radius: 6px;
  color: var(--c-text-muted);
  cursor: pointer;
  padding: 0.2rem 0.4rem;
  font-size: 0.75rem;
  transition: all 0.2s ease;
}

.bat-refresh:hover {
  color: var(--bat-color);
  border-color: var(--bat-color);
}

.bat-refresh.spinning i {
  animation: spin 0.8s linear infinite;
}

/* ── 电池可视化 ──────────────────────────────────── */
.battery-visual {
  position: relative;
  display: flex;
  justify-content: center;
  align-items: center;
  padding: 0.5rem 0;
  margin-bottom: 0.5rem;
}

.battery-svg {
  width: 140px;
  height: 126px;
  filter: drop-shadow(0 0 6px var(--bat-color-soft, transparent));
}

/* 电池外框发光 */
.battery-outline {
  filter: drop-shadow(0 0 3px var(--bat-color-glow, transparent));
  transition: stroke 0.5s ease;
}

/* 填充高度过渡 */
.battery-fill-bg {
  transition: y 1s cubic-bezier(0.4, 0, 0.2, 1),
              height 1s cubic-bezier(0.4, 0, 0.2, 1),
              fill 0.5s ease;
}

/* ── 波浪动画 ─────────────────────────────────────── */
.wave {
  transition: d 1s cubic-bezier(0.4, 0, 0.2, 1);
}

.wave-1 {
  animation: waveScroll1 3s linear infinite;
}

.wave-2 {
  animation: waveScroll2 4s linear infinite;
}

@keyframes waveScroll1 {
  0% { transform: translateX(0); }
  100% { transform: translateX(-100px); }
}

@keyframes waveScroll2 {
  0% { transform: translateX(-100px); }
  100% { transform: translateX(0); }
}

/* ── 充电闪电 ─────────────────────────────────────── */
.charge-bolt-group {
  animation: chargePulse 2s ease-in-out infinite;
}

.charge-bolt {
  filter: drop-shadow(0 0 6px var(--bat-color, #4ade80))
          drop-shadow(0 0 12px var(--bat-color-glow, transparent));
}

@keyframes chargePulse {
  0%, 100% {
    opacity: 0.7;
    transform: scale(1);
  }
  50% {
    opacity: 1;
    transform: scale(1.05);
  }
}

/* ── 百分比叠加 ──────────────────────────────────── */
.battery-percent-overlay {
  position: absolute;
  display: flex;
  align-items: baseline;
  justify-content: center;
  pointer-events: none;
}

.percent-number {
  font-family: 'Nunito', sans-serif;
  font-size: 2rem;
  font-weight: 800;
  color: var(--c-text-primary);
  text-shadow:
    0 0 10px var(--bat-color-glow, transparent),
    0 0 20px var(--bat-color-soft, transparent);
  line-height: 1;
}

.percent-sign {
  font-family: 'Nunito', sans-serif;
  font-size: 1rem;
  font-weight: 700;
  color: var(--c-text-secondary);
  margin-left: 2px;
}

/* 充电状态时闪电隐藏百分比 (闪电更醒目) */
.charging .battery-percent-overlay {
  opacity: 0;
}

/* ── 外发光环 ─────────────────────────────────────── */
.battery-glow-ring {
  position: absolute;
  width: 160px;
  height: 150px;
  border-radius: 50%;
  background: radial-gradient(ellipse, var(--bat-color-soft, transparent) 0%, transparent 70%);
  pointer-events: none;
  opacity: 0.5;
  transition: opacity 0.4s ease;
}

.charging .battery-glow-ring {
  animation: ringPulse 3s ease-in-out infinite;
}

@keyframes ringPulse {
  0%, 100% { opacity: 0.3; transform: scale(1); }
  50% { opacity: 0.7; transform: scale(1.08); }
}

/* ── 数据统计 ─────────────────────────────────────── */
.battery-stats {
  display: flex;
  flex-direction: column;
  gap: 0.375rem;
}

.stat-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 0.3rem 0.5rem;
  background: var(--c-bg-tertiary);
  border-radius: var(--radius-sm);
  border: 1px solid transparent;
  transition: border-color 0.2s ease;
}

.stat-item:hover {
  border-color: var(--c-border-light);
}

.stat-label {
  font-size: 0.75rem;
  color: var(--c-text-muted);
}

.stat-value {
  font-size: 0.8rem;
  font-weight: 600;
  color: var(--c-text-primary);
  display: flex;
  align-items: center;
  gap: 0.35rem;
}

.stat-icon {
  font-size: 0.75rem;
  color: var(--bat-color);
  text-shadow: 0 0 6px var(--bat-color-glow, transparent);
}

.stat-value.is-charging {
  color: var(--c-success);
}

.stat-time {
  font-family: 'JetBrains Mono', 'Consolas', monospace;
  font-size: 0.7rem;
  color: var(--c-text-muted);
}

/* ── 全局动画 ─────────────────────────────────────── */
@keyframes chargeGlow {
  0%, 100% {
    box-shadow: 0 0 6px var(--bat-color-soft, transparent);
    border-color: var(--c-border-light);
  }
  50% {
    box-shadow: 0 0 16px var(--bat-color-glow, transparent),
                inset 0 0 12px var(--bat-color-soft, transparent);
    border-color: var(--bat-color, var(--c-border));
  }
}

@keyframes spin {
  from { transform: rotate(0deg); }
  to { transform: rotate(360deg); }
}

/* ── 浅色主题 ─────────────────────────────────────── */
:root[data-theme="light"] .battery-panel,
[data-theme="light"] .battery-panel {
  background: #f0f2f6;
  border-color: #d8dce6;
}

:root[data-theme="light"] .stat-item,
[data-theme="light"] .stat-item {
  background: #e8ebf0;
}

:root[data-theme="light"] .percent-number,
[data-theme="light"] .percent-number {
  color: #1a1a2e;
}
</style>
