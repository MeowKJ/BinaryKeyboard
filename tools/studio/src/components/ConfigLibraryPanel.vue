<script setup lang="ts">
import { computed, onMounted, ref, watch } from 'vue';
import { useConfirm } from 'primevue/useconfirm';
import StudioDialog from '@/components/StudioDialog.vue';
import { showToast } from '@/services/toastService';
import {
  createConfigItem,
  deleteConfigItem,
  downloadConfigBackup,
  importBackupTextAsConfig,
  listConfigItems,
  readFileAsText,
  renameConfigItem,
} from '@/services/configLibraryService';
import {
  applyConfigBackupToDevice,
  checkConfigCompatibility,
  createConfigBackupFromDevice,
} from '@/services/configSnapshotService';
import type { ConfigLibraryItem } from '@/types/configBackup';
import { useDeviceStore } from '@/stores/deviceStore';

const props = defineProps<{ visible: boolean }>();
const emit = defineEmits<{ 'update:visible': [value: boolean] }>();

const dialogVisible = computed({
  get: () => props.visible,
  set: (value: boolean) => emit('update:visible', value),
});

const deviceStore = useDeviceStore();
const confirm = useConfirm();
const configs = ref<ConfigLibraryItem[]>([]);
const selectedConfigIndex = ref<number | null>(null);
const isBusy = ref(false);
const fileInputEl = ref<HTMLInputElement | null>(null);
const nameDialogVisible = ref(false);
const nameDialogMode = ref<'save' | 'rename'>('save');
const nameDialogValue = ref('');

const selectedConfig = computed(() => configs.value.find((item) => item.index === selectedConfigIndex.value) ?? null);
const configCount = computed(() => configs.value.length);
const nameDialogTitle = computed(() => nameDialogMode.value === 'save' ? '保存配置' : '重命名配置');
const nameDialogAction = computed(() => nameDialogMode.value === 'save' ? '保存' : '重命名');
const libraryHint = '保存当前键盘配置，包含键位、灯光、FN、系统模式和宏内容；恢复时会先做设备兼容校验。';
const deviceLabel = computed(() => {
  const info = deviceStore.deviceInfo;
  if (!info) return '未连接';
  const version = deviceStore.firmwareVersionLabel;
  return `${info.protocolLabel} · ${deviceStore.keyboardTypeName} · ${version}`;
});

function formatTime(value?: string): string {
  if (!value) return 'empty';
  return new Date(value).toLocaleString();
}

function configLabel(index: number): string {
  return `配置 ${index + 1}`;
}

function moduleSummary(item: ConfigLibraryItem | null): string {
  if (!item) return '未选择配置';
  const config = item.backup.config;
  const parts = ['键位'];
  if (config.rgb) parts.push('RGB');
  if (config.fnKeys) parts.push('FN');
  if (config.osMode) parts.push('OS');
  if (config.log) parts.push('日志');
  if (config.macros?.length) parts.push(`宏 ${config.macros.length}`);
  return parts.join(' / ');
}

async function reloadConfigs(selectIndex = selectedConfigIndex.value): Promise<void> {
  configs.value = await listConfigItems();
  if (configs.value.length === 0) {
    selectedConfigIndex.value = null;
    return;
  }
  selectedConfigIndex.value = configs.value.some((item) => item.index === selectIndex)
    ? selectIndex
    : configs.value[0].index;
}

function errorText(error: unknown): string {
  return error instanceof Error ? error.message : String(error || 'Unknown error');
}

async function runBusy(task: () => Promise<void>, failureTitle = '操作失败'): Promise<void> {
  if (isBusy.value) return;
  isBusy.value = true;
  try {
    await task();
  } catch (error) {
    showToast('error', failureTitle, errorText(error));
  } finally {
    isBusy.value = false;
  }
}

function openSaveNameDialog(): void {
  nameDialogMode.value = 'save';
  nameDialogValue.value = `${deviceStore.keyboardTypeName} 配置`;
  nameDialogVisible.value = true;
}

function openRenameDialog(): void {
  const item = selectedConfig.value;
  if (!item) return;
  nameDialogMode.value = 'rename';
  nameDialogValue.value = item.name;
  nameDialogVisible.value = true;
}

async function confirmNameDialog(): Promise<void> {
  const name = nameDialogValue.value.trim();
  if (!name) {
    showToast('warn', '名称为空', '请输入配置名称');
    return;
  }

  nameDialogVisible.value = false;
  if (nameDialogMode.value === 'save') {
    await saveCurrentAsConfig(name);
  } else {
    await renameSelected(name);
  }
}

async function saveCurrentAsConfig(name: string): Promise<void> {
  await runBusy(async () => {
    const backup = await createConfigBackupFromDevice(name);
    const item = await createConfigItem(backup, name);
    await reloadConfigs(item.index);
    showToast('success', '已保存到配置库', name);
  });
}

async function applySelectedToDevice(): Promise<void> {
  const item = selectedConfig.value;
  if (!item) {
    showToast('warn', '未选择配置', '请选择一个配置');
    return;
  }

  const compatibility = checkConfigCompatibility(item.backup);
  if (!compatibility.ok) {
    showToast('error', '配置不兼容', compatibility.blocking.join('；'));
    return;
  }

  const warningText = compatibility.warnings.length ? `\n\n提示：${compatibility.warnings.join('；')}` : '';
  confirm.require({
    header: '恢复配置',
    message: `将「${item.name}」写入当前键盘。${warningText}`,
    icon: 'pi pi-exclamation-triangle',
    acceptClass: 'p-button-danger',
    acceptLabel: '恢复配置',
    rejectLabel: '取消',
    accept: async () => {
      await runBusy(async () => {
        await applyConfigBackupToDevice(item.backup);
        showToast('success', '配置已恢复', `已从「${item.name}」写入当前键盘`);
      });
    },
  });
}

async function renameSelected(name: string): Promise<void> {
  const item = selectedConfig.value;
  if (!item) return;
  await runBusy(async () => {
    await renameConfigItem(item.index, name);
    await reloadConfigs(item.index);
    showToast('success', '已重命名', name);
  });
}

async function deleteSelected(): Promise<void> {
  const item = selectedConfig.value;
  if (!item) return;
  confirm.require({
    header: '删除配置',
    message: `删除「${item.name}」？`,
    icon: 'pi pi-trash',
    acceptClass: 'p-button-danger',
    acceptLabel: '删除',
    rejectLabel: '取消',
    accept: async () => {
      await runBusy(async () => {
        await deleteConfigItem(item.index);
        await reloadConfigs();
        showToast('success', '已删除配置', item.name);
      });
    },
  });
}

async function exportSelected(): Promise<void> {
  const item = selectedConfig.value;
  if (!item) {
    showToast('warn', '未选择配置', '没有可导出的配置');
    return;
  }
  await runBusy(async () => {
    await downloadConfigBackup(item.backup, item.name);
    showToast('success', '已导出配置文件', item.name);
  }, '导出失败');
}

function chooseImportFile(): void {
  fileInputEl.value?.click();
}

async function importFile(event: Event): Promise<void> {
  const input = event.target as HTMLInputElement;
  const file = input.files?.[0];
  input.value = '';
  if (!file) return;

  await runBusy(async () => {
    const text = await readFileAsText(file);
    const item = await importBackupTextAsConfig(text);
    await reloadConfigs(item.index);
    showToast('success', '已导入到配置库', item.name);
  });
}

watch(() => props.visible, (visible) => {
  if (visible) void reloadConfigs();
});

onMounted(() => {
  void reloadConfigs();
});
</script>

<template>
  <StudioDialog
    v-model:visible="dialogVisible"
    size="md"
    class="config-library-dialog"
    :showHeader="false"
  >
    <div class="library-shell">
      <header class="library-head">
        <div>
          <span class="library-kicker">CONFIG LIBRARY</span>
          <h2>配置库</h2>
          <p>{{ deviceLabel }}</p>
        </div>
        <button class="icon-btn" type="button" title="关闭" @click="dialogVisible = false">
          <i class="pi pi-times"></i>
        </button>
      </header>

      <div class="library-summary">
        <p>{{ libraryHint }}</p>
        <code>{{ configCount }}</code>
      </div>

      <div class="library-body">
        <aside class="config-list">
          <button
            v-for="item in configs"
            :key="item.id"
            type="button"
            class="config-item"
            :class="{ active: selectedConfigIndex === item.index }"
            @click="selectedConfigIndex = item.index"
          >
            <span class="config-index">{{ String(item.index + 1).padStart(2, '0') }}</span>
            <span class="config-main">
              <strong>{{ item.name }}</strong>
              <small>{{ moduleSummary(item) }}</small>
            </span>
            <i class="pi pi-database"></i>
          </button>
          <div v-if="configs.length === 0" class="config-empty-list">
            还没有保存的配置
          </div>
        </aside>

        <section class="config-detail">
          <div class="detail-card" :class="{ empty: !selectedConfig }">
            <div class="detail-topline">
              <span>{{ selectedConfig ? configLabel(selectedConfig.index) : '未选择' }}</span>
              <code>{{ configCount }}</code>
            </div>
            <template v-if="selectedConfig">
              <h3>{{ selectedConfig.name }}</h3>
              <dl>
                <div>
                  <dt>设备</dt>
                  <dd>{{ selectedConfig.backup.device.protocolLabel }} · {{ selectedConfig.backup.device.chipFamily }}</dd>
                </div>
                <div>
                  <dt>型号</dt>
                  <dd>type {{ selectedConfig.backup.device.keyboardType }} · {{ selectedConfig.backup.device.actualKeyCount }} keys</dd>
                </div>
                <div>
                  <dt>固件</dt>
                  <dd>{{ selectedConfig.backup.device.firmwareVersion }}</dd>
                </div>
                <div>
                  <dt>模块</dt>
                  <dd>{{ moduleSummary(selectedConfig) }}</dd>
                </div>
                <div>
                  <dt>更新</dt>
                  <dd>{{ formatTime(selectedConfig.updatedAt) }}</dd>
                </div>
              </dl>
            </template>
            <template v-else>
              <h3>配置库为空</h3>
              <p>保存当前键盘配置，或从 JSON 文件导入配置。</p>
            </template>
          </div>

          <div class="action-grid">
            <button type="button" class="action primary" :disabled="isBusy" @click="openSaveNameDialog">
              <i class="pi pi-save"></i>
              <span>保存配置</span>
            </button>
            <button type="button" class="action danger" :disabled="isBusy || !selectedConfig" @click="applySelectedToDevice">
              <i class="pi pi-upload"></i>
              <span>恢复配置</span>
            </button>
            <button type="button" class="action" :disabled="isBusy || !selectedConfig" @click="exportSelected">
              <i class="pi pi-download"></i>
              <span>导出配置</span>
            </button>
            <button type="button" class="action" :disabled="isBusy" @click="chooseImportFile">
              <i class="pi pi-file-import"></i>
              <span>导入配置</span>
            </button>
            <button type="button" class="action" :disabled="isBusy || !selectedConfig" @click="openRenameDialog">
              <i class="pi pi-pencil"></i>
              <span>重命名</span>
            </button>
            <button type="button" class="action" :disabled="isBusy || !selectedConfig" @click="deleteSelected">
              <i class="pi pi-trash"></i>
              <span>删除</span>
            </button>
          </div>

          <input ref="fileInputEl" type="file" accept=".json,.binarykeyboard-config.json,application/json" hidden @change="importFile" />
        </section>
      </div>

      <div v-if="isBusy" class="busy-line">
        <i class="pi pi-spin pi-spinner"></i>
        <span>处理中...</span>
      </div>
    </div>
  </StudioDialog>

  <StudioDialog
    v-model:visible="nameDialogVisible"
    size="sm"
    :header="nameDialogTitle"
    class="config-name-dialog"
  >
    <div class="name-editor">
      <label for="config-item-name">配置名称</label>
      <input
        id="config-item-name"
        v-model="nameDialogValue"
        class="name-input"
        type="text"
        maxlength="64"
        autocomplete="off"
        @keydown.enter.prevent="confirmNameDialog"
      />
    </div>
    <template #footer>
      <div class="name-dialog-footer">
        <button type="button" class="name-action ghost" @click="nameDialogVisible = false">取消</button>
        <button type="button" class="name-action primary" :disabled="isBusy" @click="confirmNameDialog">
          {{ nameDialogAction }}
        </button>
      </div>
    </template>
  </StudioDialog>
</template>

<style scoped>
.config-library-dialog :deep(.p-dialog-content) {
  padding: 0;
  background: transparent;
  overflow: hidden;
}

.library-shell {
  position: relative;
  min-height: 620px;
  color: var(--c-text-primary);
  background:
    linear-gradient(90deg, rgba(255, 255, 255, 0.035) 0 1px, transparent 1px 56px),
    linear-gradient(135deg, var(--c-bg-primary), var(--c-bg-secondary));
  border: 1px solid var(--c-border);
  border-radius: var(--radius-md);
  overflow: hidden;
}

.library-head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 1rem;
  padding: 1rem 1.1rem;
  border-bottom: 1px solid var(--c-border);
  background: color-mix(in srgb, var(--c-bg-tertiary) 82%, transparent);
}

.library-summary {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 1rem;
  padding: 0.68rem 0.95rem;
  border-bottom: 1px solid var(--c-border);
  color: var(--c-text-muted);
  background: color-mix(in srgb, var(--c-bg-secondary) 76%, transparent);
  font-size: 0.72rem;
  font-weight: 750;
}

.library-summary p {
  margin: 0;
}

.library-summary code {
  flex: 0 0 auto;
  color: var(--c-accent-light);
  font-family: 'JetBrains Mono', 'SFMono-Regular', Consolas, monospace;
  font-size: 0.66rem;
  font-weight: 900;
}

.library-kicker,
.detail-topline,
.config-index,
.config-item small,
.detail-card dt,
.detail-card dd,
.action {
  font-family: 'JetBrains Mono', 'SFMono-Regular', Consolas, monospace;
}

.library-kicker {
  color: var(--c-accent-light);
  font-size: 0.62rem;
  font-weight: 900;
  letter-spacing: 0.08em;
}

.library-head h2 {
  margin: 0.18rem 0 0;
  font-size: 1.08rem;
  line-height: 1.1;
}

.library-head p {
  margin: 0.34rem 0 0;
  color: var(--c-text-muted);
  font-size: 0.76rem;
}

.icon-btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 2rem;
  height: 2rem;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  background: var(--c-bg-secondary);
  color: var(--c-text-secondary);
  cursor: pointer;
}

.library-body {
  display: grid;
  grid-template-columns: minmax(260px, 34%) 1fr;
  min-height: 520px;
}

.config-list {
  display: flex;
  flex-direction: column;
  gap: 0.45rem;
  padding: 0.8rem;
  border-right: 1px solid var(--c-border);
  overflow-y: auto;
}

.config-item {
  display: grid;
  grid-template-columns: 2rem minmax(0, 1fr) 1.2rem;
  align-items: center;
  gap: 0.62rem;
  min-height: 3.35rem;
  padding: 0.58rem 0.64rem;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  background: var(--c-bg-secondary);
  color: var(--c-text-primary);
  text-align: left;
  cursor: pointer;
}

.config-item.active {
  border-color: var(--c-accent);
  background: var(--c-accent-soft);
}

.config-item.active .config-index {
  color: var(--c-accent-light);
}

.config-index {
  color: var(--c-text-muted);
  font-size: 0.7rem;
  font-weight: 950;
}

.config-main {
  display: flex;
  min-width: 0;
  flex-direction: column;
  gap: 0.15rem;
}

.config-main strong,
.config-main small {
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.config-main strong {
  font-size: 0.82rem;
}

.config-main small {
  color: var(--c-text-muted);
  font-size: 0.6rem;
}

.config-empty-list {
  padding: 0.8rem 0.65rem;
  color: var(--c-text-muted);
  font-size: 0.72rem;
  font-weight: 800;
}

.config-detail {
  display: flex;
  flex-direction: column;
  gap: 0.85rem;
  padding: 1rem;
}

.detail-card {
  min-height: 260px;
  padding: 1rem;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  background:
    linear-gradient(135deg, color-mix(in srgb, var(--c-bg-secondary) 84%, var(--c-accent) 8%), var(--c-bg-secondary));
}

.detail-card.empty {
  border-style: dashed;
}

.detail-topline {
  display: flex;
  justify-content: space-between;
  color: var(--c-text-muted);
  font-size: 0.62rem;
  font-weight: 900;
}

.detail-card h3 {
  margin: 1rem 0 0.8rem;
  font-size: 1.08rem;
}

.detail-card p {
  color: var(--c-text-muted);
  font-size: 0.82rem;
}

.detail-card dl {
  display: grid;
  gap: 0.58rem;
  margin: 0;
}

.detail-card dl div {
  display: grid;
  grid-template-columns: 4rem 1fr;
  gap: 0.8rem;
  align-items: baseline;
}

.detail-card dt {
  color: var(--c-text-muted);
  font-size: 0.62rem;
  font-weight: 900;
}

.detail-card dd {
  margin: 0;
  color: var(--c-text-primary);
  font-size: 0.7rem;
  font-weight: 800;
}

.action-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 0.55rem;
}

.action {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 0.42rem;
  min-height: 2.35rem;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  background: var(--c-bg-secondary);
  color: var(--c-text-secondary);
  cursor: pointer;
  font-size: 0.68rem;
  font-weight: 900;
}

.action.primary {
  border-color: color-mix(in srgb, var(--c-accent) 62%, var(--c-border));
  background: var(--c-accent-soft);
  color: var(--c-accent-light);
}

.action.danger {
  border-color: color-mix(in srgb, var(--c-danger) 52%, var(--c-border));
  color: var(--c-danger);
}

.action:disabled {
  opacity: 0.42;
  cursor: default;
}

.busy-line {
  position: absolute;
  left: 1rem;
  right: 1rem;
  bottom: 0.85rem;
  display: flex;
  align-items: center;
  gap: 0.5rem;
  color: var(--c-accent-light);
  font-size: 0.76rem;
  font-weight: 800;
}

.name-editor {
  display: grid;
  gap: 0.5rem;
}

.name-editor label {
  color: var(--c-text-muted);
  font-size: 0.72rem;
  font-weight: 800;
}

.name-input {
  width: 100%;
  min-height: 2.5rem;
  padding: 0 0.75rem;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  background: var(--c-bg-primary);
  color: var(--c-text-primary);
  font: inherit;
  outline: none;
}

.name-input:focus {
  border-color: var(--c-accent);
  box-shadow: 0 0 0 2px var(--c-accent-soft);
}

.name-dialog-footer {
  display: flex;
  justify-content: flex-end;
  gap: 0.5rem;
}

.name-action {
  min-height: 2.25rem;
  padding: 0 0.9rem;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  background: var(--c-bg-tertiary);
  color: var(--c-text-secondary);
  cursor: pointer;
  font-size: 0.78rem;
  font-weight: 800;
}

.name-action.primary {
  border-color: color-mix(in srgb, var(--c-accent) 62%, var(--c-border));
  background: var(--c-accent-soft);
  color: var(--c-accent-light);
}

.name-action.ghost {
  background: transparent;
}

.name-action:disabled {
  opacity: 0.48;
  cursor: default;
}

@media (max-width: 820px) {
  .library-body {
    grid-template-columns: 1fr;
  }

  .config-list {
    max-height: 260px;
    border-right: 0;
    border-bottom: 1px solid var(--c-border);
  }

  .action-grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}
</style>
