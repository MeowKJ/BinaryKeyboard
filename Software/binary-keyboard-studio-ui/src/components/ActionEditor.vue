<template>
  <Dialog 
    v-model:visible="dialogVisible" 
    :header="`编辑按键 ${keyIndex + 1}`"
    :style="{ width: '520px' }"
    modal
    :closable="!isListening"
    :closeOnEscape="!isListening"
    class="action-editor-dialog"
  >
    <Tabs v-model:value="activeTab">
      <TabList>
        <Tab value="keyboard"><i class="pi pi-keyboard"></i> 键盘</Tab>
        <Tab value="media"><i class="pi pi-volume-up"></i> 媒体</Tab>
        <Tab value="mouse"><i class="pi pi-desktop"></i> 鼠标</Tab>
        <Tab value="layer"><i class="pi pi-layer-group"></i> 层</Tab>
        <Tab value="macro"><i class="pi pi-code"></i> 宏</Tab>
      </TabList>

      <TabPanels>
        <!-- 键盘 -->
        <TabPanel value="keyboard">
          <div class="editor-panel">
            <div class="keyboard-capture" :class="{ listening: isListening }">
              <div v-if="!isListening" class="capture-preview">
                <span class="preview-key">{{ previewKeyName }}</span>
                <Button 
                  label="点击录入按键" 
                  icon="pi pi-circle" 
                  severity="secondary"
                  @click="startListening"
                />
              </div>
              <div v-else class="capture-listening">
                <ProgressSpinner strokeWidth="4" style="width: 40px; height: 40px;" />
                <span>请按下要设置的按键...</span>
                <Button 
                  label="取消" 
                  icon="pi pi-times" 
                  severity="secondary" 
                  text
                  @click="cancelListening"
                />
              </div>
            </div>

            <Divider />

            <div class="modifier-section">
              <label class="section-label">修饰键</label>
              <div class="modifier-grid">
                <ToggleButton v-model="modCtrl" onLabel="Ctrl" offLabel="Ctrl" />
                <ToggleButton v-model="modShift" onLabel="Shift" offLabel="Shift" />
                <ToggleButton v-model="modAlt" onLabel="Alt" offLabel="Alt" />
                <ToggleButton v-model="modGui" onLabel="Win" offLabel="Win" />
              </div>
            </div>
          </div>
        </TabPanel>

        <!-- 媒体 -->
        <TabPanel value="media">
          <div class="editor-panel">
            <div class="media-grid">
              <button 
                v-for="key in consumerKeys" 
                :key="key.code"
                class="media-key"
                :class="{ active: isConsumerActive(key.code) }"
                @click="selectConsumer(key.code)"
              >
                {{ key.name }}
              </button>
            </div>
          </div>
        </TabPanel>

        <!-- 鼠标 -->
        <TabPanel value="mouse">
          <div class="editor-panel">
            <div class="mouse-section">
              <label class="section-label">鼠标按键</label>
              <div class="mouse-buttons">
                <button 
                  v-for="btn in mouseButtons" 
                  :key="btn.value"
                  class="option-btn"
                  :class="{ active: editAction.type === ActionType.MOUSE_BTN && editAction.param1 === btn.value }"
                  @click="selectMouseButton(btn.value)"
                >
                  {{ btn.label }}
                </button>
              </div>
            </div>

            <Divider />

            <div class="wheel-section">
              <label class="section-label">滚轮</label>
              <div class="wheel-buttons">
                <button 
                  v-for="dir in wheelDirections" 
                  :key="dir.value"
                  class="option-btn"
                  :class="{ active: editAction.type === ActionType.MOUSE_WHEEL && editAction.param1 === dir.value }"
                  @click="selectWheel(dir.value)"
                >
                  {{ dir.label }}
                </button>
              </div>
            </div>
          </div>
        </TabPanel>

        <!-- 层切换 -->
        <TabPanel value="layer">
          <div class="editor-panel">
            <div class="layer-section">
              <label class="section-label">操作类型</label>
              <div class="layer-op-buttons">
                <button 
                  v-for="op in layerOpOptions" 
                  :key="op.value"
                  class="option-btn"
                  :class="{ active: layerOp === op.value }"
                  @click="layerOp = op.value"
                >
                  {{ op.label }}
                </button>
              </div>
            </div>

            <Divider />

            <div class="layer-target">
              <label class="section-label">目标层</label>
              <div class="layer-buttons">
                <button 
                  v-for="i in 4" 
                  :key="i"
                  class="option-btn layer-btn"
                  :class="{ active: layerTarget === i - 1 }"
                  @click="layerTarget = i - 1"
                >
                  层 {{ i }}
                </button>
              </div>
            </div>
          </div>
        </TabPanel>

        <!-- 宏 -->
        <TabPanel value="macro">
          <div class="editor-panel">
            <div class="macro-section">
              <label class="section-label">选择宏</label>
              <div class="macro-grid">
                <button 
                  v-for="i in 8" 
                  :key="i"
                  class="option-btn macro-btn"
                  :class="{ active: macroId === i - 1 }"
                  @click="macroId = i - 1"
                >
                  宏 {{ i }}
                </button>
              </div>
              <p class="macro-hint">
                <i class="pi pi-info-circle"></i>
                宏编辑功能暂未开放
              </p>
            </div>
          </div>
        </TabPanel>
      </TabPanels>
    </Tabs>

    <template #footer>
      <div class="dialog-footer">
        <Button 
          label="清空" 
          icon="pi pi-trash" 
          severity="danger" 
          text
          @click="clearAction"
        />
        <div class="footer-right">
          <Button label="取消" severity="secondary" @click="dialogVisible = false" />
          <Button label="确定" icon="pi pi-check" @click="confirmAction" />
        </div>
      </div>
    </template>
  </Dialog>
</template>

<script setup lang="ts">
import { ref, computed, watch } from 'vue';
import {
  ActionType,
  Modifier,
  MouseButton,
  WheelDirection,
  LayerOp,
  type KeyAction,
  createEmptyAction,
} from '@/types/protocol';
import { getKeycodeName, getHidFromEvent } from '@/utils/keycodes';
import { CONSUMER_KEYS } from '@/utils/consumer';

const props = defineProps<{
  visible: boolean;
  keyIndex: number;
  action: KeyAction;
}>();

const emit = defineEmits<{
  'update:visible': [value: boolean];
  save: [action: KeyAction];
}>();

// 对话框可见性
const dialogVisible = computed({
  get: () => props.visible,
  set: (val) => emit('update:visible', val),
});

// 当前编辑的动作
const editAction = ref<KeyAction>(createEmptyAction());
const activeTab = ref('keyboard');
const isListening = ref(false);

// 键盘相关
const keycode = ref(0);
const modCtrl = ref(false);
const modShift = ref(false);
const modAlt = ref(false);
const modGui = ref(false);

// 层相关
const layerOp = ref(LayerOp.TOGGLE);
const layerTarget = ref(0);

// 宏相关
const macroId = ref(0);

// 预览
const previewKeyName = computed(() => {
  if (activeTab.value !== 'keyboard') return '';
  const mod = (modCtrl.value ? Modifier.LCTRL : 0) |
              (modShift.value ? Modifier.LSHIFT : 0) |
              (modAlt.value ? Modifier.LALT : 0) |
              (modGui.value ? Modifier.LGUI : 0);
  return getKeycodeName(keycode.value, mod) || '未设置';
});

// 数据
const consumerKeys = CONSUMER_KEYS;

const mouseButtons = [
  { label: '左键', value: MouseButton.LEFT },
  { label: '右键', value: MouseButton.RIGHT },
  { label: '中键', value: MouseButton.MIDDLE },
  { label: '后退', value: MouseButton.BACK },
  { label: '前进', value: MouseButton.FORWARD },
];

const wheelDirections = [
  { label: '向上', value: WheelDirection.UP },
  { label: '向下', value: WheelDirection.DOWN },
  { label: '中键点击', value: WheelDirection.CLICK },
];

const layerOpOptions = [
  { label: '切换', value: LayerOp.TOGGLE },
  { label: '按住', value: LayerOp.MOMENTARY },
  { label: '跳转', value: LayerOp.SET },
];

// 初始化编辑状态
watch(() => props.visible, (visible) => {
  if (visible) {
    initFromAction(props.action);
  }
});

function initFromAction(action: KeyAction) {
  editAction.value = { ...action };

  switch (action.type) {
    case ActionType.KEYBOARD:
      activeTab.value = 'keyboard';
      keycode.value = action.param1;
      modCtrl.value = !!(action.modifier & Modifier.LCTRL);
      modShift.value = !!(action.modifier & Modifier.LSHIFT);
      modAlt.value = !!(action.modifier & Modifier.LALT);
      modGui.value = !!(action.modifier & Modifier.LGUI);
      break;
    case ActionType.CONSUMER:
      activeTab.value = 'media';
      break;
    case ActionType.MOUSE_BTN:
    case ActionType.MOUSE_WHEEL:
      activeTab.value = 'mouse';
      break;
    case ActionType.LAYER:
      activeTab.value = 'layer';
      layerOp.value = action.modifier;
      layerTarget.value = action.param1;
      break;
    case ActionType.MACRO:
      activeTab.value = 'macro';
      macroId.value = action.param1;
      break;
    default:
      activeTab.value = 'keyboard';
      keycode.value = 0;
      modCtrl.value = false;
      modShift.value = false;
      modAlt.value = false;
      modGui.value = false;
  }
}

// 键盘监听
function startListening() {
  isListening.value = true;
  window.addEventListener('keydown', handleKeyDown);
}

function cancelListening() {
  isListening.value = false;
  window.removeEventListener('keydown', handleKeyDown);
}

function handleKeyDown(event: KeyboardEvent) {
  event.preventDefault();
  event.stopPropagation();

  const { keycode: hid, modifier } = getHidFromEvent(event);
  
  if (hid > 0) {
    keycode.value = hid;
    modCtrl.value = !!(modifier & Modifier.LCTRL);
    modShift.value = !!(modifier & Modifier.LSHIFT);
    modAlt.value = !!(modifier & Modifier.LALT);
    modGui.value = !!(modifier & Modifier.LGUI);
  }

  cancelListening();
}

// 多媒体键
function isConsumerActive(code: number): boolean {
  return editAction.value.type === ActionType.CONSUMER &&
    editAction.value.param1 === (code & 0xff) &&
    editAction.value.param2 === ((code >> 8) & 0xff);
}

function selectConsumer(code: number) {
  editAction.value = {
    type: ActionType.CONSUMER,
    modifier: 0,
    param1: code & 0xff,
    param2: (code >> 8) & 0xff,
  };
}

// 鼠标按键
function selectMouseButton(button: number) {
  editAction.value = {
    type: ActionType.MOUSE_BTN,
    modifier: 0,
    param1: button,
    param2: 0,
  };
}

// 滚轮
function selectWheel(direction: number) {
  editAction.value = {
    type: ActionType.MOUSE_WHEEL,
    modifier: 0,
    param1: direction,
    param2: 0,
  };
}

// 清空动作
function clearAction() {
  editAction.value = createEmptyAction();
  keycode.value = 0;
  modCtrl.value = false;
  modShift.value = false;
  modAlt.value = false;
  modGui.value = false;
}

// 确认动作
function confirmAction() {
  let finalAction: KeyAction;

  switch (activeTab.value) {
    case 'keyboard':
      const mod = (modCtrl.value ? Modifier.LCTRL : 0) |
                  (modShift.value ? Modifier.LSHIFT : 0) |
                  (modAlt.value ? Modifier.LALT : 0) |
                  (modGui.value ? Modifier.LGUI : 0);
      finalAction = {
        type: ActionType.KEYBOARD,
        modifier: mod,
        param1: keycode.value,
        param2: 0,
      };
      break;
    case 'media':
      finalAction = { ...editAction.value };
      break;
    case 'mouse':
      finalAction = { ...editAction.value };
      break;
    case 'layer':
      finalAction = {
        type: ActionType.LAYER,
        modifier: layerOp.value,
        param1: layerTarget.value,
        param2: 0,
      };
      break;
    case 'macro':
      finalAction = {
        type: ActionType.MACRO,
        modifier: 0,
        param1: macroId.value,
        param2: 0,
      };
      break;
    default:
      finalAction = createEmptyAction();
  }

  emit('save', finalAction);
}
</script>

<style scoped>
.editor-panel {
  padding: 1rem 0;
}

.section-label {
  display: block;
  font-size: 0.85rem;
  font-weight: 700;
  color: var(--c-text-muted);
  margin-bottom: 0.75rem;
}

/* 键盘捕获 */
.keyboard-capture {
  background: var(--c-bg-tertiary);
  border: 2px dashed var(--c-border);
  border-radius: var(--radius-lg);
  padding: 1.5rem;
  text-align: center;
  transition: all var(--transition-fast);
}

.keyboard-capture.listening {
  border-color: var(--c-accent);
  background: var(--c-accent-soft);
}

.capture-preview {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 1rem;
}

.preview-key {
  font-size: 1.5rem;
  font-weight: 800;
  color: var(--c-accent);
}

.capture-listening {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 1rem;
  color: var(--c-text-secondary);
}

/* 修饰键 */
.modifier-grid {
  display: flex;
  gap: 0.5rem;
  flex-wrap: wrap;
}

/* 通用选项按钮 */
.option-btn {
  padding: 0.625rem 1rem;
  font-size: 0.875rem;
  font-weight: 600;
  font-family: inherit;
  border-radius: var(--radius-sm);
  border: 1px solid var(--c-border);
  background: var(--c-bg-tertiary);
  color: var(--c-text-muted);
  cursor: pointer;
  transition: all var(--transition-fast);
}

.option-btn:hover {
  background: var(--c-bg-hover);
  border-color: var(--c-accent);
  color: var(--c-text-secondary);
}

.option-btn.active {
  background: var(--c-accent-soft);
  border-color: var(--c-accent);
  color: var(--c-accent);
}

/* 媒体键 */
.media-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 0.5rem;
}

.media-key {
  padding: 0.75rem 0.5rem;
  font-size: 0.8rem;
  font-weight: 600;
  font-family: inherit;
  border-radius: var(--radius-sm);
  border: 1px solid var(--c-border);
  background: var(--c-bg-tertiary);
  color: var(--c-text-muted);
  cursor: pointer;
  transition: all var(--transition-fast);
  text-align: center;
}

.media-key:hover {
  background: var(--c-bg-hover);
  border-color: var(--c-accent);
  color: var(--c-text-secondary);
}

.media-key.active {
  background: var(--c-accent-soft);
  border-color: var(--c-accent);
  color: var(--c-accent);
}

/* 鼠标 */
.mouse-buttons,
.wheel-buttons {
  display: flex;
  gap: 0.5rem;
  flex-wrap: wrap;
}

/* 层 */
.layer-op-buttons,
.layer-buttons {
  display: flex;
  gap: 0.5rem;
  flex-wrap: wrap;
}

.layer-btn {
  min-width: 70px;
}

/* 宏 */
.macro-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 0.5rem;
}

.macro-btn {
  text-align: center;
}

.macro-hint {
  margin-top: 1rem;
  font-size: 0.85rem;
  color: var(--c-text-muted);
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

/* 底部 */
.dialog-footer {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.footer-right {
  display: flex;
  gap: 0.5rem;
}
</style>

<style>
/* 对话框全局样式覆盖 */
.action-editor-dialog .p-dialog-header {
  background: var(--c-bg-secondary) !important;
}

.action-editor-dialog .p-dialog-content {
  background: var(--c-bg-secondary) !important;
}

.action-editor-dialog .p-dialog-footer {
  background: var(--c-bg-secondary) !important;
}
</style>
