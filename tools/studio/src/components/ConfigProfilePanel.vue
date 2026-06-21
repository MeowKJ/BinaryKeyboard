<script setup lang="ts">
import { computed, onMounted, ref } from 'vue';
import { useStudioAssetStore } from '@/stores/studioAssetStore';
import { showToast } from '@/services/toastService';

const assetStore = useStudioAssetStore();
const fileInput = ref<HTMLInputElement | null>(null);
const newProfileName = ref('');

const progressPercent = computed(() => {
  if (!assetStore.progress.total) return 0;
  return Math.round((assetStore.progress.current * 100) / assetStore.progress.total);
});

onMounted(() => {
  assetStore.refreshProfiles().catch((error) => {
    showToast('error', '配置档案加载失败', error instanceof Error ? error.message : '未知错误');
  });
});

function downloadJson(filename: string, json: string): void {
  const blob = new Blob([json], { type: 'application/json;charset=utf-8' });
  const url = URL.createObjectURL(blob);
  const anchor = document.createElement('a');
  anchor.href = url;
  anchor.download = filename;
  anchor.click();
  URL.revokeObjectURL(url);
}

function formatTime(value: string): string {
  const date = new Date(value);
  if (Number.isNaN(date.getTime())) return '';
  return date.toLocaleString('zh-CN', {
    month: '2-digit',
    day: '2-digit',
    hour: '2-digit',
    minute: '2-digit',
  });
}

async function saveProfile(): Promise<void> {
  try {
    const profile = await assetStore.saveCurrentProfile(newProfileName.value);
    newProfileName.value = '';
    showToast('success', '配置已保存', profile.name);
  } catch (error) {
    showToast('error', '保存失败', error instanceof Error ? error.message : '未知错误');
  }
}

async function applyProfile(id: string): Promise<void> {
  try {
    await assetStore.applyProfile(id);
    showToast('success', '配置已应用', '已写入当前键盘');
  } catch (error) {
    showToast('error', '应用失败', error instanceof Error ? error.message : '未知错误');
  }
}

async function deleteProfile(id: string): Promise<void> {
  try {
    await assetStore.removeProfile(id);
    showToast('success', '已删除', '配置档案已移除');
  } catch (error) {
    showToast('error', '删除失败', error instanceof Error ? error.message : '未知错误');
  }
}

function exportOne(id: string): void {
  try {
    downloadJson('meowkeyboard-config.json', assetStore.exportProfile(id));
  } catch (error) {
    showToast('error', '导出失败', error instanceof Error ? error.message : '未知错误');
  }
}

function exportAll(): void {
  downloadJson('meowkeyboard-config-library.json', assetStore.exportAllProfiles());
}

function openImport(): void {
  if (!fileInput.value) return;
  fileInput.value.value = '';
  fileInput.value.click();
}

async function onImportFile(event: Event): Promise<void> {
  const input = event.target as HTMLInputElement;
  const file = input.files?.[0];
  if (!file) return;
  try {
    const count = await assetStore.importProfileJson(await file.text());
    showToast('success', '导入成功', `${count} 个配置档案已导入`);
  } catch (error) {
    showToast('error', '导入失败', error instanceof Error ? error.message : '未知错误');
  } finally {
    input.value = '';
  }
}
</script>

<template>
  <div class="panel profile-panel">
    <div class="profile-header">
      <h3 class="panel-title">
        <i class="pi pi-objects-column"></i>
        配置档案
      </h3>
      <div class="profile-header-actions">
        <button class="profile-icon-btn" title="导入配置" type="button" @click="openImport">
          <i class="pi pi-upload"></i>
        </button>
        <button class="profile-icon-btn" title="导出全部" type="button" @click="exportAll">
          <i class="pi pi-download"></i>
        </button>
      </div>
    </div>

    <div class="profile-save-row">
      <input v-model="newProfileName" class="profile-name-input" placeholder="配置名称" />
      <button class="profile-save-btn" type="button" title="保存当前配置" @click="saveProfile">
        <i class="pi pi-save"></i>
      </button>
    </div>

    <div v-if="assetStore.progress.active" class="profile-progress">
      <div class="profile-progress-row">
        <span>{{ assetStore.progress.detail }}</span>
        <span>{{ progressPercent }}%</span>
      </div>
      <div class="profile-progress-track">
        <div class="profile-progress-fill" :style="{ width: `${progressPercent}%` }"></div>
      </div>
    </div>

    <div class="profile-list">
      <div v-for="profile in assetStore.profiles" :key="profile.id" class="profile-item">
        <button class="profile-main" type="button" title="应用到当前键盘" @click="applyProfile(profile.id)">
          <span class="profile-name">{{ profile.name }}</span>
          <span class="profile-meta">{{ profile.sourceDevice.protocolLabel }} · {{ formatTime(profile.updatedAt) }}</span>
        </button>
        <div class="profile-actions">
          <button class="profile-icon-btn" title="导出" type="button" @click="exportOne(profile.id)">
            <i class="pi pi-download"></i>
          </button>
          <button class="profile-icon-btn danger" title="删除" type="button" @click="deleteProfile(profile.id)">
            <i class="pi pi-trash"></i>
          </button>
        </div>
      </div>
      <div v-if="assetStore.profiles.length === 0" class="profile-empty">暂无配置档案</div>
    </div>

    <input ref="fileInput" class="profile-file-input" type="file" accept="application/json,.json" @change="onImportFile" />
  </div>
</template>

<style scoped>
.profile-panel {
  padding: 0.72rem;
}

.profile-header,
.profile-save-row,
.profile-progress-row,
.profile-item {
  display: flex;
  align-items: center;
  gap: 0.45rem;
}

.profile-header {
  justify-content: space-between;
  margin-bottom: 0.55rem;
}

.profile-header .panel-title {
  margin: 0;
}

.profile-header-actions,
.profile-actions {
  display: inline-flex;
  align-items: center;
  gap: 0.25rem;
}

.profile-save-row {
  margin-bottom: 0.55rem;
}

.profile-name-input {
  min-width: 0;
  flex: 1;
  height: 1.85rem;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  background: var(--c-bg-tertiary);
  color: var(--c-text-primary);
  padding: 0 0.55rem;
  font: inherit;
  font-size: 0.72rem;
  font-weight: 750;
}

.profile-save-btn,
.profile-icon-btn {
  width: 1.85rem;
  height: 1.85rem;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  background: var(--c-bg-secondary);
  color: var(--c-text-muted);
  cursor: pointer;
  transition: all var(--transition-fast);
}

.profile-save-btn {
  border-color: color-mix(in srgb, var(--c-accent) 58%, var(--c-border));
  color: var(--c-accent);
}

.profile-icon-btn:hover,
.profile-save-btn:hover {
  transform: translateY(-1px);
  border-color: var(--c-accent);
  background: var(--c-bg-hover);
  color: var(--c-accent);
}

.profile-icon-btn.danger:hover {
  border-color: var(--c-danger);
  color: var(--c-danger);
}

.profile-list {
  display: flex;
  flex-direction: column;
  gap: 0.35rem;
}

.profile-item {
  padding: 0.35rem;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  background: var(--c-bg-tertiary);
}

.profile-main {
  min-width: 0;
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 0.08rem;
  border: 0;
  padding: 0;
  background: transparent;
  color: var(--c-text-primary);
  text-align: left;
  cursor: pointer;
}

.profile-name,
.profile-meta {
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.profile-name {
  font-size: 0.74rem;
  font-weight: 850;
}

.profile-meta,
.profile-empty {
  color: var(--c-text-muted);
  font-size: 0.62rem;
  font-weight: 750;
}

.profile-empty {
  padding: 0.35rem 0;
}

.profile-progress {
  margin-bottom: 0.55rem;
}

.profile-progress-row {
  justify-content: space-between;
  color: var(--c-text-muted);
  font-size: 0.65rem;
  font-weight: 800;
  margin-bottom: 0.25rem;
}

.profile-progress-track {
  height: 0.35rem;
  border-radius: 999px;
  overflow: hidden;
  background: var(--c-bg-primary);
}

.profile-progress-fill {
  height: 100%;
  background: var(--c-accent);
  transition: width 0.18s ease;
}

.profile-file-input {
  display: none;
}
</style>
