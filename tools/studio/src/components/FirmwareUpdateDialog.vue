<template>
  <StudioDialog
    v-model:visible="visible"
    size="sm"
    :header="dialogTitle"
    :closable="!isUpdating"
    :close-on-escape="!isUpdating"
    :dismissable-mask="!isUpdating"
    class="firmware-update-dialog"
  >
    <div class="iap-dialog">
      <!-- 更新前确认 -->
      <template v-if="progress.stage === 'idle'">
        <div v-if="isCh592" class="iap-chip-brief">
          <div class="iap-chip-icon">592</div>
          <div class="iap-chip-copy">
            <strong>CH592F IAP 刷写</strong>
            <span>{{ modelLabel }} 固件 · {{ iapModeLabel }}</span>
          </div>
        </div>

        <div class="iap-versions">
          <div class="iap-ver-row">
            <span class="iap-ver-label">当前版本</span>
            <span class="iap-ver-value">{{ currentVersion }}</span>
          </div>
          <div class="iap-ver-arrow">→</div>
          <div class="iap-ver-row">
            <span class="iap-ver-label">{{ targetLabel }}</span>
            <span class="iap-ver-value iap-ver-new">{{ targetVersionLabel }}</span>
          </div>
        </div>
        <p class="iap-warning">
          <i class="pi pi-exclamation-triangle"></i>
          {{ warningText }}
        </p>
        <div v-if="isCh592" class="iap-notes">
          <div class="iap-note">
            <i class="pi pi-cloud-download"></i>
            <span>自动下载对应型号的 OTA bin，不需要手动进入 BOOT。</span>
          </div>
          <div class="iap-note">
            <i class="pi pi-refresh"></i>
            <span>刷写完成后会短暂断连，Studio 会等待设备重新枚举。</span>
          </div>
        </div>
      </template>

      <!-- 更新中 -->
      <template v-else-if="progress.stage !== 'error' && progress.stage !== 'done'">
        <div class="iap-progress-info">
          <span class="iap-stage-text">{{ progress.message }}</span>
        </div>
        <ProgressBar :value="progress.percent" :show-value="true" />
      </template>

      <!-- 完成 -->
      <template v-else-if="progress.stage === 'done'">
        <div class="iap-result iap-success">
          <i class="pi pi-check-circle"></i>
          <span>{{ successText }}</span>
        </div>
      </template>

      <!-- 错误 -->
      <template v-else-if="progress.stage === 'error'">
        <div class="iap-result iap-error">
          <i class="pi pi-times-circle"></i>
          <div class="iap-result-copy">
            <div class="iap-result-title">{{ progress.message || '更新失败' }}</div>
            <div v-if="progress.error && progress.error !== progress.message" class="iap-result-detail">
              {{ progress.error }}
            </div>
            <div v-if="progress.hint" class="iap-result-hint">
              <i class="pi pi-info-circle"></i>
              <span>{{ progress.hint }}</span>
            </div>
          </div>
        </div>
      </template>
    </div>

    <template #footer>
      <div class="iap-footer">
        <Button
          v-if="progress.stage === 'idle'"
          label="开始更新"
          icon="pi pi-download"
          @click="startUpdate"
        />
        <Button
          v-if="progress.stage === 'idle'"
          label="取消"
          severity="secondary"
          @click="visible = false"
        />
        <Button
          v-if="progress.stage === 'done' || progress.stage === 'error'"
          label="关闭"
          @click="visible = false"
        />
      </div>
    </template>
  </StudioDialog>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue';
import Button from 'primevue/button';
import ProgressBar from 'primevue/progressbar';
import StudioDialog from '@/components/StudioDialog.vue';
import { hidService } from '@/services/HidService';
import { performIapUpdate, type IapProgress } from '@/services/iapService';
import { useDeviceStore } from '@/stores/deviceStore';
import { DeviceProtocol, KeyboardType } from '@/types/protocol';

const props = defineProps<{
  targetVersion: string;
}>();

const visible = defineModel<boolean>('visible', { default: false });

const deviceStore = useDeviceStore();

const currentVersion = computed(() => deviceStore.firmwareVersionLabel);
const targetVersionLabel = computed(() =>
  props.targetVersion === 'dev' ? 'dev' : `v${props.targetVersion}`
);
const isCh592 = computed(() => deviceStore.deviceInfo?.protocol === DeviceProtocol.CH592);
const dialogTitle = computed(() => (isCh592.value ? 'CH592F 固件更新' : '固件更新'));
const targetLabel = computed(() =>
  props.targetVersion && targetVersionLabel.value !== currentVersion.value ? '最新版本' : '目标版本'
);
const modelLabel = computed(() => `CH592F-${resolveModel()}`);
const iapModeLabel = computed(() => {
  if (deviceStore.isDevFirmware) return `刷写正式版 ${targetVersionLabel.value}`;
  if (targetVersionLabel.value === currentVersion.value) return '重刷当前版本';
  return `${currentVersion.value} → ${targetVersionLabel.value}`;
});
const warningText = computed(() => {
  if (isCh592.value) return '刷写过程中请保持连接，不要关闭 Studio 或拔出设备。';
  return '更新过程中请勿断开设备或关闭页面';
});
const successText = computed(() => {
  if (isCh592.value) return 'CH592F 固件写入成功，设备已重启。若页面仍显示断开，请等待自动重连或重新连接。';
  return '固件更新成功！设备已重启。';
});

const progress = ref<IapProgress>({
  stage: 'idle',
  percent: 0,
  message: '',
});

const isUpdating = computed(() =>
  progress.value.stage !== 'idle' &&
  progress.value.stage !== 'done' &&
  progress.value.stage !== 'error'
);

/** 将 KeyboardType 映射为固件文件名中的型号 */
function resolveModel(): string {
  const type = deviceStore.deviceInfo?.keyboardType;
  switch (type) {
    case KeyboardType.FIVE_KEYS: return '5KEY';
    case KeyboardType.KNOB: return 'KNOB';
    default: return '5KEY';
  }
}

async function startUpdate() {
  const transport = hidService.getIapTransport();
  if (!transport) {
    progress.value = { stage: 'error', percent: 0, message: '设备未连接', error: '设备未连接' };
    return;
  }

  const model = resolveModel();
  progress.value = { stage: 'downloading', percent: 0, message: '准备中...' };
  deviceStore.iapInProgress = true;

  try {
    await performIapUpdate(
      transport,
      props.targetVersion,
      model,
      (p) => { progress.value = p; },
    );
    progress.value = { stage: 'done', percent: 100, message: '更新完成' };
  } catch {
    // error is already set by performIapUpdate callback
    if (progress.value.stage !== 'error') {
      progress.value = { stage: 'error', percent: 0, message: '更新失败', error: '未知错误' };
    }
  } finally {
    deviceStore.iapInProgress = false;
  }
}
</script>

<style scoped>
.iap-dialog {
  display: flex;
  flex-direction: column;
  gap: 1rem;
}

.iap-chip-brief {
  display: grid;
  grid-template-columns: 3rem minmax(0, 1fr);
  align-items: center;
  gap: 0.75rem;
  padding: 0.8rem 0.9rem;
  border-radius: var(--radius-sm);
  border: 1px solid color-mix(in srgb, var(--c-accent) 42%, var(--c-border));
  background:
    linear-gradient(135deg, color-mix(in srgb, var(--c-accent) 14%, transparent), transparent),
    var(--c-bg-primary);
}

.iap-chip-icon {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 2.65rem;
  height: 2.65rem;
  border-radius: var(--radius-sm);
  border: 1px solid color-mix(in srgb, var(--c-accent) 54%, var(--c-border));
  background: var(--c-accent-soft);
  color: var(--c-accent-light);
  font-family: 'JetBrains Mono', 'Fira Code', monospace;
  font-size: 0.82rem;
  font-weight: 900;
}

.iap-chip-copy {
  display: flex;
  flex-direction: column;
  gap: 0.18rem;
  min-width: 0;
}

.iap-chip-copy strong {
  color: var(--c-text-primary);
  font-size: 0.9rem;
  font-weight: 900;
}

.iap-chip-copy span {
  color: var(--c-text-secondary);
  font-size: 0.76rem;
  font-weight: 800;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.iap-versions {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 1rem;
  padding: 1rem;
  border-radius: var(--radius-sm);
  background: var(--c-bg-primary);
  border: 1px solid var(--c-border-light);
}

.iap-ver-row {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 0.25rem;
}

.iap-ver-label {
  font-size: 0.75rem;
  color: var(--c-text-secondary);
}

.iap-ver-value {
  font-size: 1rem;
  font-weight: 700;
  font-family: 'JetBrains Mono', 'Fira Code', monospace;
}

.iap-ver-new {
  color: #4ade80;
}

.iap-ver-arrow {
  font-size: 1.2rem;
  color: var(--c-text-secondary);
}

.iap-warning {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  font-size: 0.8rem;
  color: #fbbf24;
  margin: 0;
}

.iap-notes {
  display: grid;
  gap: 0.42rem;
  padding: 0.75rem;
  border-radius: var(--radius-sm);
  border: 1px solid var(--c-border-light);
  background: color-mix(in srgb, var(--c-bg-primary) 82%, transparent);
}

.iap-note {
  display: flex;
  align-items: flex-start;
  gap: 0.48rem;
  color: var(--c-text-secondary);
  font-size: 0.76rem;
  line-height: 1.45;
}

.iap-note i {
  margin-top: 0.1rem;
  color: var(--c-accent-light);
  font-size: 0.76rem;
}

.iap-progress-info {
  text-align: center;
}

.iap-stage-text {
  font-size: 0.85rem;
  color: var(--c-text-primary);
}

.iap-result {
  display: flex;
  align-items: flex-start;
  gap: 0.5rem;
  padding: 1rem;
  border-radius: var(--radius-sm);
}

.iap-result-copy {
  display: flex;
  flex-direction: column;
  gap: 0.5rem;
  min-width: 0;
}

.iap-result-title {
  font-weight: 700;
  line-height: 1.4;
}

.iap-result-detail {
  font-size: 0.9rem;
  line-height: 1.5;
  color: var(--c-text-secondary);
  overflow-wrap: anywhere;
}

.iap-result-hint {
  display: flex;
  align-items: flex-start;
  gap: 0.4rem;
  padding-top: 0.25rem;
  font-size: 0.82rem;
  line-height: 1.45;
  color: #fbbf24;
}

.iap-success {
  background: rgba(74, 222, 128, 0.1);
  color: #4ade80;
  border: 1px solid rgba(74, 222, 128, 0.3);
}

.iap-error {
  background: rgba(248, 113, 113, 0.1);
  color: #f87171;
  border: 1px solid rgba(248, 113, 113, 0.3);
}

.iap-footer {
  display: flex;
  gap: 0.5rem;
  justify-content: flex-end;
}
</style>
