<template>
  <div class="panel fn-panel">
    <h3 class="panel-title">
      <i class="pi pi-bolt"></i>
      FN 键设置
    </h3>
    <div class="fn-config">
      <div v-for="idx in [0, 1]" :key="idx" class="fn-group">
        <span class="fn-group-title">FN{{ idx + 1 }}</span>
        <div class="fn-item">
          <span class="fn-label">单击</span>
          <select v-model="deviceStore.fnKeyConfig.fnKeys[idx].clickAction" class="fn-select">
            <option v-for="opt in fnActionOptions" :key="opt.value" :value="opt.value">{{ opt.label }}</option>
          </select>
        </div>
        <div class="fn-item">
          <span class="fn-label">长按</span>
          <select v-model="deviceStore.fnKeyConfig.fnKeys[idx].longAction" class="fn-select">
            <option v-for="opt in fnActionOptions" :key="opt.value" :value="opt.value">{{ opt.label }}</option>
          </select>
        </div>
      </div>
      <Button label="保存 FN" icon="pi pi-check" size="small" @click="saveFn"
        class="fn-save-btn btn-primary" />
    </div>
  </div>
</template>

<script setup lang="ts">
import { useDeviceStore } from '@/stores/deviceStore';
import { showToast } from '@/services/toastService';
import { FnAction } from '@/types/protocol';

const deviceStore = useDeviceStore();

const fnActionOptions = [
  { value: FnAction.NONE, label: '无动作' },
  { value: FnAction.MODE_TOGGLE, label: '切换模式' },
  { value: FnAction.BLE_CLEAR_BONDS, label: '清除配对' },
  { value: FnAction.RGB_TOGGLE, label: 'RGB 开关' },
  { value: FnAction.RGB_MODE_NEXT, label: 'RGB 下一模式' },
  { value: FnAction.RGB_MODE_PREV, label: 'RGB 上一模式' },
  { value: FnAction.RGB_BRIGHT_UP, label: '亮度+' },
  { value: FnAction.RGB_BRIGHT_DOWN, label: '亮度-' },
  { value: FnAction.LAYER_NEXT, label: '下一层' },
  { value: FnAction.LAYER_PREV, label: '上一层' },
  { value: FnAction.SLEEP, label: '休眠' },
];

async function saveFn() {
  try {
    await deviceStore.saveFnKeyConfig();
    showToast('success', 'FN 键已保存', 'FN 键配置已保存到设备');
  } catch (error) {
    showToast('error', '保存失败', error instanceof Error ? error.message : '未知错误');
  }
}
</script>

<style scoped>
.fn-config {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.fn-group {
  display: flex;
  flex-direction: column;
  gap: 6px;
  padding: 8px;
  border-radius: 6px;
  background: var(--surface-hover, rgba(255, 255, 255, 0.03));
}

.fn-group-title {
  font-weight: 600;
  font-size: 0.85rem;
  color: var(--primary-color);
}

.fn-item {
  display: flex;
  align-items: center;
  gap: 8px;
}

.fn-label {
  font-size: 0.8rem;
  color: var(--text-secondary);
  min-width: 32px;
}

.fn-select {
  flex: 1;
  padding: 5px 8px;
  border-radius: 6px;
  border: 1px solid var(--border-color);
  background: var(--surface-card);
  color: var(--text-primary);
  font-size: 0.8rem;
}

.fn-save-btn {
  margin-top: 4px;
}
</style>
