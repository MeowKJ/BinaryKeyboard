<template>
  <div class="panel keyboard-status-panel">
    <h3 class="panel-title">
      <i class="pi pi-desktop"></i>
      键盘状态
    </h3>

    <div class="status-grid">
      <!-- 电池状态 - 占满一行两列 -->
      <div v-if="supportsBattery" class="status-card battery-card" :class="{ charging: isCharging }">
        <div class="status-icon battery-icon-wrap" :style="{ '--bat-c': batColor }">
          <span class="battery-glyph" :class="{ charging: isCharging }">
            <span class="battery-glyph-fill" :style="{ width: displayBatteryLevel + '%' }"></span>
          </span>
          <i v-if="isCharging" class="pi pi-bolt battery-charge-icon"></i>
        </div>
        <div class="status-content battery-content">
          <span class="status-label">电池</span>
          <div class="battery-info">
            <span class="bat-percent" :style="{ color: batColor }">{{ displayBatteryLevel }}%</span>
            <span class="bat-state">{{ batteryStateLabel }}</span>
            <span class="bat-voltage">{{ voltageLabel }}</span>
            <span v-if="isCharging" class="bat-charging">
              <i class="pi pi-bolt"></i> 充电中
            </span>
          </div>
        </div>
        <!-- 迷你电量条 -->
        <div class="bat-bar-wrap">
          <div class="bat-bar-bg">
            <div class="bat-bar-fill" :style="{ width: displayBatteryLevel + '%', background: batColor }"></div>
          </div>
        </div>
      </div>

      <!-- 当前工作模式 -->
      <div class="status-card connection-card" :class="{ wide: !supportsBattery && !supportsRgb }">
        <div class="status-icon">
          <i class="pi" :class="connectionIcon"></i>
        </div>
        <div class="status-content">
          <span class="status-label">当前模式</span>
          <span class="status-value">{{ connectionMode }}</span>
        </div>
      </div>

      <!-- RGB 状态 -->
      <div v-if="supportsRgb" class="status-card rgb-card">
        <div class="status-icon">
          <i class="pi pi-sun"></i>
        </div>
        <div class="status-content">
          <span class="status-label">RGB</span>
          <span class="status-value">{{ rgbStatus }}</span>
        </div>
      </div>

      <div v-if="supportsOsMode" class="status-card os-mode-card">
        <div class="status-icon">
          <i class="pi" :class="osModeIcon"></i>
        </div>
        <div class="status-content">
          <span class="status-label">系统</span>
          <div class="os-mode-toggle" role="group" aria-label="系统模式">
            <button
              type="button"
              class="os-mode-btn"
              :class="{ active: osMode === OsMode.WIN }"
              :disabled="isSavingOsMode || osMode === OsMode.WIN"
              title="Win 模式"
              @click="setOsMode(OsMode.WIN)"
            >
              <i class="pi pi-microsoft"></i>
              <span>Win</span>
            </button>
            <button
              type="button"
              class="os-mode-btn"
              :class="{ active: osMode === OsMode.MAC }"
              :disabled="isSavingOsMode || osMode === OsMode.MAC"
              title="Mac 模式"
              @click="setOsMode(OsMode.MAC)"
            >
              <i class="pi pi-apple"></i>
              <span>Mac</span>
            </button>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue';
import { useDeviceStore } from '@/stores/deviceStore';
import { OsMode } from '@/types/protocol';

const deviceStore = useDeviceStore();
const supportsBattery = computed(() => deviceStore.supportsBattery);
const supportsRgb = computed(() => deviceStore.supportsRgb);
const supportsOsMode = computed(() => deviceStore.supportsOsMode);
const isSavingOsMode = ref(false);

// 电池
const batteryLevel = computed(() => deviceStore.deviceStatus?.batteryLevel ?? 0);
const voltage = computed(() => deviceStore.batteryVoltage);
const isCharging = computed(() => deviceStore.deviceStatus?.isCharging ?? false);
const displayBatteryLevel = computed(() => Math.max(0, Math.min(100, Math.round(batteryLevel.value))));

const batColor = computed(() => {
  if (isCharging.value) return '#4ade80';
  if (displayBatteryLevel.value > 60) return '#4ade80';
  if (displayBatteryLevel.value > 20) return '#fbbf24';
  return '#fb7185';
});

const batteryStateLabel = computed(() => {
  if (isCharging.value) return '补能';
  if (displayBatteryLevel.value > 80) return '充足';
  if (displayBatteryLevel.value > 40) return '正常';
  if (displayBatteryLevel.value > 20) return '偏低';
  return '低电量';
});

const voltageLabel = computed(() => {
  if (!Number.isFinite(voltage.value) || voltage.value <= 0) return '-- V';
  return `${voltage.value.toFixed(2)}V`;
});

// 当前工作模式
const connectionMode = computed(() => {
  if (!deviceStore.supportsWireless) return 'USB 模式';
  const mode = deviceStore.deviceStatus?.workMode ?? 0;
  return mode === 1 ? 'BLE 模式' : 'USB 模式';
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
  if (!supportsRgb.value) return '不支持';
  const mode = deviceStore.rgbConfig?.mode ?? 0;
  return rgbModeNames[mode] ?? '关闭';
});

const osMode = computed(() => deviceStore.osModeConfig.mode);
const osModeIcon = computed(() => (osMode.value === OsMode.MAC ? 'pi-apple' : 'pi-microsoft'));

async function setOsMode(mode: OsMode) {
  if (isSavingOsMode.value || osMode.value === mode) return;
  isSavingOsMode.value = true;
  try {
    await deviceStore.saveOsMode(mode);
  } finally {
    isSavingOsMode.value = false;
  }
}
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

.status-card.wide {
  grid-column: 1 / -1;
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
  position: relative;
  background: color-mix(in srgb, var(--bat-c, #4ade80) 12%, transparent);
  color: var(--bat-c, #4ade80);
}

.battery-glyph {
  position: relative;
  width: 1.25rem;
  height: 0.7rem;
  border: 2px solid currentColor;
  border-radius: 4px;
  padding: 2px;
  box-shadow: inset 0 0 0 1px color-mix(in srgb, currentColor 8%, transparent);
}

.battery-glyph::after {
  content: '';
  position: absolute;
  top: 50%;
  right: -5px;
  width: 3px;
  height: 8px;
  border-radius: 0 2px 2px 0;
  background: currentColor;
  transform: translateY(-50%);
}

.battery-glyph-fill {
  display: block;
  height: 100%;
  min-width: 2px;
  border-radius: 2px;
  background: currentColor;
  transition: width 0.8s cubic-bezier(0.4, 0, 0.2, 1);
}

.battery-charge-icon {
  position: absolute;
  right: 0.16rem;
  bottom: 0.12rem;
  font-size: 0.58rem;
  color: #ffffff;
  filter: drop-shadow(0 0 4px var(--bat-c, #4ade80));
}

.battery-content {
  flex: 1;
}

.battery-info {
  display: flex;
  align-items: center;
  gap: 0.4rem;
  font-size: 0.8rem;
  min-width: 0;
  flex-wrap: wrap;
}

.bat-percent {
  font-weight: 700;
  font-size: 0.9rem;
}

.bat-state {
  color: var(--c-text-secondary);
  font-size: 0.72rem;
  font-weight: 700;
  padding: 0.08rem 0.38rem;
  border-radius: 999px;
  background: color-mix(in srgb, var(--bat-c, #4ade80) 14%, transparent);
  border: 1px solid color-mix(in srgb, var(--bat-c, #4ade80) 26%, transparent);
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

/* ── Win / Mac 模式 ── */
.os-mode-card {
  grid-column: 1 / -1;
  border-left: 3px solid #14b8a6;
}

.os-mode-card .status-icon {
  background: rgba(20, 184, 166, 0.1);
  color: #14b8a6;
}

.os-mode-toggle {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 0.35rem;
  width: 100%;
  max-width: 11.5rem;
}

.os-mode-btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 0.3rem;
  min-height: 1.8rem;
  padding: 0 0.55rem;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  background: var(--c-bg-secondary);
  color: var(--c-text-secondary);
  font-size: 0.72rem;
  font-weight: 700;
  cursor: pointer;
  transition: border-color 0.16s ease, background 0.16s ease, color 0.16s ease;
}

.os-mode-btn .pi {
  font-size: 0.72rem;
}

.os-mode-btn:hover:not(:disabled) {
  border-color: #14b8a6;
  color: var(--c-text-primary);
}

.os-mode-btn.active {
  border-color: #14b8a6;
  background: rgba(20, 184, 166, 0.16);
  color: #5eead4;
}

.os-mode-btn:disabled {
  cursor: default;
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

[data-theme="light"] .os-mode-btn {
  background: #ffffff;
}

[data-theme="light"] .os-mode-btn.active {
  background: #ccfbf1;
  color: #0f766e;
}
</style>
