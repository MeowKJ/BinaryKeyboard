<template>
  <section class="key-test-panel" :class="{ active: enabled }">
    <div class="key-test-header">
      <div class="key-test-title">
        <i class="pi pi-circle-fill" :class="{ live: enabled }"></i>
        <span>按键测试</span>
      </div>
      <div class="key-test-actions">
        <button class="icon-button" :disabled="!events.length" title="清空" @click="clearEvents">
          <i class="pi pi-trash"></i>
        </button>
        <button class="toggle-button" :class="{ active: enabled }" :disabled="busy || !canTest" @click="toggle">
          <i :class="enabled ? 'pi pi-stop' : 'pi pi-play'"></i>
          <span>{{ enabled ? '停止' : '开始' }}</span>
        </button>
      </div>
    </div>

    <div class="key-test-summary">
      <div class="summary-cell">
        <span class="summary-label">最近</span>
        <strong>{{ latestLabel }}</strong>
      </div>
      <div class="summary-cell">
        <span class="summary-label">按下中</span>
        <strong>{{ pressedCount }}</strong>
      </div>
      <div class="summary-cell">
        <span class="summary-label">事件</span>
        <strong>{{ events.length }}</strong>
      </div>
    </div>

    <div class="event-list">
      <div v-if="!canTest" class="empty-row">当前设备不支持日志</div>
      <div v-else-if="events.length === 0" class="empty-row">{{ enabled ? '等待事件' : '未开始' }}</div>
      <div v-for="event in events" :key="event.id" class="event-row" :class="{ pressed: event.pressed }">
        <span class="event-index">K{{ event.keyIndex }}</span>
        <span class="event-state">{{ event.pressed ? '按下' : '释放' }}</span>
        <span class="event-action">{{ event.actionLabel }}</span>
        <span class="event-time">{{ formatTime(event.at) }}</span>
      </div>
    </div>
  </section>
</template>

<script setup lang="ts">
import { computed, onBeforeUnmount, ref } from 'vue';
import { hidService } from '@/services/HidService';
import { showToast } from '@/services/toastService';
import { useDeviceStore } from '@/stores/deviceStore';
import { ActionType, Command, LogCategory, type LogConfig } from '@/types/protocol';
import { getConsumerName } from '@/utils/consumer';
import { getKeycodeName } from '@/utils/keycodes';

export interface KeyTestEvent {
  id: number;
  keyIndex: number;
  pressed: boolean;
  actionType: number;
  param: number;
  actionLabel: string;
  at: number;
}

const emit = defineEmits<{
  keyEvent: [event: KeyTestEvent];
  reset: [];
}>();

const deviceStore = useDeviceStore();
const enabled = ref(false);
const busy = ref(false);
const events = ref<KeyTestEvent[]>([]);
const pressedKeys = ref(new Set<number>());
const previousLogConfig = ref<LogConfig | null>(null);
let unsubscribe: (() => void) | null = null;
let nextEventId = 1;

const canTest = computed(() => deviceStore.isConnected && deviceStore.supportsLogs);
const pressedCount = computed(() => pressedKeys.value.size);
const latestLabel = computed(() => {
  const latest = events.value[0];
  return latest ? `K${latest.keyIndex} ${latest.pressed ? '按下' : '释放'}` : '-';
});

function actionLabel(type: number, param: number): string {
  switch (type) {
    case ActionType.NONE:
      return '无动作';
    case ActionType.KEYBOARD:
      return getKeycodeName(param, 0);
    case ActionType.MOUSE_BTN:
      return `鼠标 0x${param.toString(16).padStart(2, '0')}`;
    case ActionType.MOUSE_WHEEL:
      return param === 1 ? '滚轮上' : param === 2 ? '滚轮下' : param === 3 ? '中键' : '滚轮';
    case ActionType.CONSUMER:
      return getConsumerName(param) || `媒体 0x${param.toString(16).padStart(2, '0')}`;
    case ActionType.MACRO:
      return `宏 ${param + 1}`;
    case ActionType.LAYER:
      return `层 ${param + 1}`;
    default:
      return `动作 0x${type.toString(16).padStart(2, '0')}`;
  }
}

function parseKeyEvent(frame: Uint8Array): KeyTestEvent | null {
  if (frame[0] !== Command.LOG || frame[1] !== LogCategory.KEY_EVENT || frame[2] < 4) {
    return null;
  }

  const keyIndex = frame[3];
  const pressed = frame[4] !== 0;
  const type = frame[5];
  const param = frame[6];

  return {
    id: nextEventId++,
    keyIndex,
    pressed,
    actionType: type,
    param,
    actionLabel: actionLabel(type, param),
    at: Date.now(),
  };
}

function handleDeviceEvent(event: { frame: Uint8Array }): void {
  const keyEvent = parseKeyEvent(event.frame);
  if (!keyEvent) return;

  const nextPressed = new Set(pressedKeys.value);
  if (keyEvent.pressed) {
    nextPressed.add(keyEvent.keyIndex);
  } else {
    nextPressed.delete(keyEvent.keyIndex);
  }
  pressedKeys.value = nextPressed;

  events.value = [keyEvent, ...events.value].slice(0, 12);
  emit('keyEvent', keyEvent);
}

async function start(): Promise<void> {
  if (!canTest.value || busy.value || enabled.value) return;
  busy.value = true;
  try {
    previousLogConfig.value = await hidService.getLogConfig().catch(() => null);
    await hidService.setLogConfig({ enabled: true });
    unsubscribe = hidService.onDeviceEvent(handleDeviceEvent);
    enabled.value = true;
  } catch (error) {
    showToast('error', '测试启动失败', error instanceof Error ? error.message : '无法打开日志');
  } finally {
    busy.value = false;
  }
}

async function stop(): Promise<void> {
  if (busy.value || !enabled.value) return;
  busy.value = true;
  try {
    unsubscribe?.();
    unsubscribe = null;
    enabled.value = false;
    pressedKeys.value = new Set();
    emit('reset');
    if (previousLogConfig.value) {
      await hidService.setLogConfig(previousLogConfig.value).catch(() => {});
      previousLogConfig.value = null;
    }
  } finally {
    busy.value = false;
  }
}

function toggle(): void {
  void (enabled.value ? stop() : start());
}

function clearEvents(): void {
  events.value = [];
  pressedKeys.value = new Set();
  emit('reset');
}

function formatTime(ts: number): string {
  const d = new Date(ts);
  return `${d.toLocaleTimeString('zh-CN', { hour12: false })}.${String(d.getMilliseconds()).padStart(3, '0')}`;
}

onBeforeUnmount(() => {
  if (enabled.value) {
    void stop();
  }
});
</script>

<style scoped>
.key-test-panel {
  width: min(560px, calc(100vw - var(--sidebar-width) - 5rem));
  margin-top: 0.9rem;
  background: color-mix(in srgb, var(--c-bg-secondary) 92%, transparent);
  border: 1px solid var(--c-border);
  border-radius: var(--radius-lg);
  overflow: hidden;
  position: relative;
  z-index: 1;
  pointer-events: auto;
}

.key-test-panel.active {
  border-color: color-mix(in srgb, var(--c-success) 48%, var(--c-border));
}

.key-test-header,
.key-test-summary,
.event-row {
  display: grid;
  grid-template-columns: 1fr auto;
  align-items: center;
}

.key-test-header {
  padding: 0.75rem 0.9rem;
  background: var(--c-bg-tertiary);
  border-bottom: 1px solid var(--c-border);
}

.key-test-title,
.key-test-actions,
.event-row,
.summary-cell {
  display: flex;
  align-items: center;
}

.key-test-title {
  gap: 0.5rem;
  font-size: 0.9rem;
  font-weight: 800;
  color: var(--c-text-primary);
}

.key-test-title .pi-circle-fill {
  font-size: 0.5rem;
  color: var(--c-text-muted);
}

.key-test-title .pi-circle-fill.live {
  color: var(--c-success);
  filter: drop-shadow(0 0 8px rgba(74, 222, 128, 0.55));
}

.key-test-actions {
  gap: 0.5rem;
}

.icon-button,
.toggle-button {
  border: 1px solid var(--c-border);
  background: var(--c-bg-secondary);
  color: var(--c-text-secondary);
  border-radius: var(--radius-sm);
  height: 32px;
  cursor: pointer;
  transition: all var(--transition-fast);
}

.icon-button {
  width: 32px;
}

.toggle-button {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 0.4rem;
  padding: 0 0.75rem;
  font-weight: 800;
}

.icon-button:hover,
.toggle-button:hover {
  border-color: var(--c-accent);
  color: var(--c-text-primary);
}

.toggle-button.active {
  border-color: var(--c-success);
  color: var(--c-success);
}

.icon-button:disabled,
.toggle-button:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.key-test-summary {
  grid-template-columns: repeat(3, 1fr);
  gap: 1px;
  background: var(--c-border);
}

.summary-cell {
  min-width: 0;
  justify-content: space-between;
  gap: 0.6rem;
  padding: 0.65rem 0.8rem;
  background: var(--c-bg-secondary);
}

.summary-label {
  color: var(--c-text-muted);
  font-size: 0.72rem;
  font-weight: 700;
}

.summary-cell strong {
  min-width: 0;
  color: var(--c-text-primary);
  font-size: 0.82rem;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.event-list {
  max-height: 160px;
  overflow-y: auto;
}

.event-row {
  grid-template-columns: 54px 56px minmax(0, 1fr) 92px;
  gap: 0.55rem;
  padding: 0.52rem 0.8rem;
  border-top: 1px solid var(--c-border-light);
  font-size: 0.78rem;
}

.event-row.pressed {
  background: color-mix(in srgb, var(--c-success) 9%, transparent);
}

.event-index {
  color: var(--c-accent);
  font-weight: 900;
}

.event-state {
  color: var(--c-text-secondary);
  font-weight: 800;
}

.event-action {
  min-width: 0;
  color: var(--c-text-primary);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.event-time {
  color: var(--c-text-muted);
  text-align: right;
  font-variant-numeric: tabular-nums;
}

.empty-row {
  padding: 1rem;
  color: var(--c-text-muted);
  text-align: center;
  font-size: 0.82rem;
}
</style>
