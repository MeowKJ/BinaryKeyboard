<template>
  <div class="panel keyboard-status-panel">
    <h3 class="panel-title">
      <i class="pi pi-desktop"></i>
      键盘状态
    </h3>

    <div class="status-grid">
      <!-- 电池状态 - 占满一行两列 -->
      <div class="status-card battery-card" :class="{ charging: isCharging }">
        <div class="status-icon battery-icon-wrap" :style="{ '--bat-c': batColor }">
          <i class="pi" :class="batteryIcon"></i>
        </div>
        <div class="status-content battery-content">
          <span class="status-label">电池</span>
          <div class="battery-info">
            <span class="bat-percent" :style="{ color: batColor }">{{ batteryLevel }}%</span>
            <span class="bat-dot">·</span>
            <span class="bat-voltage">{{ voltage.toFixed(2) }}V</span>
            <span v-if="isCharging" class="bat-charging">
              <i class="pi pi-bolt"></i> 充电中
            </span>
          </div>
        </div>
        <!-- 迷你电量条 -->
        <div class="bat-bar-wrap">
          <div class="bat-bar-bg">
            <div class="bat-bar-fill" :style="{ width: batteryLevel + '%', background: batColor }"></div>
          </div>
        </div>
      </div>

      <!-- 连接模式 -->
      <div class="status-card connection-card">
        <div class="status-icon">
          <i class="pi" :class="connectionIcon"></i>
        </div>
        <div class="status-content">
          <span class="status-label">连接</span>
          <span class="status-value">{{ connectionMode }}</span>
        </div>
      </div>

      <!-- RGB 状态 -->
      <div class="status-card rgb-card">
        <div class="status-icon">
          <i class="pi pi-sun"></i>
        </div>
        <div class="status-content">
          <span class="status-label">RGB</span>
          <span class="status-value">{{ rgbStatus }}</span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import { useDeviceStore } from '@/stores/deviceStore';

const deviceStore = useDeviceStore();

// 电池
const batteryLevel = computed(() => deviceStore.deviceStatus?.batteryLevel ?? 0);
const voltage = computed(() => deviceStore.batteryVoltage);
const isCharging = computed(() => deviceStore.deviceStatus?.isCharging ?? false);

const batColor = computed(() => {
  if (isCharging.value) return '#4ade80';
  if (batteryLevel.value > 60) return '#4ade80';
  if (batteryLevel.value > 20) return '#fbbf24';
  return '#fb7185';
});

const batteryIcon = computed(() => {
  if (isCharging.value) return 'pi-bolt';
  if (batteryLevel.value > 75) return 'pi-battery';
  if (batteryLevel.value > 25) return 'pi-battery';
  return 'pi-battery';
});

// 连接模式
const connectionMode = computed(() => {
  const mode = deviceStore.deviceStatus?.workMode ?? 0;
  return mode === 1 ? 'BLE' : 'USB';
});

const connectionIcon = computed(() => {
  const mode = deviceStore.deviceStatus?.workMode ?? 0;
  return mode === 1 ? 'pi-wifi' : 'pi-link';
});

// RGB 状态
const rgbModeNames: Record<number, string> = {
  1: '静态',
  2: '呼吸',
  3: '闪烁',
  4: '彩虹',
  5: '指示灯',
};

const rgbStatus = computed(() => {
  const mode = deviceStore.rgbConfig?.mode ?? 0;
  return rgbModeNames[mode] ?? '关闭';
});
</script>

<style scoped>
.keyboard-status-panel {
  /* 继承父级 panel 样式 */
}

.status-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 0.5rem;
}

.status-card {
  position: relative;
  display: flex;
  align-items: center;
  gap: 0.6rem;
  padding: 0.6rem 0.75rem;
  background: var(--c-bg-tertiary);
  border: 1px solid var(--c-border);
  border-radius: var(--radius-md);
  transition: all 0.2s ease;
  overflow: hidden;
}

.status-card:hover {
  border-color: var(--c-accent);
  transform: translateY(-1px);
  box-shadow: 0 4px 12px rgba(139, 92, 246, 0.1);
}

.status-icon {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 2rem;
  height: 2rem;
  border-radius: var(--radius-sm);
  background: var(--c-accent-soft);
  color: var(--c-accent);
  font-size: 0.9rem;
  flex-shrink: 0;
}

.status-content {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 0.1rem;
  min-width: 0;
}

.status-label {
  font-size: 0.65rem;
  color: var(--c-text-secondary);
  font-weight: 500;
  text-transform: uppercase;
  letter-spacing: 0.03em;
}

.status-value {
  font-size: 0.8rem;
  color: var(--c-text-primary);
  font-weight: 600;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

/* ── 电池卡片 ── */
.battery-card {
  grid-column: 1 / -1;
  border-left: 3px solid var(--bat-c, #4ade80);
  flex-wrap: wrap;
}

.battery-card:hover {
  border-color: var(--bat-c, #4ade80);
}

.battery-icon-wrap {
  background: color-mix(in srgb, var(--bat-c, #4ade80) 12%, transparent);
  color: var(--bat-c, #4ade80);
}

.battery-content {
  flex: 1;
}

.battery-info {
  display: flex;
  align-items: center;
  gap: 0.4rem;
  font-size: 0.8rem;
}

.bat-percent {
  font-weight: 700;
  font-size: 0.9rem;
}

.bat-dot {
  color: var(--c-text-muted);
  font-size: 0.7rem;
}

.bat-voltage {
  color: var(--c-text-secondary);
  font-weight: 500;
  font-family: 'JetBrains Mono', 'Consolas', monospace;
  font-size: 0.75rem;
}

.bat-charging {
  color: #4ade80;
  font-weight: 600;
  font-size: 0.7rem;
  display: flex;
  align-items: center;
  gap: 0.2rem;
  animation: chargePulse 2s ease-in-out infinite;
}

.bat-charging .pi {
  font-size: 0.6rem;
}

/* 迷你电量条 */
.bat-bar-wrap {
  width: 100%;
  flex-basis: 100%;
  padding-top: 0.35rem;
}

.bat-bar-bg {
  width: 100%;
  height: 3px;
  background: var(--c-bg-secondary);
  border-radius: 2px;
  overflow: hidden;
}

.bat-bar-fill {
  height: 100%;
  border-radius: 2px;
  transition: width 0.8s cubic-bezier(0.4, 0, 0.2, 1), background 0.5s ease;
}

.battery-card.charging .bat-bar-fill {
  animation: barGlow 2s ease-in-out infinite;
}

@keyframes chargePulse {
  0%, 100% { opacity: 0.8; }
  50% { opacity: 1; }
}

@keyframes barGlow {
  0%, 100% { box-shadow: none; opacity: 0.8; }
  50% { box-shadow: 0 0 6px currentColor; opacity: 1; }
}

/* ── 连接卡片 ── */
.connection-card {
  border-left: 3px solid #3b82f6;
}

.connection-card .status-icon {
  background: rgba(59, 130, 246, 0.1);
  color: #3b82f6;
}

/* ── RGB 卡片 ── */
.rgb-card {
  border-left: 3px solid #ec4899;
}

.rgb-card .status-icon {
  background: rgba(236, 72, 153, 0.1);
  color: #ec4899;
}

/* ── 亮色主题 ── */
[data-theme="light"] .status-card {
  background: #f8f9fa;
}

[data-theme="light"] .status-card:hover {
  box-shadow: 0 4px 12px rgba(139, 92, 246, 0.08);
}

[data-theme="light"] .bat-bar-bg {
  background: #e2e8f0;
}
</style>
