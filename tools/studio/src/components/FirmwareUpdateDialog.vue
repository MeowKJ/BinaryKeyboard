<template>
  <Dialog
    v-model:visible="visible"
    header="固件更新"
    :modal="true"
    :closable="!isUpdating"
    :close-on-escape="!isUpdating"
    :style="{ width: '460px', maxWidth: 'calc(100vw - 2rem)' }"
  >
    <div class="iap-dialog">
      <!-- 更新前确认 -->
      <template v-if="progress.stage === 'idle'">
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
          更新过程中请勿断开设备或关闭页面
        </p>
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
          <span>固件更新成功！设备已重启。</span>
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
  </Dialog>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue';
import Dialog from 'primevue/dialog';
import Button from 'primevue/button';
import ProgressBar from 'primevue/progressbar';
import { hidService } from '@/services/HidService';
import { performIapUpdate, type IapProgress } from '@/services/iapService';
import { useDeviceStore } from '@/stores/deviceStore';
import { KeyboardType } from '@/types/protocol';

const props = defineProps<{
  targetVersion: string;
}>();

const visible = defineModel<boolean>('visible', { default: false });

const deviceStore = useDeviceStore();

const currentVersion = computed(() => deviceStore.firmwareVersionLabel);
const targetVersionLabel = computed(() =>
  props.targetVersion === 'dev' ? 'dev' : `v${props.targetVersion}`
);
const targetLabel = computed(() =>
  props.targetVersion && targetVersionLabel.value !== currentVersion.value ? '最新版本' : '目标版本'
);

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
