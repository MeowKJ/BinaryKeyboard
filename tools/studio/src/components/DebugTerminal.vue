<template>
  <div v-if="terminalStore.isOpen" class="glow-terminal-panel" :style="{ height: terminalStore.panelHeight + 'px' }">
    <!-- 拖拽调整手柄 -->
    <div class="glow-resize-handle" @mousedown="onResizeStart">
      <div class="glow-resize-grip"></div>
    </div>

    <!-- 终端头部 -->
    <div class="glow-header">
      <div class="glow-header-left">
        <div class="glow-title">
          <span class="glow-title-icon glow-paw">🐾</span>
          <span>Debug Terminal</span>
          <span class="glow-cat-emoji">😺</span>
        </div>

        <!-- 过滤器标签 -->
        <div class="glow-filters">
          <button
            v-for="f in filterOptions"
            :key="f.key"
            class="glow-filter-btn"
            :class="{ active: terminalStore.filter === f.key }"
            :style="f.key !== 'all' && terminalStore.filter === f.key ? { '--filter-glow': f.color } : {}"
            @click="terminalStore.setFilter(f.key)"
          >
            <span class="glow-filter-dot" :style="{ background: f.color }"></span>
            {{ f.label }}
            <span class="glow-filter-count">{{ f.count }}</span>
          </button>
          <span v-if="terminalStore.errorCount > 0" class="glow-error-pill">
            {{ terminalStore.errorCount }} ERR
          </span>
        </div>
      </div>

      <div class="glow-header-right">
        <button
          class="glow-action-btn"
          :class="{ active: showSettings }"
          @click="showSettings = !showSettings"
          title="Log Settings"
        >
          <span class="glow-action-icon">&#x2699;</span>
        </button>
        <button
          class="glow-action-btn"
          :class="{ active: terminalStore.autoScroll }"
          @click="terminalStore.autoScroll = !terminalStore.autoScroll"
          title="Auto scroll"
        >
          <span class="glow-action-icon">&#x25BC;</span>
        </button>
        <button class="glow-action-btn" @click="terminalStore.clear()" title="Clear">
          <span class="glow-action-icon">&#x2715;</span>
        </button>
        <button class="glow-action-btn close" @click="terminalStore.isOpen = false" title="Close">
          <span class="glow-action-icon">&#x2014;</span>
        </button>
      </div>
    </div>

    <!-- 日志配置面板 -->
    <Transition name="glow-settings">
      <div v-if="showSettings" class="glow-settings-panel">
        <div class="glow-settings-row">
          <label class="glow-settings-label">HID 日志</label>
          <button
            class="glow-toggle-btn"
            :class="{ on: logConfig.enabled }"
            @click="toggleLogEnabled"
          >{{ logConfig.enabled ? 'ON' : 'OFF' }}</button>
          <button class="glow-settings-btn save" @click="saveLogConfig">保存</button>
        </div>

        <div class="glow-settings-row">
          <label class="glow-settings-label">类别过滤</label>
          <div class="glow-mask-chips">
            <button
              v-for="cat in logMaskOptions"
              :key="cat.key"
              class="glow-chip"
              :class="{ active: (terminalStore.categoryMask & cat.mask) !== 0 }"
              @click="terminalStore.toggleCategory(cat.mask)"
            >{{ cat.label }}</button>
          </div>
        </div>
      </div>
    </Transition>

    <!-- CRT 扫描线覆盖 -->
    <div class="glow-scanlines"></div>

    <!-- 终端内容 -->
    <div class="glow-body" ref="terminalBodyRef">
      <!-- 背景猫爪装饰 -->
      <div class="glow-paw-decorations">
        <span class="glow-deco-paw paw-1">🐾</span>
        <span class="glow-deco-paw paw-2">🐾</span>
        <span class="glow-deco-paw paw-3">🐾</span>
        <span class="glow-deco-yarn">🧶</span>
      </div>

      <div v-if="terminalStore.filteredEntries.length === 0" class="glow-empty">
        <div class="glow-empty-cat">😺</div>
        <div class="glow-empty-icon">🐾 🐾 🐾</div>
        <span>Waiting for meow data...</span>
        <span class="glow-empty-hint">Connect keyboard to start capturing packets</span>
      </div>

      <div
        v-for="entry in terminalStore.filteredEntries"
        :key="entry.id"
        class="glow-entry"
        :class="[
          `entry-${entry.direction}`,
          `level-${entry.level}`,
          { expanded: terminalStore.expandedId === entry.id }
        ]"
        :style="{
          '--entry-glow': entry.glowColor,
          '--entry-glow-soft': entry.glowColor + '20',
          '--entry-glow-med': entry.glowColor + '40',
        }"
        @click="terminalStore.toggleExpand(entry.id)"
      >
        <!-- 发光左边线 -->
        <div class="glow-entry-bar" :style="{ background: entry.glowColor }"></div>

        <!-- 主内容行 -->
        <div class="glow-entry-main">
          <!-- 时间 -->
          <span class="glow-time">{{ formatTime(entry.timestamp) }}</span>

          <!-- 方向标签 -->
          <span class="glow-direction" :class="entry.direction">
            {{ directionLabel(entry) }}
          </span>

          <!-- 命令名(发光) -->
          <span
            class="glow-command"
            :style="{ color: entry.glowColor, textShadow: `0 0 8px ${entry.glowColor}80` }"
          >{{ entry.command }}</span>

          <!-- HEX 前缀 (前3字节) -->
          <span class="glow-hex-brief">{{ briefHex(entry.rawHex) }}</span>

          <!-- 数据长度 -->
          <span class="glow-len">{{ entry.dataLen }}B</span>

          <!-- 状态码 -->
          <span
            class="glow-status"
            :class="{ error: entry.level === 'error', muted: !entry.statusCode }"
          >{{ entry.statusCode || '--' }}</span>

          <!-- 耗时 -->
          <span class="glow-duration">{{ entry.duration !== undefined ? `${entry.duration}ms` : '--' }}</span>

          <!-- 解析摘要 -->
          <span class="glow-parsed-brief">{{ entry.parsed }}</span>
        </div>

        <!-- 展开面板 (悬浮发光) -->
        <Transition name="glow-expand">
          <div v-if="terminalStore.expandedId === entry.id" class="glow-expand-panel">
            <div class="glow-expand-grid">
              <div class="glow-field">
                <span class="glow-field-label">CMD</span>
                <span class="glow-field-value hex">0x{{ entry.cmdHex.toUpperCase() }}</span>
              </div>
              <div class="glow-field">
                <span class="glow-field-label">SUB</span>
                <span class="glow-field-value hex">0x{{ entry.sub.toString(16).padStart(2, '0').toUpperCase() }}</span>
              </div>
              <div class="glow-field">
                <span class="glow-field-label">LEN</span>
                <span class="glow-field-value">{{ entry.dataLen }}</span>
              </div>
              <div v-if="entry.statusCode" class="glow-field">
                <span class="glow-field-label">STATUS</span>
                <span class="glow-field-value" :class="{ error: entry.level === 'error' }">{{ entry.statusCode }}</span>
              </div>
              <div v-if="entry.duration !== undefined" class="glow-field">
                <span class="glow-field-label">RTT</span>
                <span class="glow-field-value">{{ entry.duration }}ms</span>
              </div>
              <div v-if="entry.category" class="glow-field">
                <span class="glow-field-label">CAT</span>
                <span class="glow-field-value">{{ entry.category.toUpperCase() }}</span>
              </div>
            </div>
            <div class="glow-expand-section">
              <span class="glow-section-label">PARSED</span>
              <span class="glow-section-text">{{ entry.parsed }}</span>
            </div>
            <div class="glow-expand-section">
              <span class="glow-section-label">HEX DUMP</span>
              <div class="glow-hex-dump">{{ entry.rawHex }}</div>
            </div>
          </div>
        </Transition>
      </div>
    </div>
  </div>

  <!-- 底部状态栏 (始终显示) -->
  <div class="glow-statusbar">
    <button class="glow-statusbar-toggle" @click="terminalStore.toggle()" :class="{ active: terminalStore.isOpen }">
      <span class="glow-statusbar-icon">🐾</span>
      <span>Debug Terminal</span>
      <span v-if="terminalStore.entries.length > 0" class="glow-statusbar-count">
        {{ terminalStore.entries.length }}
      </span>
      <span v-if="terminalStore.isOpen" class="glow-statusbar-meow">😺</span>
    </button>

    <div class="glow-statusbar-info">
      <span v-if="terminalStore.errorCount > 0" class="glow-statusbar-errors">
        {{ terminalStore.errorCount }} err
      </span>
      <span class="glow-statusbar-stats">
        <span class="stat-item stat-tx">TX {{ terminalStore.sendCount }}</span>
        <span class="stat-item stat-rx">RX {{ terminalStore.receiveCount }}</span>
        <span v-if="terminalStore.deviceCount > 0" class="stat-item stat-dev">DEV {{ terminalStore.deviceCount }}</span>
      </span>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, watch, nextTick, computed } from 'vue';
import { useTerminalStore, type FilterMode } from '@/stores/terminalStore';
import type { TerminalEntry } from '@/stores/terminalStore';
import { hidService } from '@/services/HidService';
import { LOG_MASK_LABELS, createDefaultLogConfig, type LogConfig } from '@/types/protocol';

const terminalStore = useTerminalStore();
const terminalBodyRef = ref<HTMLElement | null>(null);
const showSettings = ref(false);
const logConfig = reactive<LogConfig>(createDefaultLogConfig());
const logMaskOptions = LOG_MASK_LABELS;  // 类别过滤现在是 UI 本地状态

// 过滤器选项
const filterOptions = computed(() => [
  { key: 'all' as FilterMode, label: 'All', color: '#94a3b8', count: terminalStore.entries.length },
  { key: 'send' as FilterMode, label: 'TX', color: '#60a5fa', count: terminalStore.sendCount },
  { key: 'receive' as FilterMode, label: 'RX', color: '#4ade80', count: terminalStore.receiveCount },
  { key: 'device' as FilterMode, label: 'DEV', color: '#f472b6', count: terminalStore.deviceCount },
]);

// 自动滚动
watch(
  () => terminalStore.filteredEntries.length,
  async () => {
    if (terminalStore.autoScroll && terminalBodyRef.value) {
      await nextTick();
      terminalBodyRef.value.scrollTop = terminalBodyRef.value.scrollHeight;
    }
  },
);

// 时间格式化
function formatTime(ts: number): string {
  const d = new Date(ts);
  return d.toLocaleTimeString('zh-CN', { hour12: false }) + '.' + String(d.getMilliseconds()).padStart(3, '0');
}

// 方向标签
function directionLabel(entry: TerminalEntry): string {
  if (entry.direction === 'send') return 'TX';
  if (entry.direction === 'receive') return 'RX';
  return 'DEV';
}

// 简短 HEX（CMD+SUB+LEN 前3字节）
function briefHex(rawHex: string): string {
  return rawHex.split(' ').slice(0, 3).join(' ');
}

// 日志配置操作 (固件端只控制开关)
async function loadLogConfig() {
  try {
    const cfg = await hidService.getLogConfig();
    Object.assign(logConfig, cfg);
  } catch { /* 设备未连接时忽略 */ }
}

function toggleLogEnabled() {
  logConfig.enabled = !logConfig.enabled;
  hidService.setLogConfig(logConfig).catch(() => {});
}

async function saveLogConfig() {
  try {
    await hidService.setLogConfig(logConfig);
    await hidService.saveConfig();
  } catch { /* ignore */ }
}

// 面板打开时自动加载固件配置
watch(showSettings, async (val) => {
  if (val) await loadLogConfig();
});

// 拖拽调整高度
let resizing = false;
let startY = 0;
let startHeight = 0;

function onResizeStart(e: MouseEvent) {
  resizing = true;
  startY = e.clientY;
  startHeight = terminalStore.panelHeight;
  document.addEventListener('mousemove', onResizeMove);
  document.addEventListener('mouseup', onResizeEnd);
  document.body.style.cursor = 'ns-resize';
  document.body.style.userSelect = 'none';
}

function onResizeMove(e: MouseEvent) {
  if (!resizing) return;
  const delta = startY - e.clientY;
  const newHeight = Math.min(Math.max(startHeight + delta, 150), window.innerHeight * 0.7);
  terminalStore.panelHeight = newHeight;
}

function onResizeEnd() {
  resizing = false;
  document.removeEventListener('mousemove', onResizeMove);
  document.removeEventListener('mouseup', onResizeEnd);
  document.body.style.cursor = '';
  document.body.style.userSelect = '';
}
</script>

<style scoped>
/* ============================================================================
   CSS Variables
   ============================================================================ */
.glow-terminal-panel {
  --glow-bg: #0a0a0f;
  --glow-bg-header: #0d0d14;
  --glow-bg-entry: rgba(255, 255, 255, 0.02);
  --glow-bg-entry-hover: rgba(255, 255, 255, 0.04);
  --glow-border: rgba(255, 255, 255, 0.06);
  --glow-text: #c8ccd4;
  --glow-text-dim: #6b7280;
  --glow-text-muted: #3f4550;
  --glow-font: 'JetBrains Mono', 'SF Mono', 'Fira Code', 'Cascadia Code', 'Consolas', monospace;
}

/* ============================================================================
   Terminal Panel
   ============================================================================ */
.glow-terminal-panel {
  position: fixed;
  bottom: 32px;
  left: 0;
  right: 0;
  z-index: 200;
  display: flex;
  flex-direction: column;
  background:
    radial-gradient(ellipse at bottom right, rgba(232, 121, 249, 0.03) 0%, transparent 50%),
    radial-gradient(ellipse at top left, rgba(96, 165, 250, 0.02) 0%, transparent 50%),
    var(--glow-bg);
  border-top: 1px solid var(--glow-border);
  font-family: var(--glow-font);
  overflow: hidden;
  box-shadow: 0 -4px 24px rgba(232, 121, 249, 0.08);
}

/* CRT 扫描线 */
.glow-scanlines {
  pointer-events: none;
  position: absolute;
  inset: 0;
  z-index: 10;
  background: repeating-linear-gradient(
    0deg,
    transparent,
    transparent 2px,
    rgba(0, 0, 0, 0.08) 2px,
    rgba(0, 0, 0, 0.08) 4px
  );
  mix-blend-mode: multiply;
}

/* ============================================================================
   Resize Handle
   ============================================================================ */
.glow-resize-handle {
  height: 6px;
  cursor: ns-resize;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
  position: relative;
  z-index: 11;
}

.glow-resize-grip {
  width: 48px;
  height: 2px;
  border-radius: 1px;
  background: var(--glow-border);
  opacity: 0;
  transition: opacity 0.2s, background 0.2s;
}

.glow-resize-handle:hover .glow-resize-grip {
  opacity: 1;
  background: var(--glow-text-dim);
}

/* ============================================================================
   Header
   ============================================================================ */
.glow-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 12px;
  height: 34px;
  background: var(--glow-bg-header);
  border-bottom: 1px solid var(--glow-border);
  flex-shrink: 0;
  position: relative;
  z-index: 5;
}

.glow-header-left {
  display: flex;
  align-items: center;
  gap: 16px;
}

.glow-title {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 0.75rem;
  font-weight: 700;
  color: var(--glow-text-dim);
  letter-spacing: 0.05em;
  text-transform: uppercase;
}

.glow-title-icon {
  color: #e879f9;
  font-size: 0.85rem;
  text-shadow: 0 0 8px rgba(232, 121, 249, 0.6);
}

/* 猫爪动画 */
.glow-paw {
  display: inline-block;
  animation: pawBounce 2s ease-in-out infinite;
}

@keyframes pawBounce {
  0%, 100% { transform: rotate(-10deg) scale(1); }
  50% { transform: rotate(10deg) scale(1.1); }
}

/* 装饰图标 */
.glow-cat-emoji {
  font-size: 0.9rem;
  margin-left: 6px;
  animation: catBlink 3s ease-in-out infinite;
  filter: drop-shadow(0 0 4px rgba(232, 121, 249, 0.3));
}

@keyframes catBlink {
  0%, 90%, 100% { opacity: 1; }
  95% { opacity: 0.3; }
}

/* Filter Buttons */
.glow-filters {
  display: flex;
  align-items: center;
  gap: 2px;
}

.glow-filter-btn {
  display: flex;
  align-items: center;
  gap: 5px;
  padding: 3px 10px;
  font-size: 0.68rem;
  font-weight: 600;
  font-family: var(--glow-font);
  color: var(--glow-text-dim);
  background: transparent;
  border: 1px solid transparent;
  border-radius: 4px;
  cursor: pointer;
  transition: all 0.15s;
  letter-spacing: 0.03em;
}

.glow-filter-dot {
  width: 6px;
  height: 6px;
  border-radius: 50%;
  opacity: 0.5;
  transition: opacity 0.15s;
}

.glow-filter-btn:hover {
  background: rgba(255, 255, 255, 0.04);
  color: var(--glow-text);
}

.glow-filter-btn:hover .glow-filter-dot {
  opacity: 1;
}

.glow-filter-btn.active {
  background: rgba(255, 255, 255, 0.06);
  color: var(--glow-text);
  border-color: var(--glow-border);
  box-shadow: 0 0 8px var(--filter-glow, transparent);
}

.glow-filter-btn.active .glow-filter-dot {
  opacity: 1;
  box-shadow: 0 0 6px currentColor;
}

.glow-filter-count {
  font-size: 0.62rem;
  opacity: 0.5;
}

.glow-error-pill {
  font-size: 0.65rem;
  font-weight: 700;
  font-family: var(--glow-font);
  color: #fb7185;
  padding: 2px 8px;
  background: rgba(251, 113, 133, 0.1);
  border: 1px solid rgba(251, 113, 133, 0.2);
  border-radius: 10px;
  margin-left: 4px;
  text-shadow: 0 0 8px rgba(251, 113, 133, 0.4);
}

/* Header Actions */
.glow-header-right {
  display: flex;
  align-items: center;
  gap: 1px;
}

.glow-action-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 28px;
  height: 24px;
  font-size: 0.72rem;
  color: var(--glow-text-muted);
  background: transparent;
  border: none;
  border-radius: 3px;
  cursor: pointer;
  transition: all 0.15s;
  font-family: var(--glow-font);
}

.glow-action-icon {
  font-size: 0.7rem;
}

.glow-action-btn:hover {
  background: rgba(255, 255, 255, 0.06);
  color: var(--glow-text);
}

.glow-action-btn.active {
  color: #e879f9;
  text-shadow: 0 0 6px rgba(232, 121, 249, 0.5);
}

.glow-action-btn.close:hover {
  background: rgba(251, 113, 133, 0.12);
  color: #fb7185;
}

/* ============================================================================
   Body
   ============================================================================ */
.glow-body {
  flex: 1;
  overflow-y: auto;
  overflow-x: auto;
  padding: 6px 0;
  position: relative;
  z-index: 1;
}

/* 背景猫爪装饰 */
.glow-paw-decorations {
  position: absolute;
  inset: 0;
  pointer-events: none;
  z-index: 0;
  overflow: hidden;
}

.glow-deco-paw {
  position: absolute;
  font-size: 1.5rem;
  opacity: 0.05;
  filter: grayscale(30%);
  user-select: none;
}

.paw-1 {
  top: 15%;
  right: 8%;
  animation: pawFloat1 6s ease-in-out infinite;
}

.paw-2 {
  bottom: 20%;
  left: 5%;
  animation: pawFloat2 7s ease-in-out infinite 1s;
}

.paw-3 {
  top: 50%;
  left: 50%;
  animation: pawFloat3 8s ease-in-out infinite 2s;
}

@keyframes pawFloat1 {
  0%, 100% { transform: translateY(0) rotate(10deg); }
  50% { transform: translateY(-15px) rotate(-10deg); }
}

@keyframes pawFloat2 {
  0%, 100% { transform: translateY(0) rotate(-15deg); }
  50% { transform: translateY(-10px) rotate(15deg); }
}

@keyframes pawFloat3 {
  0%, 100% { transform: translate(-50%, -50%) rotate(5deg); }
  50% { transform: translate(-50%, calc(-50% - 12px)) rotate(-5deg); }
}

.glow-deco-yarn {
  position: absolute;
  bottom: 30%;
  right: 12%;
  font-size: 1.2rem;
  opacity: 0.08;
  animation: yarnRoll 5s linear infinite;
}

@keyframes yarnRoll {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}

/* Scrollbar */
.glow-body::-webkit-scrollbar {
  width: 5px;
}

.glow-body::-webkit-scrollbar-track {
  background: transparent;
}

.glow-body::-webkit-scrollbar-thumb {
  background: rgba(255, 255, 255, 0.08);
  border-radius: 3px;
}

.glow-body::-webkit-scrollbar-thumb:hover {
  background: rgba(255, 255, 255, 0.15);
}

/* Empty State */
.glow-empty {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 8px;
  height: 100%;
  color: var(--glow-text-muted);
  font-family: var(--glow-font);
}

.glow-empty-cat {
  font-size: 3.5rem;
  animation: catFloat 3s ease-in-out infinite;
  filter: drop-shadow(0 0 12px rgba(232, 121, 249, 0.3));
}

@keyframes catFloat {
  0%, 100% { transform: translateY(0) rotate(-5deg); }
  50% { transform: translateY(-10px) rotate(5deg); }
}

.glow-empty-icon {
  font-size: 1.2rem;
  opacity: 0.3;
  animation: pawWalk 3s ease-in-out infinite;
  letter-spacing: 0.5rem;
}

@keyframes pawWalk {
  0%, 100% { opacity: 0.2; }
  50% { opacity: 0.4; }
}

.glow-empty span {
  font-size: 0.78rem;
  font-weight: 600;
  letter-spacing: 0.02em;
}

.glow-empty-hint {
  font-size: 0.68rem !important;
  font-weight: 400 !important;
  opacity: 0.5;
}

/* ============================================================================
   Entry
   ============================================================================ */
.glow-entry {
  position: relative;
  display: flex;
  flex-direction: column;
  padding: 0;
  margin: 0 6px 5px 6px;
  border-radius: 4px;
  cursor: pointer;
  transition: background 0.15s, box-shadow 0.25s;
  background: transparent;
}

.glow-entry:hover {
  background: var(--glow-bg-entry-hover);
  box-shadow:
    0 0 1px var(--entry-glow, transparent),
    inset 0 0 20px var(--entry-glow-soft, transparent);
}

.glow-entry.expanded {
  background: var(--glow-bg-entry-hover);
  box-shadow:
    0 0 2px var(--entry-glow, transparent),
    0 0 16px var(--entry-glow-soft, transparent),
    inset 0 0 30px var(--entry-glow-soft, transparent);
  border: 1px solid var(--entry-glow-med, transparent);
  margin-bottom: 8px;
}

/* Left glow bar */
.glow-entry-bar {
  position: absolute;
  left: 0;
  top: 4px;
  bottom: 4px;
  width: 2px;
  border-radius: 1px;
  opacity: 0.6;
  transition: opacity 0.15s, box-shadow 0.25s, width 0.15s;
}

.glow-entry:hover .glow-entry-bar {
  opacity: 1;
  box-shadow: 0 0 6px var(--entry-glow, transparent);
  width: 3px;
}

.glow-entry.expanded .glow-entry-bar {
  opacity: 1;
  box-shadow: 0 0 10px var(--entry-glow, transparent);
  width: 3px;
}

/* 猫爪悬浮提示 */
.glow-entry::before {
  content: '🐾';
  position: absolute;
  left: -20px;
  top: 50%;
  transform: translateY(-50%);
  font-size: 0.8rem;
  opacity: 0;
  transition: opacity 0.2s, left 0.2s;
  pointer-events: none;
}

.glow-entry:hover::before {
  opacity: 0.4;
  left: -16px;
}

/* Main content line */
.glow-entry-main {
  display: grid;
  grid-template-columns: 96px 44px 84px 96px 52px 48px 72px minmax(260px, 1fr);
  align-items: center;
  gap: 8px;
  padding: 6px 10px 6px 14px;
  min-height: 28px;
  min-width: 860px;
}

.glow-time {
  font-size: 0.65rem;
  color: var(--glow-text-muted);
  width: 96px;
}

.glow-direction {
  font-size: 0.6rem;
  font-weight: 800;
  padding: 1px 6px;
  border-radius: 3px;
  letter-spacing: 0.05em;
  width: 44px;
  text-align: center;
}

.glow-direction.send {
  color: #60a5fa;
  background: rgba(96, 165, 250, 0.1);
  text-shadow: 0 0 6px rgba(96, 165, 250, 0.3);
}

.glow-direction.receive {
  color: #4ade80;
  background: rgba(74, 222, 128, 0.1);
  text-shadow: 0 0 6px rgba(74, 222, 128, 0.3);
}

.glow-direction.device {
  color: #f472b6;
  background: rgba(244, 114, 182, 0.1);
  text-shadow: 0 0 6px rgba(244, 114, 182, 0.3);
}

.glow-command {
  font-size: 0.73rem;
  font-weight: 700;
  letter-spacing: 0.02em;
  width: 84px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.glow-hex-brief {
  font-size: 0.62rem;
  color: var(--glow-text-muted);
  letter-spacing: 0.08em;
  width: 96px;
  text-align: center;
}

.glow-len {
  font-size: 0.62rem;
  color: var(--glow-text-muted);
  opacity: 0.6;
  width: 52px;
  text-align: center;
}

.glow-status {
  font-size: 0.62rem;
  font-weight: 700;
  color: #4ade80;
  padding: 0 4px;
  background: rgba(74, 222, 128, 0.08);
  border-radius: 3px;
  text-shadow: 0 0 6px rgba(74, 222, 128, 0.3);
  width: 48px;
  min-width: 48px;
  text-align: center;
  justify-self: center;
}

.glow-status.error {
  color: #fb7185;
  background: rgba(251, 113, 133, 0.08);
  text-shadow: 0 0 6px rgba(251, 113, 133, 0.3);
}

.glow-status.muted {
  color: var(--glow-text-muted);
  background: rgba(255, 255, 255, 0.04);
  text-shadow: none;
}

.glow-duration {
  font-size: 0.6rem;
  color: var(--glow-text-muted);
  opacity: 0.5;
  width: 72px;
  text-align: center;
}

.glow-parsed-brief {
  font-size: 0.68rem;
  color: var(--glow-text-dim);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  flex: 1;
  min-width: 0;
}

@media (max-width: 900px) {
  .glow-entry-main {
    grid-template-columns: 88px 40px 76px 84px 48px 44px 64px minmax(180px, 1fr);
    min-width: 720px;
    gap: 6px;
  }
  .glow-command {
    width: 76px;
  }
}

/* Error entry highlight */
.glow-entry.level-error {
  background: rgba(251, 113, 133, 0.03);
}

.glow-entry.level-error:hover {
  background: rgba(251, 113, 133, 0.06);
}

/* ============================================================================
   Expand Panel
   ============================================================================ */
.glow-expand-panel {
  padding: 6px 14px 10px 14px;
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.glow-expand-grid {
  display: flex;
  flex-wrap: wrap;
  gap: 4px 16px;
}

.glow-field {
  display: flex;
  align-items: center;
  gap: 6px;
}

.glow-field-label {
  font-size: 0.58rem;
  font-weight: 700;
  color: var(--glow-text-muted);
  letter-spacing: 0.08em;
  text-transform: uppercase;
  min-width: 32px;
}

.glow-field-value {
  font-size: 0.68rem;
  color: var(--glow-text);
}

.glow-field-value.hex {
  color: var(--entry-glow, #e879f9);
  text-shadow: 0 0 6px var(--entry-glow-soft, rgba(232, 121, 249, 0.2));
}

.glow-field-value.error {
  color: #fb7185;
  text-shadow: 0 0 6px rgba(251, 113, 133, 0.3);
}

.glow-expand-section {
  display: flex;
  flex-direction: column;
  gap: 3px;
}

.glow-section-label {
  font-size: 0.56rem;
  font-weight: 700;
  color: var(--glow-text-muted);
  letter-spacing: 0.1em;
  text-transform: uppercase;
}

.glow-section-text {
  font-size: 0.7rem;
  color: var(--glow-text);
  line-height: 1.5;
}

.glow-hex-dump {
  font-size: 0.65rem;
  color: var(--glow-text-dim);
  line-height: 1.6;
  letter-spacing: 0.12em;
  word-break: break-all;
  padding: 4px 8px;
  background: rgba(0, 0, 0, 0.3);
  border-radius: 3px;
  border: 1px solid var(--glow-border);
}

/* Expand transition */
.glow-expand-enter-active {
  transition: all 0.2s ease-out;
}
.glow-expand-leave-active {
  transition: all 0.15s ease-in;
}
.glow-expand-enter-from,
.glow-expand-leave-to {
  opacity: 0;
  max-height: 0;
  padding-top: 0;
  padding-bottom: 0;
}
.glow-expand-enter-to,
.glow-expand-leave-from {
  opacity: 1;
  max-height: 300px;
}

/* ============================================================================
   Status Bar
   ============================================================================ */
.glow-statusbar {
  position: fixed;
  bottom: 0;
  left: 0;
  right: 0;
  height: 32px;
  z-index: 201;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 8px;
  background: #0d0d14;
  border-top: 1px solid rgba(255, 255, 255, 0.06);
  font-size: 0.68rem;
  font-family: 'JetBrains Mono', 'SF Mono', 'Fira Code', monospace;
}

.glow-statusbar-toggle {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 0 14px;
  height: 100%;
  color: #6b7280;
  background: transparent;
  border: none;
  border-radius: 0;
  cursor: pointer;
  font-size: 0.72rem;
  font-weight: 600;
  font-family: inherit;
  transition: all 0.15s;
  letter-spacing: 0.03em;
}

.glow-statusbar-icon {
  font-size: 0.72rem;
}

.glow-statusbar-toggle:hover {
  background: rgba(255, 255, 255, 0.06);
  color: #c8ccd4;
}

.glow-statusbar-toggle.active {
  color: #e879f9;
  text-shadow: 0 0 6px rgba(232, 121, 249, 0.4);
}

.glow-statusbar-toggle.active .glow-statusbar-icon {
  text-shadow: 0 0 8px rgba(232, 121, 249, 0.6);
}

.glow-statusbar-count {
  font-size: 0.6rem;
  padding: 0 5px;
  background: rgba(232, 121, 249, 0.1);
  color: #e879f9;
  border-radius: 8px;
  min-width: 16px;
  text-align: center;
}

.glow-statusbar-meow {
  font-size: 0.75rem;
  margin-left: 4px;
  animation: meowBounce 1.5s ease-in-out infinite;
}

@keyframes meowBounce {
  0%, 100% { transform: scale(1); }
  50% { transform: scale(1.15); }
}

.glow-statusbar-info {
  display: flex;
  align-items: center;
  gap: 12px;
}

.glow-statusbar-errors {
  color: #fb7185;
  font-weight: 700;
  text-shadow: 0 0 6px rgba(251, 113, 133, 0.3);
}

.glow-statusbar-stats {
  display: flex;
  align-items: center;
  gap: 10px;
}

.stat-item {
  font-size: 0.62rem;
  font-weight: 600;
  letter-spacing: 0.05em;
}

.stat-tx {
  color: #60a5fa;
  text-shadow: 0 0 4px rgba(96, 165, 250, 0.2);
}

.stat-rx {
  color: #4ade80;
  text-shadow: 0 0 4px rgba(74, 222, 128, 0.2);
}

.stat-dev {
  color: #f472b6;
  text-shadow: 0 0 4px rgba(244, 114, 182, 0.2);
}

/* ============================================================================
   Settings Panel
   ============================================================================ */
.glow-settings-panel {
  padding: 8px 12px;
  background: var(--glow-bg-header);
  border-bottom: 1px solid var(--glow-border);
  display: flex;
  flex-wrap: wrap;
  gap: 10px 20px;
  align-items: center;
  flex-shrink: 0;
  position: relative;
  z-index: 5;
  font-family: var(--glow-font);
}

.glow-settings-row {
  display: flex;
  align-items: center;
  gap: 8px;
}

.glow-settings-label {
  font-size: 0.62rem;
  font-weight: 700;
  color: var(--glow-text-muted);
  letter-spacing: 0.06em;
  text-transform: uppercase;
  min-width: 50px;
  flex-shrink: 0;
}

.glow-toggle-btn {
  font-size: 0.62rem;
  font-weight: 800;
  font-family: var(--glow-font);
  padding: 2px 10px;
  border-radius: 10px;
  border: 1px solid rgba(255, 255, 255, 0.1);
  cursor: pointer;
  transition: all 0.15s;
  letter-spacing: 0.05em;
  background: rgba(251, 113, 133, 0.12);
  color: #fb7185;
}

.glow-toggle-btn.on {
  background: rgba(74, 222, 128, 0.12);
  color: #4ade80;
  border-color: rgba(74, 222, 128, 0.2);
  text-shadow: 0 0 6px rgba(74, 222, 128, 0.3);
}

.glow-mask-chips {
  display: flex;
  gap: 3px;
  flex-wrap: wrap;
}

.glow-chip {
  font-size: 0.58rem;
  font-weight: 700;
  font-family: var(--glow-font);
  padding: 2px 7px;
  border-radius: 3px;
  border: 1px solid var(--glow-border);
  background: transparent;
  color: var(--glow-text-muted);
  cursor: pointer;
  transition: all 0.12s;
  letter-spacing: 0.03em;
}

.glow-chip:hover {
  background: rgba(255, 255, 255, 0.04);
  color: var(--glow-text);
}

.glow-chip.active {
  background: rgba(232, 121, 249, 0.1);
  color: #e879f9;
  border-color: rgba(232, 121, 249, 0.25);
  text-shadow: 0 0 4px rgba(232, 121, 249, 0.2);
}

.glow-settings-btn {
  font-size: 0.6rem;
  font-weight: 700;
  font-family: var(--glow-font);
  padding: 3px 10px;
  border-radius: 3px;
  border: 1px solid var(--glow-border);
  background: transparent;
  color: var(--glow-text-dim);
  cursor: pointer;
  transition: all 0.12s;
  letter-spacing: 0.03em;
}

.glow-settings-btn:hover {
  background: rgba(255, 255, 255, 0.04);
  color: var(--glow-text);
}

.glow-settings-btn.save {
  background: rgba(74, 222, 128, 0.08);
  color: #4ade80;
  border-color: rgba(74, 222, 128, 0.2);
}

.glow-settings-btn.save:hover {
  background: rgba(74, 222, 128, 0.15);
  text-shadow: 0 0 6px rgba(74, 222, 128, 0.3);
}

/* Settings transition */
.glow-settings-enter-active { transition: all 0.2s ease-out; }
.glow-settings-leave-active { transition: all 0.15s ease-in; }
.glow-settings-enter-from,
.glow-settings-leave-to {
  opacity: 0;
  max-height: 0;
  padding-top: 0;
  padding-bottom: 0;
}
.glow-settings-enter-to,
.glow-settings-leave-from {
  opacity: 1;
  max-height: 80px;
}

/* ============================================================================
   Light Theme
   ============================================================================ */
[data-theme="light"] .glow-terminal-panel {
  --glow-bg: #f8f9fb;
  --glow-bg-header: #f0f2f5;
  --glow-bg-entry: rgba(0, 0, 0, 0.01);
  --glow-bg-entry-hover: rgba(0, 0, 0, 0.03);
  --glow-border: rgba(0, 0, 0, 0.08);
  --glow-text: #1a2744;
  --glow-text-dim: #4a5b78;
  --glow-text-muted: #8094b0;
}

[data-theme="light"] .glow-scanlines {
  display: none;
}

[data-theme="light"] .glow-statusbar {
  background: #f0f2f5;
  border-top-color: rgba(0, 0, 0, 0.08);
}

[data-theme="light"] .glow-statusbar-toggle {
  color: #8094b0;
}

[data-theme="light"] .glow-statusbar-toggle:hover {
  background: rgba(0, 0, 0, 0.04);
  color: #1a2744;
}

[data-theme="light"] .glow-hex-dump {
  background: rgba(0, 0, 0, 0.03);
  border-color: rgba(0, 0, 0, 0.06);
}

[data-theme="light"] .glow-title-icon {
  color: #3b82f6;
  text-shadow: none;
}

[data-theme="light"] .glow-statusbar-toggle.active {
  color: #3b82f6;
  text-shadow: none;
}

[data-theme="light"] .glow-statusbar-count {
  background: rgba(59, 130, 246, 0.1);
  color: #3b82f6;
}

[data-theme="light"] .glow-settings-panel {
  background: #f0f2f5;
  border-bottom-color: rgba(0, 0, 0, 0.06);
}

[data-theme="light"] .glow-toggle-btn {
  border-color: rgba(0, 0, 0, 0.1);
}

[data-theme="light"] .glow-chip {
  border-color: rgba(0, 0, 0, 0.1);
  color: #8094b0;
}

[data-theme="light"] .glow-chip.active {
  background: rgba(59, 130, 246, 0.08);
  color: #3b82f6;
  border-color: rgba(59, 130, 246, 0.2);
  text-shadow: none;
}

[data-theme="light"] .glow-settings-btn.save {
  background: rgba(34, 197, 94, 0.08);
  color: #16a34a;
  border-color: rgba(34, 197, 94, 0.2);
}
</style>
