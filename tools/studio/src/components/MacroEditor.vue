<template>
  <Dialog
    v-model:visible="dialogVisible"
    :style="{ width: '680px', maxHeight: '85vh' }"
    modal
    :closable="!isRecording && capturingIdx < 0"
    :closeOnEscape="!isRecording && capturingIdx < 0"
    class="macro-editor-dialog"
    @hide="cleanupInteractiveState"
  >
    <template #header>
      <div class="editor-header">
        <div class="editor-title">
          <span class="editor-title-main">宏编辑器</span>
          <span class="editor-title-slot">槽位 {{ slot + 1 }}</span>
        </div>
        <div class="header-name-wrap">
          <InputText
            :modelValue="macroStore.editingName"
            placeholder="输入宏名称"
            class="header-name-input"
            @update:modelValue="onNameInput"
          />
          <span class="header-name-bytes">
            {{ nameByteCount }}/{{ MACRO_NAME_MAX_BYTES }}
          </span>
        </div>
      </div>
    </template>

    <div class="pipeline-container" ref="pipelineRef">
      <div v-if="macroStore.editingCards.length === 0" class="pipeline-empty">
        <i class="pi pi-inbox"></i>
        <span>点击下方按钮添加动作，或录制一段操作</span>
      </div>

      <VueDraggable
        v-else
        v-model="macroStore.editingCards"
        :animation="200"
        :move="canMoveCard"
        handle=".card-drag"
        ghostClass="card-ghost"
        chosenClass="card-chosen"
        class="pipeline-list"
      >
        <div
          v-for="(card, idx) in macroStore.editingCards"
          :key="idx"
          class="action-card"
          :class="actionCategory(card.action)"
        >
          <div v-if="idx > 0" class="connector">
            <div class="connector-line"></div>
          </div>

          <div class="card-body">
            <div class="card-drag" v-tooltip.left="'拖拽排序'">
              <i class="pi pi-bars"></i>
            </div>

            <div class="card-icon">
              <i :class="actionIcon(card.action)"></i>
            </div>

            <div class="card-content">
              <span class="card-category">{{ actionCategoryLabel(card.action) }}</span>

              <button
                v-if="isKeyboardAction(card.action)"
                class="card-desc card-desc-btn"
                :class="{ capturing: capturingIdx === idx }"
                @click="startCaptureForCard(idx)"
              >
                <template v-if="capturingIdx === idx">按下按键…</template>
                <template v-else>
                  {{ actionDescription(card.action) }}
                  <i class="pi pi-pencil card-edit-hint"></i>
                </template>
              </button>

              <span v-else class="card-desc">{{ actionDescription(card.action) }}</span>
            </div>

            <div class="card-delay">
              <label class="delay-label">后延时</label>
              <div class="delay-input-wrap">
                <input
                  :value="card.delayMs"
                  type="number"
                  min="0"
                  step="10"
                  inputmode="numeric"
                  class="delay-input"
                  @change="onDelayInput(idx, $event)"
                  @blur="onDelayInput(idx, $event)"
                />
                <span class="delay-unit">ms</span>
              </div>
            </div>

            <button class="card-remove" @click="removeCard(idx)" v-tooltip.right="'删除'">
              <i class="pi pi-times"></i>
            </button>
          </div>
        </div>
      </VueDraggable>
    </div>

    <div class="pipeline-stats" :class="{ over: macroStore.editingOverLimit }">
      <span>{{ macroStore.editingCards.length }} 卡片</span>
      <span>·</span>
      <span>{{ macroStore.editingActionCount }} 动作</span>
      <span>·</span>
      <span>{{ macroStore.editingDataSize }} / {{ MACRO_MAX_DATA_SIZE }} 字节</span>
    </div>

    <div v-if="showMouseMenu" class="sub-menu">
      <div class="sub-menu-title">鼠标按键</div>
      <div class="sub-menu-grid">
        <button
          v-for="btn in mouseOptions"
          :key="btn.param"
          class="sub-menu-item"
          @click="addMouseClick(btn.param); showMouseMenu = false"
        >
          {{ btn.label }}
        </button>
      </div>
      <div class="sub-menu-title">滚轮</div>
      <div class="sub-menu-grid">
        <button class="sub-menu-item" @click="addWheel(1); showMouseMenu = false">向上</button>
        <button class="sub-menu-item" @click="addWheel(2); showMouseMenu = false">向下</button>
      </div>
    </div>

    <div v-if="showConsumerMenu" class="sub-menu">
      <div class="sub-menu-grid cols-3">
        <button
          v-for="key in CONSUMER_KEYS"
          :key="key.code"
          class="sub-menu-item"
          @click="addConsumer(key.code); showConsumerMenu = false"
        >
          {{ key.name }}
        </button>
      </div>
    </div>

    <template #footer>
      <div class="editor-footer">
        <div class="footer-toolbar">
          <div class="add-bar">
            <button class="add-btn add-keyboard" @click="addKeyAction" v-tooltip.top="'添加按键动作'">
              <i class="pi pi-keyboard"></i>
              <span>按键</span>
            </button>
            <button
              class="add-btn add-mouse"
              @click="showMouseMenu = !showMouseMenu; showConsumerMenu = false"
              v-tooltip.top="'添加鼠标动作'"
            >
              <i class="pi pi-desktop"></i>
              <span>鼠标</span>
            </button>
            <button
              class="add-btn add-consumer"
              @click="showConsumerMenu = !showConsumerMenu; showMouseMenu = false"
              v-tooltip.top="'添加媒体动作'"
            >
              <i class="pi pi-volume-up"></i>
              <span>媒体</span>
            </button>
            <button
              class="add-btn add-record"
              :class="{ recording: isRecording }"
              @click="isRecording ? stopRecording() : startRecording()"
              v-tooltip.top="isRecording ? '停止录制' : '开始录制'"
            >
              <i :class="isRecording ? 'pi pi-stop-circle' : 'pi pi-circle-fill'"></i>
              <span>{{ isRecording ? `停止 ${recordedCount}` : '录制' }}</span>
            </button>
          </div>
        </div>
        <div class="footer-actions">
          <Button
            v-if="macroStore.slotValid[slot]"
            label="删除宏"
            icon="pi pi-trash"
            severity="danger"
            text
            @click="handleDelete"
            :loading="macroStore.isLoading"
          />
          <Button label="取消" severity="secondary" @click="closeEditor" />
          <Button
            label="保存"
            icon="pi pi-check"
            @click="handleSave"
            :loading="macroStore.isSaving"
            :disabled="macroStore.editingOverLimit || macroStore.editingCards.length === 0"
          />
        </div>
      </div>
    </template>
  </Dialog>
</template>

<script setup lang="ts">
import { computed, onBeforeUnmount, ref, watch } from "vue";
import { VueDraggable } from "vue-draggable-plus";
import { useMacroStore } from "@/stores/macroStore";
import {
  MACRO_MAX_DATA_SIZE,
  MACRO_NAME_MAX_BYTES,
  MacroActionType,
  type MacroAction,
} from "@/types/protocol";
import { CONSUMER_KEYS, getConsumerName } from "@/utils/consumer";
import { getHidFromEvent, KEYCODE_NAMES } from "@/utils/keycodes";
import { truncateUtf8ByBytes, utf8ByteLength } from "@/utils/utf8";

const props = defineProps<{
  visible: boolean;
  slot: number;
}>();

const emit = defineEmits<{
  "update:visible": [value: boolean];
}>();

const macroStore = useMacroStore();

const dialogVisible = computed({
  get: () => props.visible,
  set: (value: boolean) => emit("update:visible", value),
});

const pipelineRef = ref<HTMLElement | null>(null);
const showMouseMenu = ref(false);
const showConsumerMenu = ref(false);
const capturingIdx = ref(-1);

const isRecording = ref(false);
const recordedCount = ref(0);
let recordLastTime = 0;

const nameByteCount = computed(() => utf8ByteLength(macroStore.editingName));

const mouseOptions = [
  { label: "左键", param: 0x01 },
  { label: "右键", param: 0x02 },
  { label: "中键", param: 0x04 },
  { label: "后退", param: 0x08 },
  { label: "前进", param: 0x10 },
];

const MOD_NAMES: Record<number, string> = {
  0x01: "LCtrl",
  0x02: "LShift",
  0x04: "LAlt",
  0x08: "LWin",
  0x10: "RCtrl",
  0x20: "RShift",
  0x40: "RAlt",
  0x80: "RWin",
};

const MOUSE_BTN_NAMES: Record<number, string> = {
  0x01: "左键",
  0x02: "右键",
  0x04: "中键",
  0x08: "后退",
  0x10: "前进",
};

const CODE_TO_MOD: Record<string, number> = {
  ControlLeft: 0x01,
  ShiftLeft: 0x02,
  AltLeft: 0x04,
  MetaLeft: 0x08,
  ControlRight: 0x10,
  ShiftRight: 0x20,
  AltRight: 0x40,
  MetaRight: 0x80,
};

function getPairTypes(actionType: MacroActionType): {
  press: MacroActionType;
  release: MacroActionType;
} | null {
  switch (actionType) {
    case MacroActionType.KEY_DOWN:
    case MacroActionType.KEY_UP:
      return {
        press: MacroActionType.KEY_DOWN,
        release: MacroActionType.KEY_UP,
      };
    case MacroActionType.MOD_DOWN:
    case MacroActionType.MOD_UP:
      return {
        press: MacroActionType.MOD_DOWN,
        release: MacroActionType.MOD_UP,
      };
    case MacroActionType.MOUSE_DOWN:
    case MacroActionType.MOUSE_UP:
      return {
        press: MacroActionType.MOUSE_DOWN,
        release: MacroActionType.MOUSE_UP,
      };
    default:
      return null;
  }
}

function isPressAction(actionType: MacroActionType): boolean {
  const pair = getPairTypes(actionType);
  return pair?.press === actionType;
}

function isReleaseAction(actionType: MacroActionType): boolean {
  const pair = getPairTypes(actionType);
  return pair?.release === actionType;
}

function onNameInput(value: string | number | undefined): void {
  macroStore.editingName = truncateUtf8ByBytes(String(value ?? ""), MACRO_NAME_MAX_BYTES);
}

function actionCategory(action: MacroAction): string {
  switch (action.type) {
    case MacroActionType.KEY_DOWN:
    case MacroActionType.KEY_UP:
    case MacroActionType.MOD_DOWN:
    case MacroActionType.MOD_UP:
      return "cat-keyboard";
    case MacroActionType.MOUSE_DOWN:
    case MacroActionType.MOUSE_UP:
    case MacroActionType.WHEEL:
      return "cat-mouse";
    case MacroActionType.CONSUMER:
      return "cat-consumer";
    default:
      return "";
  }
}

function actionCategoryLabel(action: MacroAction): string {
  switch (action.type) {
    case MacroActionType.KEY_DOWN:
    case MacroActionType.KEY_UP:
      return "键盘";
    case MacroActionType.MOD_DOWN:
    case MacroActionType.MOD_UP:
      return "修饰键";
    case MacroActionType.MOUSE_DOWN:
    case MacroActionType.MOUSE_UP:
      return "鼠标";
    case MacroActionType.WHEEL:
      return "滚轮";
    case MacroActionType.CONSUMER:
      return "媒体";
    default:
      return "未知";
  }
}

function actionIcon(action: MacroAction): string {
  switch (action.type) {
    case MacroActionType.KEY_DOWN:
    case MacroActionType.KEY_UP:
    case MacroActionType.MOD_DOWN:
    case MacroActionType.MOD_UP:
      return "pi pi-keyboard";
    case MacroActionType.MOUSE_DOWN:
    case MacroActionType.MOUSE_UP:
    case MacroActionType.WHEEL:
      return "pi pi-desktop";
    case MacroActionType.CONSUMER:
      return "pi pi-volume-up";
    default:
      return "pi pi-question";
  }
}

function isKeyboardAction(action: MacroAction): boolean {
  return (
    action.type === MacroActionType.KEY_DOWN ||
    action.type === MacroActionType.KEY_UP ||
    action.type === MacroActionType.MOD_DOWN ||
    action.type === MacroActionType.MOD_UP
  );
}

function actionDescription(action: MacroAction): string {
  switch (action.type) {
    case MacroActionType.KEY_DOWN:
      return `按下 ${KEYCODE_NAMES[action.param] || `0x${action.param.toString(16)}`}`;
    case MacroActionType.KEY_UP:
      return `释放 ${KEYCODE_NAMES[action.param] || `0x${action.param.toString(16)}`}`;
    case MacroActionType.MOD_DOWN:
      return `按下 ${MOD_NAMES[action.param] || `Mod(0x${action.param.toString(16)})`}`;
    case MacroActionType.MOD_UP:
      return `释放 ${MOD_NAMES[action.param] || `Mod(0x${action.param.toString(16)})`}`;
    case MacroActionType.MOUSE_DOWN:
      return `按下 ${MOUSE_BTN_NAMES[action.param] || "鼠标键"}`;
    case MacroActionType.MOUSE_UP:
      return `释放 ${MOUSE_BTN_NAMES[action.param] || "鼠标键"}`;
    case MacroActionType.WHEEL:
      return action.param === 1 ? "滚轮向上" : "滚轮向下";
    case MacroActionType.CONSUMER:
      return getConsumerName(action.param) || `Consumer(0x${action.param.toString(16)})`;
    default:
      return "未知动作";
  }
}

function scrollToBottom(): void {
  requestAnimationFrame(() => {
    if (pipelineRef.value) {
      pipelineRef.value.scrollTop = pipelineRef.value.scrollHeight;
    }
  });
}

function removeCard(idx: number): void {
  if (capturingIdx.value === idx) {
    cancelCapture();
  }
  macroStore.editingCards.splice(idx, 1);
}

function normalizeDelay(delayMs: number): number {
  if (delayMs <= 0) {
    return 0;
  }
  return Math.max(10, Math.round(delayMs / 10) * 10);
}

function setDelayMs(idx: number, delayMs: number): void {
  const card = macroStore.editingCards[idx];
  if (!card) {
    return;
  }
  macroStore.editingCards[idx] = {
    ...card,
    delayMs: normalizeDelay(delayMs),
  };
}

function onDelayInput(idx: number, event: Event): void {
  const target = event.target as HTMLInputElement | null;
  if (!target) {
    return;
  }

  const nextDelay = Number(target.value);
  const card = macroStore.editingCards[idx];
  if (!card) {
    return;
  }

  if (!Number.isFinite(nextDelay) || nextDelay < 0) {
    target.value = String(card.delayMs);
    return;
  }

  const normalized = normalizeDelay(nextDelay);
  setDelayMs(idx, normalized);
  target.value = String(normalized);
}

function addCard(action: MacroAction, delayMs = 0): void {
  macroStore.editingCards.push({
    action: { ...action },
    delayMs,
  });
}

function addKeyAction(): void {
  showMouseMenu.value = false;
  showConsumerMenu.value = false;

  const baseIdx = macroStore.editingCards.length;
  addCard({ type: MacroActionType.KEY_DOWN, param: 0x2c }, 50);
  addCard({ type: MacroActionType.KEY_UP, param: 0x2c }, 0);
  scrollToBottom();
  startCaptureForCard(baseIdx);
}

function addMouseClick(btnMask: number): void {
  addCard({ type: MacroActionType.MOUSE_DOWN, param: btnMask }, 50);
  addCard({ type: MacroActionType.MOUSE_UP, param: btnMask }, 0);
  scrollToBottom();
}

function addWheel(direction: number): void {
  addCard({ type: MacroActionType.WHEEL, param: direction }, 0);
  scrollToBottom();
}

function addConsumer(code: number): void {
  addCard({ type: MacroActionType.CONSUMER, param: code }, 0);
  scrollToBottom();
}

function replaceCardAction(idx: number, action: MacroAction): void {
  const card = macroStore.editingCards[idx];
  if (!card) {
    return;
  }
  macroStore.editingCards[idx] = {
    ...card,
    action: { ...action },
  };
}

function findMatchingPairIndex(idx: number): number {
  const card = macroStore.editingCards[idx];
  if (!card) {
    return -1;
  }

  const pair = getPairTypes(card.action.type);
  if (!pair) {
    return -1;
  }

  if (card.action.type === pair.press) {
    for (let i = idx + 1; i < macroStore.editingCards.length; i++) {
      const other = macroStore.editingCards[i].action;
      if (other.type === pair.release && other.param === card.action.param) {
        return i;
      }
      if (other.type === pair.press && other.param === card.action.param) {
        break;
      }
    }
    return -1;
  }

  for (let i = idx - 1; i >= 0; i--) {
    const other = macroStore.editingCards[i].action;
    if (other.type === pair.press && other.param === card.action.param) {
      return i;
    }
    if (other.type === pair.release && other.param === card.action.param) {
      break;
    }
  }

  return -1;
}

function canMoveCard(event: {
  draggedContext?: {
    index: number;
    futureIndex?: number;
    element?: { action?: MacroAction };
  };
}): boolean {
  const draggedIndex = event.draggedContext?.index ?? -1;
  const futureIndex = event.draggedContext?.futureIndex ?? -1;

  if (draggedIndex < 0 || futureIndex < 0) {
    return true;
  }

  const draggedCard = macroStore.editingCards[draggedIndex];
  if (!draggedCard) {
    return true;
  }

  const pairIndex = findMatchingPairIndex(draggedIndex);
  if (pairIndex < 0) {
    return true;
  }

  if (isReleaseAction(draggedCard.action.type) && futureIndex <= pairIndex) {
    return false;
  }

  if (isPressAction(draggedCard.action.type) && futureIndex >= pairIndex) {
    return false;
  }

  return true;
}

function startCaptureForCard(idx: number): void {
  if (isRecording.value) {
    return;
  }
  capturingIdx.value = idx;
  window.addEventListener("keydown", handleCardCapture, true);
}

function cancelCapture(): void {
  capturingIdx.value = -1;
  window.removeEventListener("keydown", handleCardCapture, true);
}

function handleCardCapture(event: KeyboardEvent): void {
  event.preventDefault();
  event.stopPropagation();

  if (event.code === "Escape") {
    cancelCapture();
    return;
  }

  const { keycode, modifier } = getHidFromEvent(event);
  if (keycode === 0 && modifier === 0) {
    return;
  }

  const idx = capturingIdx.value;
  const card = macroStore.editingCards[idx];
  if (!card) {
    cancelCapture();
    return;
  }

  if (card.action.type === MacroActionType.KEY_DOWN || card.action.type === MacroActionType.KEY_UP) {
    const nextKeycode = keycode > 0 ? keycode : card.action.param;
    replaceCardAction(idx, { type: card.action.type, param: nextKeycode });

    const pairIdx = findMatchingKeyAction(idx);
    if (pairIdx >= 0) {
      replaceCardAction(pairIdx, {
        type: macroStore.editingCards[pairIdx].action.type,
        param: nextKeycode,
      });
    }

    if (modifier !== 0) {
      ensureModifierWrap(idx, modifier);
    }
  } else if (card.action.type === MacroActionType.MOD_DOWN || card.action.type === MacroActionType.MOD_UP) {
    const nextMod = modifier || card.action.param;
    replaceCardAction(idx, { type: card.action.type, param: nextMod });

    const pairType =
      card.action.type === MacroActionType.MOD_DOWN
        ? MacroActionType.MOD_UP
        : MacroActionType.MOD_DOWN;
    const direction = card.action.type === MacroActionType.MOD_DOWN ? 1 : -1;

    for (
      let i = idx + direction;
      i >= 0 && i < macroStore.editingCards.length;
      i += direction
    ) {
      const other = macroStore.editingCards[i].action;
      if (other.type === pairType && other.param === card.action.param) {
        replaceCardAction(i, { type: pairType, param: nextMod });
        break;
      }
    }
  }

  cancelCapture();
}

function findMatchingKeyAction(idx: number): number {
  const pairIndex = findMatchingPairIndex(idx);
  if (pairIndex < 0) {
    return -1;
  }
  const card = macroStore.editingCards[idx];
  return card &&
    (card.action.type === MacroActionType.KEY_DOWN ||
      card.action.type === MacroActionType.KEY_UP)
    ? pairIndex
    : -1;
}

function ensureModifierWrap(keyIdx: number, modifier: number): void {
  const pairIdx = findMatchingKeyAction(keyIdx);
  if (pairIdx < 0) {
    return;
  }

  const keyDownIdx =
    macroStore.editingCards[keyIdx].action.type === MacroActionType.KEY_DOWN ? keyIdx : pairIdx;
  let keyUpIdx =
    macroStore.editingCards[keyIdx].action.type === MacroActionType.KEY_UP ? keyIdx : pairIdx;

  const modBits = [0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80];
  let insertsBefore = 0;

  for (const bit of modBits) {
    if (!(modifier & bit)) {
      continue;
    }

    let found = false;
    for (let i = keyDownIdx - 1 + insertsBefore; i >= 0; i--) {
      const action = macroStore.editingCards[i].action;
      if (action.type === MacroActionType.MOD_DOWN && action.param === bit) {
        found = true;
        break;
      }
      if (action.type === MacroActionType.MOD_UP && action.param === bit) {
        break;
      }
      if (action.type === MacroActionType.KEY_DOWN) {
        break;
      }
    }

    if (!found) {
      macroStore.editingCards.splice(keyDownIdx + insertsBefore, 0, {
        action: { type: MacroActionType.MOD_DOWN, param: bit },
        delayMs: 0,
      });
      insertsBefore++;
    }
  }

  keyUpIdx += insertsBefore;

  let insertsAfter = 0;
  for (const bit of [...modBits].reverse()) {
    if (!(modifier & bit)) {
      continue;
    }

    let found = false;
    for (let i = keyUpIdx + 1 + insertsAfter; i < macroStore.editingCards.length; i++) {
      const action = macroStore.editingCards[i].action;
      if (action.type === MacroActionType.MOD_UP && action.param === bit) {
        found = true;
        break;
      }
      if (action.type === MacroActionType.MOD_DOWN && action.param === bit) {
        break;
      }
      if (action.type === MacroActionType.KEY_UP) {
        break;
      }
    }

    if (!found) {
      macroStore.editingCards.splice(keyUpIdx + 1 + insertsAfter, 0, {
        action: { type: MacroActionType.MOD_UP, param: bit },
        delayMs: 0,
      });
      insertsAfter++;
    }
  }
}

function codeToHid(code: string): number {
  const fakeEvent = {
    code,
    key: "",
    ctrlKey: false,
    shiftKey: false,
    altKey: false,
    metaKey: false,
    preventDefault() {},
    stopPropagation() {},
  } as unknown as KeyboardEvent;
  return getHidFromEvent(fakeEvent).keycode;
}

function appendRecordedAction(action: MacroAction): void {
  const now = performance.now();
  const lastIdx = macroStore.editingCards.length - 1;

  if (lastIdx >= 0) {
    const elapsed = now - recordLastTime;
    if (elapsed > 5) {
      const delayMs = Math.max(10, Math.round(elapsed / 10) * 10);
      const lastCard = macroStore.editingCards[lastIdx];
      macroStore.editingCards[lastIdx] = {
        ...lastCard,
        delayMs: lastCard.delayMs + delayMs,
      };
    }
  }

  recordLastTime = now;
  addCard(action, 0);
  recordedCount.value++;
  scrollToBottom();
}

function startRecording(): void {
  showMouseMenu.value = false;
  showConsumerMenu.value = false;
  cancelCapture();

  isRecording.value = true;
  recordedCount.value = 0;
  recordLastTime = performance.now();

  window.addEventListener("keydown", handleRecordKeyDown, true);
  window.addEventListener("keyup", handleRecordKeyUp, true);
}

function stopRecording(): void {
  isRecording.value = false;
  window.removeEventListener("keydown", handleRecordKeyDown, true);
  window.removeEventListener("keyup", handleRecordKeyUp, true);
  scrollToBottom();
}

function cleanupInteractiveState(): void {
  showMouseMenu.value = false;
  showConsumerMenu.value = false;
  cancelCapture();
  if (isRecording.value) {
    stopRecording();
  }
}

function closeEditor(): void {
  cleanupInteractiveState();
  dialogVisible.value = false;
}

function handleRecordKeyDown(event: KeyboardEvent): void {
  event.preventDefault();
  event.stopPropagation();

  if (event.code === "Escape") {
    stopRecording();
    return;
  }
  if (event.repeat) {
    return;
  }

  const modBit = CODE_TO_MOD[event.code];
  if (modBit) {
    appendRecordedAction({ type: MacroActionType.MOD_DOWN, param: modBit });
    return;
  }

  const hid = codeToHid(event.code);
  if (hid > 0) {
    appendRecordedAction({ type: MacroActionType.KEY_DOWN, param: hid });
  }
}

function handleRecordKeyUp(event: KeyboardEvent): void {
  event.preventDefault();
  event.stopPropagation();

  const modBit = CODE_TO_MOD[event.code];
  if (modBit) {
    appendRecordedAction({ type: MacroActionType.MOD_UP, param: modBit });
    return;
  }

  const hid = codeToHid(event.code);
  if (hid > 0) {
    appendRecordedAction({ type: MacroActionType.KEY_UP, param: hid });
  }
}

async function handleSave(): Promise<void> {
  cleanupInteractiveState();
  await macroStore.saveMacro();
  dialogVisible.value = false;
}

async function handleDelete(): Promise<void> {
  cleanupInteractiveState();
  await macroStore.deleteMacro(props.slot);
  dialogVisible.value = false;
}

watch(
  () => props.visible,
  (visible) => {
    if (!visible) {
      cleanupInteractiveState();
    }
  },
);

onBeforeUnmount(() => {
  cleanupInteractiveState();
});
</script>

<style scoped>
.editor-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 0.9rem;
  width: 100%;
  min-width: 0;
}

.editor-title {
  display: flex;
  align-items: baseline;
  gap: 0.45rem;
  min-width: 0;
}

.editor-title-main {
  font-size: 0.92rem;
  font-weight: 800;
  letter-spacing: 0.01em;
}

.editor-title-slot {
  color: var(--c-text-muted);
  font-size: 0.73rem;
  font-weight: 700;
  white-space: nowrap;
}

.header-name-wrap {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  min-width: 0;
  width: min(320px, 48%);
  margin-left: auto;
}

.header-name-input {
  flex: 1;
  min-width: 0;
}

.header-name-bytes {
  color: var(--c-text-muted);
  font-size: 0.72rem;
  font-weight: 600;
  white-space: nowrap;
}

.pipeline-container {
  min-height: 80px;
  max-height: 392px;
  overflow-y: auto;
  padding: 0.35rem 0 0.15rem;
}

.pipeline-empty {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 0.75rem;
  padding: 2rem 1rem;
  color: var(--c-text-muted);
}

.pipeline-empty i {
  font-size: 2rem;
  opacity: 0.4;
}

.pipeline-list {
  display: flex;
  flex-direction: column;
  gap: 0.72rem;
}

.action-card {
  position: relative;
}

.connector {
  position: absolute;
  top: -0.72rem;
  left: 1rem;
  height: 0.72rem;
  width: 2px;
}

.connector-line {
  width: 2px;
  height: 100%;
  margin: 0 auto;
  background: linear-gradient(180deg, transparent, var(--c-border));
}

.card-body {
  display: grid;
  grid-template-columns: 24px 34px minmax(0, 1fr) auto 24px;
  align-items: center;
  gap: 0.48rem;
  padding: 0.52rem 0.62rem;
  border: 1px solid var(--c-border);
  border-radius: 13px;
  background: color-mix(in srgb, var(--c-bg-secondary) 94%, white 6%);
}

.card-drag,
.card-remove {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 24px;
  height: 24px;
  border: 0;
  border-radius: 999px;
  background: transparent;
  color: var(--c-text-muted);
  font-size: 0.8rem;
}

.card-drag {
  cursor: grab;
}

.card-icon {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 34px;
  height: 34px;
  border-radius: 10px;
  background: var(--c-bg-tertiary, rgba(255, 255, 255, 0.06));
  color: var(--c-accent);
  font-size: 0.92rem;
}

.card-content {
  min-width: 0;
  display: flex;
  align-items: center;
  gap: 0.38rem;
}

.card-category {
  display: inline-flex;
  align-items: center;
  padding: 0.12rem 0.34rem;
  border-radius: 999px;
  background: color-mix(in srgb, var(--c-accent-soft) 65%, transparent);
  color: var(--c-text-muted);
  font-size: 0.66rem;
  font-weight: 700;
  white-space: nowrap;
}

.card-desc {
  display: inline-flex;
  align-items: center;
  gap: 0.34rem;
  min-width: 0;
  color: var(--c-text-primary);
  font-size: 0.8rem;
  line-height: 1.25;
}

.card-desc-btn {
  padding: 0;
  border: 0;
  background: transparent;
  cursor: pointer;
}

.card-desc-btn.capturing {
  color: var(--c-danger);
}

.card-edit-hint {
  color: var(--c-text-muted);
  font-size: 0.72rem;
}

.card-delay {
  display: flex;
  align-items: center;
  gap: 0.34rem;
  padding-left: 0.12rem;
}

.delay-label {
  color: var(--c-text-muted);
  font-size: 0.66rem;
  white-space: nowrap;
}

.delay-input-wrap {
  display: inline-flex;
  align-items: center;
  gap: 0.34rem;
  padding: 0.14rem 0.18rem 0.14rem 0.48rem;
  border: 1px solid var(--c-border);
  border-radius: 999px;
  background: color-mix(in srgb, var(--c-bg-primary) 84%, white 16%);
}

.delay-input {
  width: 68px;
  padding: 0;
  border: 0;
  outline: 0;
  background: transparent;
  color: var(--c-text-primary);
  text-align: right;
  font-size: 0.74rem;
  font-weight: 700;
}

.delay-input::-webkit-outer-spin-button,
.delay-input::-webkit-inner-spin-button {
  margin: 0;
}

.delay-unit {
  color: var(--c-text-muted);
  font-size: 0.7rem;
  font-weight: 700;
  white-space: nowrap;
}

.cat-keyboard .card-icon {
  color: #60a5fa;
}

.cat-mouse .card-icon {
  color: #34d399;
}

.cat-consumer .card-icon {
  color: #f59e0b;
}

.pipeline-stats {
  margin-top: 0.5rem;
  color: var(--c-text-muted);
  font-size: 0.78rem;
}

.pipeline-stats.over {
  color: var(--c-danger);
  font-weight: 700;
}

.add-bar {
  display: flex;
  flex-wrap: nowrap;
  gap: 0.38rem;
  overflow-x: auto;
  padding: 0.28rem;
  border: 1px solid var(--c-border);
  border-radius: 14px;
  background: color-mix(in srgb, var(--c-bg-secondary) 92%, white 8%);
}

.add-btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 0.34rem;
  min-width: 72px;
  padding: 0.52rem 0.68rem;
  border: 1px solid var(--c-border);
  border-radius: 11px;
  background: color-mix(in srgb, var(--c-bg-primary) 84%, white 16%);
  color: var(--c-text-primary);
  font-size: 0.76rem;
  font-weight: 700;
  white-space: nowrap;
}

.add-btn.recording {
  border-color: var(--c-danger);
  color: var(--c-danger);
  background: color-mix(in srgb, var(--c-danger) 10%, var(--c-bg-primary));
}

.sub-menu {
  margin-top: 0.55rem;
  padding: 0.68rem;
  border: 1px solid var(--c-border);
  border-radius: 14px;
  background: color-mix(in srgb, var(--c-bg-secondary) 92%, white 8%);
}

.sub-menu-title {
  margin-bottom: 0.45rem;
  color: var(--c-text-muted);
  font-size: 0.76rem;
  font-weight: 700;
}

.sub-menu-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(92px, 1fr));
  gap: 0.45rem;
}

.sub-menu-grid.cols-3 {
  grid-template-columns: repeat(3, minmax(0, 1fr));
}

.sub-menu-item {
  padding: 0.55rem 0.68rem;
  border: 1px solid var(--c-border);
  border-radius: 12px;
  background: var(--c-bg-primary);
  color: var(--c-text-primary);
  font-size: 0.78rem;
}

.editor-footer {
  display: flex;
  align-items: center;
  justify-content: space-between;
  width: 100%;
  gap: 0.7rem;
  flex-wrap: wrap;
}

.footer-toolbar {
  flex: 1;
  min-width: min(100%, 320px);
}

.footer-actions {
  display: flex;
  align-items: center;
  gap: 0.75rem;
  margin-left: auto;
}

@media (max-width: 768px) {
  .editor-header {
    align-items: flex-start;
    flex-direction: column;
  }

  .header-name-wrap {
    width: 100%;
    max-width: none;
    margin-left: 0;
  }

  .card-body {
    grid-template-columns: 24px 34px minmax(0, 1fr) 24px;
  }

  .card-delay {
    grid-column: 2 / -2;
    justify-content: flex-start;
    flex-wrap: wrap;
  }

  .editor-footer {
    align-items: stretch;
  }

  .footer-actions {
    width: 100%;
    justify-content: flex-end;
  }

  .add-btn {
    min-width: 68px;
    padding: 0.5rem 0.64rem;
  }
}
</style>
