<template>
  <Dialog 
    v-model:visible="dialogVisible" 
    :header="`编辑按键 ${keyIndex + 1}`"
    :style="{ width: '480px' }"
    modal
    :closable="!isListening"
    :closeOnEscape="!isListening"
  >
    <Tabs v-model:value="activeTab">
      <TabList>
        <Tab value="keyboard"><i class="pi pi-keyboard"></i> 键盘</Tab>
        <Tab value="media"><i class="pi pi-volume-up"></i> 媒体</Tab>
        <Tab value="mouse"><i class="pi pi-desktop"></i> 鼠标</Tab>
        <Tab value="layer"><i class="pi pi-layer-group" style="font-size: 0.875rem;"></i> 层</Tab>
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
              <Button 
                v-for="key in consumerKeys" 
                :key="key.code"
                :label="key.name"
                :severity="editAction.param1 === (key.code & 0xff) && editAction.param2 === ((key.code >> 8) & 0xff) ? 'primary' : 'secondary'"
                :outlined="!(editAction.param1 === (key.code & 0xff) && editAction.param2 === ((key.code >> 8) & 0xff))"
                size="small"
                @click="selectConsumer(key.code)"
              />
            </div>
          </div>
        </TabPanel>

        <!-- 鼠标 -->
        <TabPanel value="mouse">
          <div class="editor-panel">
            <div class="mouse-section">
              <label class="section-label">鼠标按键</label>
              <div class="mouse-buttons">
                <Button 
                  v-for="btn in mouseButtons" 
                  :key="btn.value"
                  :label="btn.label"
                  :severity="editAction.type === ActionType.MOUSE_BTN && editAction.param1 === btn.value ? 'primary' : 'secondary'"
                  :outlined="!(editAction.type === ActionType.MOUSE_BTN && editAction.param1 === btn.value)"
                  @click="selectMouseButton(btn.value)"
                />
              </div>
            </div>

            <Divider />

            <div class="wheel-section">
              <label class="section-label">滚轮</label>
              <div class="wheel-buttons">
                <Button 
                  v-for="dir in wheelDirections" 
                  :key="dir.value"
                  :label="dir.label"
                  :severity="editAction.type === ActionType.MOUSE_WHEEL && editAction.param1 === dir.value ? 'primary' : 'secondary'"
                  :outlined="!(editAction.type === ActionType.MOUSE_WHEEL && editAction.param1 === dir.value)"
                  @click="selectWheel(dir.value)"
                />
              </div>
            </div>
          </div>
        </TabPanel>

        <!-- 层切换 -->
        <TabPanel value="layer">
          <div class="editor-panel">
            <div class="layer-section">
              <label class="section-label">操作类型</label>
              <SelectButton 
                v-model="layerOp" 
                :options="layerOpOptions" 
                optionLabel="label" 
                optionValue="value"
              />
            </div>

            <Divider />

            <div class="layer-target">
              <label class="section-label">目标层</label>
              <div class="layer-buttons">
                <Button 
                  v-for="i in 4" 
                  :key="i"
                  :label="`层 ${i}`"
                  :severity="layerTarget === i - 1 ? 'primary' : 'secondary'"
                  :outlined="layerTarget !== i - 1"
                  @click="layerTarget = i - 1"
                />
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
                <Button 
                  v-for="i in 8" 
                  :key="i"
                  :label="`宏 ${i}`"
                  :severity="macroId === i - 1 ? 'primary' : 'secondary'"
                  :outlined="macroId !== i - 1"
                  @click="macroId = i - 1"
                />
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

// 选择多媒体键
function selectConsumer(code: number) {
  editAction.value = {
    type: ActionType.CONSUMER,
    modifier: 0,
    param1: code & 0xff,
    param2: (code >> 8) & 0xff,
  };
}

// 选择鼠标按键
function selectMouseButton(button: number) {
  editAction.value = {
    type: ActionType.MOUSE_BTN,
    modifier: 0,
    param1: button,
    param2: 0,
  };
}

// 选择滚轮
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
  font-weight: 600;
  color: var(--app-text-muted);
  margin-bottom: 0.75rem;
}

/* 键盘捕获 */
.keyboard-capture {
  background: var(--app-surface);
  border: 2px dashed var(--app-border);
  border-radius: 12px;
  padding: 1.5rem;
  text-align: center;
  transition: all 0.2s;
}

.keyboard-capture.listening {
  border-color: var(--app-accent);
  background: var(--app-accent-soft);
}

.capture-preview {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 1rem;
}

.preview-key {
  font-size: 1.5rem;
  font-weight: 700;
  color: var(--app-accent);
}

.capture-listening {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 1rem;
}

/* 修饰键 */
.modifier-grid {
  display: flex;
  gap: 0.5rem;
  flex-wrap: wrap;
}

/* 媒体键 */
.media-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 0.5rem;
}

/* 鼠标 */
.mouse-buttons,
.wheel-buttons {
  display: flex;
  gap: 0.5rem;
  flex-wrap: wrap;
}

/* 层 */
.layer-buttons {
  display: flex;
  gap: 0.5rem;
}

/* 宏 */
.macro-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 0.5rem;
}

.macro-hint {
  margin-top: 1rem;
  font-size: 0.85rem;
  color: var(--app-text-muted);
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
