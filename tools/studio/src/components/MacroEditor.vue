<template>
  <Dialog v-model:visible="dialogVisible" :style="{ width: 'min(1120px, calc(100vw - 24px))' }" modal
    :closable="!isRecording && capturingIdx < 0" :closeOnEscape="!isRecording && capturingIdx < 0"
    class="macro-editor-dialog" :showHeader="false" @hide="cleanupInteractiveState">

    <div class="ide-layout">
      <!-- 左侧文件栏 -->
      <aside class="ide-sidebar">
        <div class="sidebar-header">
          <i class="pi pi-code"></i>
          <span>宏</span>
          <button class="sidebar-close" @click="closeEditor" v-tooltip.right="'关闭编辑器'">
            <i class="pi pi-times"></i>
          </button>
        </div>
        <div class="sidebar-files">
          <button v-for="i in macroStore.maxSlots" :key="i" class="sidebar-file"
            :class="{ active: activeSlot === i - 1, 'has-data': macroStore.slotValid[i - 1], 'is-open': openTabs.includes(i - 1) }"
            @click="openTab(i - 1)" @dblclick="openTab(i - 1)">
            <i class="pi" :class="macroStore.slotValid[i - 1] ? 'pi-file' : 'pi-file-plus'"></i>
            <span class="sidebar-file-name">{{ macroStore.slotValid[i - 1] ? macroStore.getSlotDisplayName(i - 1) : `宏
              ${i}` }}</span>
            <span class="sidebar-file-idx">{{ i }}</span>
          </button>
        </div>
      </aside>

      <!-- 右侧主区域 -->
      <div class="ide-main">
        <!-- 标签栏（始终可见） -->
        <div class="ide-tabbar">
          <div class="ide-tabs">
            <button v-for="tabSlot in openTabs" :key="tabSlot" class="ide-tab"
              :class="{ active: activeSlot === tabSlot }" @click="switchTab(tabSlot)">
              <span class="ide-tab-label">{{ macroStore.slotValid[tabSlot] ? macroStore.getSlotDisplayName(tabSlot) : `宏
                ${tabSlot + 1}` }}</span>
              <span class="ide-tab-close" @click.stop="closeTab(tabSlot)" v-tooltip.top="'关闭'">
                <i class="pi pi-times"></i>
              </span>
            </button>
          </div>
          <!-- 名称编辑 + 模式切换（仅有标签时显示） -->
          <div v-if="openTabs.length > 0" class="ide-tabbar-actions">
            <div v-if="macroStore.maxNameBytes > 0" class="ide-name-wrap">
              <InputText :modelValue="macroStore.editingName" placeholder="名称..." class="ide-name-input"
                @update:modelValue="onNameInput" />
              <span class="ide-name-bytes">{{ nameByteCount }}/{{ macroStore.maxNameBytes }}</span>
            </div>
            <div class="editor-mode-switch" role="tablist">
              <button class="mode-btn" :class="{ active: editorMode === 'visual' }" @click="setEditorMode('visual')">
                <i class="pi pi-th-large"></i>
                <span>可视</span>
              </button>
              <button class="mode-btn" :class="{ active: editorMode === 'code' }" @click="setEditorMode('code')">
                <i class="pi pi-code"></i>
                <span>MeowMacro</span>
              </button>
            </div>
            <a class="meowmacro-doc-link" href="https://meowkj.github.io/BinaryKeyboard/meowmacro" target="_blank"
              rel="noopener" v-tooltip.bottom="'MeowMacro 语法文档'">
              <i class="pi pi-question-circle"></i>
            </a>
          </div>
          <button class="ide-close-btn" @click="closeEditor" v-tooltip.left="'关闭'">
            <i class="pi pi-times"></i>
          </button>
        </div>

        <!-- 编辑器区域 -->
        <div v-if="openTabs.length === 0" class="ide-welcome">
          <span class="watermark-letter">M</span>
          <span class="watermark-hint">从左侧选择一个宏槽位开始编辑</span>
        </div>

        <div v-else class="editor-body" :class="{ 'is-code-mode': editorMode === 'code' }">
          <div v-if="editorMode === 'visual'" class="pipeline-container" ref="pipelineRef">
            <div v-if="macroStore.editingCards.length === 0" class="pipeline-empty">
              <span class="watermark-letter">M</span>
              <span class="watermark-hint">点击下方按钮添加动作，或切到 MeowMacro 模式直接编写</span>
            </div>

            <VueDraggable v-else v-model="macroStore.editingCards" :animation="200" :move="canMoveCard"
              @start="handleDragStart" @end="handleDragEnd" handle=".card-drag" ghostClass="card-ghost"
              chosenClass="card-chosen" class="pipeline-list">
              <div v-for="(card, idx) in macroStore.editingCards" :key="idx" class="action-card" :data-card-index="idx"
                :class="[actionCategory(card.action), { 'is-hover-linked': isHoveredCard(idx) }]"
                @mouseenter="handleCardMouseEnter(idx)" @mouseleave="handleCardMouseLeave">
                <div class="card-body">
                  <div class="card-drag" v-tooltip.left="'拖拽排序'">
                    <span class="drag-dots" aria-hidden="true">
                      <span v-for="dot in 6" :key="dot"></span>
                    </span>
                  </div>

                  <div class="card-content">
                    <span class="card-category">
                      <i :class="actionIcon(card.action)"></i>
                      <span>{{ actionCategoryLabel(card.action) }}</span>
                    </span>

                    <button v-if="isKeyboardAction(card.action)" class="card-desc card-desc-btn"
                      :class="{ capturing: capturingIdx === idx }" @click="startCaptureForCard(idx)">
                      <template v-if="capturingIdx === idx">按下按键…</template>
                      <template v-else>
                        {{ actionDescription(card.action) }}
                        <i class="pi pi-pencil card-edit-hint"></i>
                      </template>
                    </button>

                    <span v-else class="card-desc">{{ actionDescription(card.action) }}</span>
                  </div>

                  <div class="card-delay">
                    <label class="delay-label">延时</label>
                    <div class="delay-input-wrap">
                      <input :value="getDelayInputValue(idx, card.delayMs)" type="number" min="0" step="10"
                        inputmode="numeric" class="delay-input" @focus="startDelayEdit(idx)"
                        @input="handleDelayDraftInput(idx, $event)" @blur="commitDelayInput(idx, $event)"
                        @keydown.enter.prevent="commitDelayInput(idx, $event)"
                        @keydown.esc.prevent="revertDelayInput(idx, $event)" @wheel="handleDelayWheel" />
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

          <div v-else class="code-editor-panel">
            <div class="code-editor-shell">
              <div class="code-editor-gutter" aria-hidden="true">
                <div ref="codeGutterTrackRef" class="code-editor-gutter-track">
                  <span v-for="line in codeLineNumbers" :key="line" class="code-editor-line-number"
                    :class="{ 'has-error': codeDiagnosticLines.has(line) }">{{ line }}</span>
                </div>
              </div>
              <div ref="codeEditorStageRef" class="code-editor-stage">
                <pre ref="codeHighlightRef" class="code-editor-highlight" aria-hidden="true"
                  v-html="highlightedCodeHtml"></pre>
                <div ref="codeMeasureRef" class="code-editor-measure" aria-hidden="true">{{ codeTextBeforeCaret }}<span
                    ref="codeCaretMarkerRef" class="code-editor-caret-marker"></span></div>
                <div v-if="!codeText" class="code-editor-placeholder">
                  <span class="watermark-letter">M</span>
                </div>
                <textarea ref="codeTextareaRef" v-model="codeText" class="code-editor-input" spellcheck="false"
                  autocomplete="off" autocorrect="off" autocapitalize="off" wrap="off" @scroll="handleCodeScroll"
                  @input="handleCodeInput" @keydown="handleCodeKeyDown" @click="refreshCodeCompletions"
                  @keyup="handleCodeKeyUp" @focus="handleCodeFocus" @blur="handleCodeBlur"></textarea>
                <div v-if="codeCompletionVisible" class="code-completion"
                  :style="{ top: `${codeCompletionPosition.top}px`, left: `${codeCompletionPosition.left}px` }">
                  <button v-for="(item, index) in codeCompletions" :key="`${item.kind}-${item.label}-${index}`"
                    class="code-completion-item" :class="{ active: index === codeCompletionIndex }"
                    @mousedown.prevent="applyCodeCompletion(index)">
                    <span class="code-completion-main">
                      <span class="code-completion-label">{{ item.label }}</span>
                      <span class="code-completion-detail">{{ item.detail }}</span>
                    </span>
                    <span class="code-completion-kind">{{ completionKindLabel(item.kind) }}</span>
                  </button>
                </div>
              </div>
            </div>
          </div>

          <div v-if="activeDiagnostics.length > 0" class="diagnostic-panel">
            <div v-for="(diagnostic, idx) in activeDiagnostics" :key="`${diagnostic.code}-${idx}`"
              class="diagnostic-item">
              <span class="diagnostic-location">{{ formatDiagnosticLocation(diagnostic) }}</span>
              <span class="diagnostic-message">{{ diagnostic.message }}</span>
            </div>
          </div>

          <div v-if="editorMode === 'visual' && showModifierMenu" class="sub-menu">
            <div class="sub-menu-title">修饰键</div>
            <div class="sub-menu-grid">
              <button v-for="modifier in modifierOptions" :key="modifier.param" class="sub-menu-item"
                @click="addModifier(modifier.param); showModifierMenu = false">
                {{ modifier.label }}
              </button>
            </div>
          </div>

          <div v-if="editorMode === 'visual' && showMouseMenu" class="sub-menu">
            <div class="sub-menu-title">鼠标按键</div>
            <div class="sub-menu-grid">
              <button v-for="btn in mouseOptions" :key="btn.param" class="sub-menu-item"
                @click="addMouseClick(btn.param); showMouseMenu = false">
                {{ btn.label }}
              </button>
            </div>
            <div class="sub-menu-title">滚轮</div>
            <div class="sub-menu-grid">
              <button class="sub-menu-item" @click="addWheel(1); showMouseMenu = false">向上</button>
              <button class="sub-menu-item" @click="addWheel(2); showMouseMenu = false">向下</button>
            </div>
          </div>

          <div v-if="editorMode === 'visual' && showConsumerMenu" class="sub-menu">
            <div class="sub-menu-grid cols-3">
              <button v-for="key in CONSUMER_KEYS" :key="key.code" class="sub-menu-item"
                @click="addConsumer(key.code); showConsumerMenu = false">
                {{ key.name }}
              </button>
            </div>
          </div>
        </div>

        <!-- 底部状态栏 -->
        <div v-if="openTabs.length > 0" class="ide-statusbar">
          <div class="statusbar-left">
            <div class="pipeline-stats" :class="{ over: displayOverLimit }">
              <span>{{ displayPrimaryCount }} {{ editorMode === 'visual' ? '卡片' : '步骤' }}</span>
              <span>·</span>
              <span>{{ displayActionCount }}/{{ macroStore.maxActions }} 动作</span>
              <span>·</span>
              <span>{{ displayDataSize }}/{{ macroStore.maxDataSize }}B</span>
            </div>
            <div v-if="editorMode === 'visual'" class="add-bar">
              <button class="add-btn add-keyboard" @click="addKeyAction" v-tooltip.top="'按键'">
                <i class="pi pi-angle-right"></i>
                <span>按键</span>
              </button>
              <button class="add-btn add-modifier"
                @click="showModifierMenu = !showModifierMenu; showMouseMenu = false; showConsumerMenu = false"
                v-tooltip.top="'修饰键'">
                <i class="pi pi-bolt"></i>
                <span>修饰</span>
              </button>
              <button class="add-btn add-mouse"
                @click="showMouseMenu = !showMouseMenu; showModifierMenu = false; showConsumerMenu = false"
                v-tooltip.top="'鼠标'">
                <i class="pi pi-desktop"></i>
                <span>鼠标</span>
              </button>
              <button class="add-btn add-consumer"
                @click="showConsumerMenu = !showConsumerMenu; showModifierMenu = false; showMouseMenu = false"
                v-tooltip.top="'媒体'">
                <i class="pi pi-volume-up"></i>
                <span>媒体</span>
              </button>
              <button class="add-btn add-record" :class="{ recording: isRecording }"
                @click="isRecording ? stopRecording() : startRecording()" v-tooltip.top="isRecording ? '停止录制' : '录制'">
                <i :class="isRecording ? 'pi pi-stop-circle' : 'pi pi-circle-fill'"></i>
                <span>{{ isRecording ? `停止 ${recordedCount}` : '录制' }}</span>
              </button>
            </div>
            <button v-if="editorMode === 'code'" class="code-toolbar-btn" @click="compileAndSimplify"
              :disabled="codeCompileState.diagnostics.length > 0" v-tooltip.top="'编译简化'">
              <i class="pi pi-bolt"></i>
              <span>编译简化</span>
            </button>
          </div>
          <div class="statusbar-right">
            <Button v-if="macroStore.slotValid[activeSlot]" label="删除" icon="pi pi-trash" severity="danger" text
              size="small" @click="handleDelete" :loading="macroStore.isLoading" />
            <Button label="保存" icon="pi pi-check" size="small" @click="handleSave" :loading="macroStore.isSaving"
              :disabled="saveDisabled" />
          </div>
        </div>
      </div>
    </div>

  </Dialog>
</template>

<script setup lang="ts">
import { computed, nextTick, onBeforeUnmount, ref, watch } from "vue";
import { VueDraggable, type MoveEvent } from "vue-draggable-plus";
import { parseActions, useMacroStore, type MacroCard } from "@/stores/macroStore";
import { showToast } from "@/services/toastService";
import {
  MacroActionType,
  type MacroAction,
} from "@/types/protocol";
import { CONSUMER_KEYS, getConsumerName } from "@/utils/consumer";
import { getHidFromEvent, KEYCODE_NAMES } from "@/utils/keycodes";
import {
  compileMacroCards,
  compileMacroDsl,
  formatMacroDslFromCards,
  getMacroDslCompletions,
  type MacroDslCompletionItem,
  type MacroDiagnostic,
} from "@/utils/macroDsl";
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

// --- IDE tab state ---
interface TabSnapshot {
  cards: MacroCard[];
  name: string;
  codeText: string;
  editorMode: "visual" | "code";
}

const openTabs = ref<number[]>([]);
const activeSlot = ref(-1);
const tabCache = ref<Map<number, TabSnapshot>>(new Map());

const pipelineRef = ref<HTMLElement | null>(null);
const codeEditorStageRef = ref<HTMLElement | null>(null);
const codeTextareaRef = ref<HTMLTextAreaElement | null>(null);
const codeHighlightRef = ref<HTMLElement | null>(null);
const codeGutterTrackRef = ref<HTMLElement | null>(null);
const codeMeasureRef = ref<HTMLElement | null>(null);
const codeCaretMarkerRef = ref<HTMLElement | null>(null);
const showModifierMenu = ref(false);
const showMouseMenu = ref(false);
const showConsumerMenu = ref(false);
const capturingIdx = ref(-1);
const editorMode = ref<"visual" | "code">("visual");
const codeText = ref("");
const codeCaretIndex = ref(0);
const codeHasFocus = ref(false);
const codeCompletions = ref<MacroDslCompletionItem[]>([]);
const codeCompletionIndex = ref(0);
const codeCompletionPosition = ref({ top: 12, left: 12 });

const isRecording = ref(false);
const recordedCount = ref(0);
let recordLastTime = 0;
const dragSnapshot = ref<MacroCard[] | null>(null);
const hoveredCardIndexes = ref<number[]>([]);
const delayDrafts = ref<Record<number, string>>({});

const nameByteCount = computed(() => utf8ByteLength(macroStore.editingName));
const visualCompileState = computed(() => compileMacroCards(macroStore.editingCards));
const codeCompileState = computed(() => compileMacroDsl(codeText.value));
const codeLineNumbers = computed(() =>
  Array.from(
    { length: Math.max(codeText.value.replace(/\r\n/g, "\n").split("\n").length, 1) },
    (_, index) => index + 1,
  ),
);
const codeTextBeforeCaret = computed(() => codeText.value.slice(0, codeCaretIndex.value));
const highlightedCodeHtml = computed(() => renderHighlightedCode(codeText.value, codeCompileState.value.diagnostics));
const codeCompletionVisible = computed(
  () => editorMode.value === "code" && codeHasFocus.value && codeCompletions.value.length > 0,
);
const codeDiagnosticLines = computed(() => {
  const lines = new Set<number>();
  for (const d of codeCompileState.value.diagnostics) {
    if (d.source?.line) lines.add(d.source.line);
  }
  return lines;
});
const activeDiagnostics = computed(() =>
  editorMode.value === "code"
    ? codeCompileState.value.diagnostics
    : visualCompileState.value.diagnostics,
);
const displayPrimaryCount = computed(() =>
  editorMode.value === "code"
    ? codeCompileState.value.steps.length
    : macroStore.editingCards.length,
);
const displayActionCount = computed(() =>
  editorMode.value === "code"
    ? codeCompileState.value.actionCount
    : macroStore.editingActionCount,
);
const displayDataSize = computed(() =>
  editorMode.value === "code"
    ? codeCompileState.value.dataSize
    : macroStore.editingDataSize,
);
const displayOverLimit = computed(() =>
  editorMode.value === "code"
    ? codeCompileState.value.actionCount > macroStore.maxActions || codeCompileState.value.dataSize > macroStore.maxDataSize
    : macroStore.editingOverLimit,
);
const saveDisabled = computed(() => {
  if (editorMode.value === "code") {
    return (
      codeCompileState.value.steps.length === 0 ||
      codeCompileState.value.diagnostics.length > 0
    );
  }

  return macroStore.editingOverLimit || macroStore.editingCards.length === 0 || activeDiagnostics.value.length > 0;
});

const mouseOptions = [
  { label: "左键", param: 0x01 },
  { label: "右键", param: 0x02 },
  { label: "中键", param: 0x04 },
  { label: "后退", param: 0x08 },
  { label: "前进", param: 0x10 },
];

const modifierOptions = [
  { label: "Ctrl", param: 0x01 },
  { label: "Shift", param: 0x02 },
  { label: "Alt", param: 0x04 },
  { label: "Win", param: 0x08 },
  { label: "RCtrl", param: 0x10 },
  { label: "RShift", param: 0x20 },
  { label: "RAlt", param: 0x40 },
  { label: "RWin", param: 0x80 },
];

const MOD_NAMES: Record<number, string> = {
  0x01: "Ctrl",
  0x02: "Shift",
  0x04: "Alt",
  0x08: "Win",
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
  macroStore.editingName = macroStore.maxNameBytes > 0
    ? truncateUtf8ByBytes(String(value ?? ""), macroStore.maxNameBytes)
    : "";
}

function escapeHtml(value: string): string {
  return value
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;");
}

function highlightDslLine(line: string): string {
  const commentIndex = line.indexOf("#");
  const codePart = commentIndex >= 0 ? line.slice(0, commentIndex) : line;
  const commentPart = commentIndex >= 0 ? line.slice(commentIndex) : "";

  let html = escapeHtml(codePart);
  html = html.replace(
    /^(\s*)(tap|down|up|delay|mouse|wheel|consumer)\b/gi,
    (_, spaces: string, keyword: string) =>
      `${spaces}<span class="token-keyword">${keyword}</span>`,
  );
  html = html.replace(
    /\b(LCtrl|LShift|LAlt|LWin|RCtrl|RShift|RAlt|RWin|Ctrl|Shift|Alt|Win|left|right|middle|back|forward|hold|wait|play_pause|next_track|prev_track|volume_up|volume_down|mute|browser_home|browser_back|browser_forward|browser_refresh|brightness_up|brightness_down)\b/gi,
    (token: string) => `<span class="token-symbol">${token}</span>`,
  );
  html = html.replace(/\b\d+(?:\.\d+)?(?:ms|s)?\b/gi, (token: string) => `<span class="token-number">${token}</span>`);
  html = html.replace(/[+*]/g, (token: string) => `<span class="token-operator">${token}</span>`);

  if (commentPart) {
    html += `<span class="token-comment">${escapeHtml(commentPart)}</span>`;
  }

  return html || "&nbsp;";
}

function renderHighlightedCode(source: string, diagnostics: MacroDiagnostic[]): string {
  const normalized = source.replace(/\r\n/g, "\n");
  const errorMap = new Map<number, string>();
  for (const d of diagnostics) {
    if (d.source?.line && !errorMap.has(d.source.line)) {
      errorMap.set(d.source.line, d.message);
    }
  }
  return normalized.split("\n").map((line, i) => {
    let html = highlightDslLine(line);
    const errMsg = errorMap.get(i + 1);
    if (errMsg) {
      html += `<span class="token-error-lens">${escapeHtml(errMsg)}</span>`;
    }
    return html;
  }).join("\n");
}

function syncCodeEditorDecorations(): void {
  const textarea = codeTextareaRef.value;
  if (!textarea) {
    return;
  }

  if (codeHighlightRef.value) {
    codeHighlightRef.value.style.transform = `translate(${-textarea.scrollLeft}px, ${-textarea.scrollTop}px)`;
  }

  if (codeGutterTrackRef.value) {
    codeGutterTrackRef.value.style.transform = `translateY(${-textarea.scrollTop}px)`;
  }
}

function handleCodeScroll(): void {
  syncCodeEditorDecorations();
  updateCodeCompletionPosition();
}

function updateCodeCaretFromTextarea(): void {
  const textarea = codeTextareaRef.value;
  if (!textarea) {
    return;
  }
  codeCaretIndex.value = textarea.selectionStart ?? 0;
}

async function updateCodeCompletionPosition(): Promise<void> {
  if (!codeCompletionVisible.value) {
    return;
  }

  await nextTick();

  const stage = codeEditorStageRef.value;
  const textarea = codeTextareaRef.value;
  const marker = codeCaretMarkerRef.value;
  if (!stage || !textarea || !marker) {
    return;
  }

  const left = Math.max(10, Math.min(
    marker.offsetLeft - textarea.scrollLeft + 12,
    stage.clientWidth - 320,
  ));
  const top = Math.max(
    10,
    marker.offsetTop - textarea.scrollTop + 28,
  );

  codeCompletionPosition.value = { top, left };
}

async function refreshCodeCompletions(): Promise<void> {
  updateCodeCaretFromTextarea();
  const textarea = codeTextareaRef.value;
  if (!textarea) {
    return;
  }

  if ((textarea.selectionEnd ?? 0) !== (textarea.selectionStart ?? 0)) {
    codeCompletions.value = [];
    return;
  }

  const suggestions = getMacroDslCompletions(codeText.value, codeCaretIndex.value).slice(0, 10);
  codeCompletions.value = suggestions;
  if (codeCompletionIndex.value >= suggestions.length) {
    codeCompletionIndex.value = 0;
  }

  await updateCodeCompletionPosition();
}

function completionKindLabel(kind: MacroDslCompletionItem["kind"]): string {
  switch (kind) {
    case "keyword":
      return "关键字";
    case "snippet":
      return "模板";
    case "symbol":
      return "符号";
    case "value":
      return "值";
    default:
      return "提示";
  }
}

async function applyCodeCompletion(index: number): Promise<void> {
  const item = codeCompletions.value[index];
  if (!item) {
    return;
  }

  const nextText =
    codeText.value.slice(0, item.replaceFrom) +
    item.insertText +
    codeText.value.slice(item.replaceTo);
  const nextCaret = item.replaceFrom + item.insertText.length;

  codeText.value = nextText;
  codeCompletionIndex.value = 0;
  codeCompletions.value = [];

  await nextTick();

  if (codeTextareaRef.value) {
    codeTextareaRef.value.focus();
    codeTextareaRef.value.selectionStart = nextCaret;
    codeTextareaRef.value.selectionEnd = nextCaret;
  }

  updateCodeCaretFromTextarea();
}

async function insertCodeTextAtSelection(text: string): Promise<void> {
  const textarea = codeTextareaRef.value;
  if (!textarea) {
    return;
  }

  const selectionStart = textarea.selectionStart ?? 0;
  const selectionEnd = textarea.selectionEnd ?? selectionStart;
  codeText.value =
    codeText.value.slice(0, selectionStart) +
    text +
    codeText.value.slice(selectionEnd);

  const nextCaret = selectionStart + text.length;
  await nextTick();

  textarea.focus();
  textarea.selectionStart = nextCaret;
  textarea.selectionEnd = nextCaret;
  updateCodeCaretFromTextarea();
  await refreshCodeCompletions();
}

function handleCodeInput(): void {
  refreshCodeCompletions();
}

function handleCodeFocus(): void {
  codeHasFocus.value = true;
  refreshCodeCompletions();
}

function handleCodeBlur(): void {
  codeHasFocus.value = false;
  window.setTimeout(() => {
    codeCompletions.value = [];
  }, 80);
}

function scrollCompletionItemIntoView(): void {
  nextTick(() => {
    const active = document.querySelector(".code-completion-item.active");
    active?.scrollIntoView({ block: "nearest" });
  });
}

function handleCodeKeyDown(event: KeyboardEvent): void {
  if (event.ctrlKey && event.key === " ") {
    event.preventDefault();
    void refreshCodeCompletions();
    return;
  }

  if (event.key === "Tab") {
    event.preventDefault();
    if (codeCompletionVisible.value) {
      void applyCodeCompletion(codeCompletionIndex.value);
      return;
    }
    void insertCodeTextAtSelection("  ");
    return;
  }

  if (!codeCompletionVisible.value) {
    return;
  }

  if (event.key === "ArrowDown") {
    event.preventDefault();
    codeCompletionIndex.value = (codeCompletionIndex.value + 1) % codeCompletions.value.length;
    scrollCompletionItemIntoView();
    return;
  }

  if (event.key === "ArrowUp") {
    event.preventDefault();
    codeCompletionIndex.value =
      (codeCompletionIndex.value - 1 + codeCompletions.value.length) % codeCompletions.value.length;
    scrollCompletionItemIntoView();
    return;
  }

  if (event.key === "Escape") {
    event.preventDefault();
    codeCompletions.value = [];
  }
}

function handleCodeKeyUp(event: KeyboardEvent): void {
  if (["ArrowDown", "ArrowUp", "Tab", "Escape"].includes(event.key)) {
    return;
  }
  refreshCodeCompletions();
}

function syncCodeFromCards(): void {
  codeText.value = formatMacroDslFromCards(macroStore.editingCards);
  requestAnimationFrame(syncCodeEditorDecorations);
  requestAnimationFrame(() => {
    updateCodeCaretFromTextarea();
    void refreshCodeCompletions();
  });
}

function compileAndSimplify(): void {
  const compiled = codeCompileState.value;
  if (compiled.diagnostics.length > 0) return;
  const cards = parseActions(compiled.actions);
  codeText.value = formatMacroDslFromCards(cards);
  requestAnimationFrame(syncCodeEditorDecorations);
}

// --- Tab management ---
function snapshotCurrentTab(): void {
  if (activeSlot.value < 0) return;
  tabCache.value.set(activeSlot.value, {
    cards: macroStore.editingCards.map((c) => ({ action: { ...c.action }, delayMs: c.delayMs })),
    name: macroStore.editingName,
    codeText: codeText.value,
    editorMode: editorMode.value,
  });
}

function restoreTabSnapshot(slot: number): void {
  const snap = tabCache.value.get(slot);
  if (snap) {
    macroStore.editingCards = snap.cards.map((c) => ({ action: { ...c.action }, delayMs: c.delayMs }));
    macroStore.editingName = snap.name;
    macroStore.editingSlot = slot;
    editorMode.value = snap.editorMode;
    codeText.value = snap.codeText;
  }
}

async function openTab(slot: number): Promise<void> {
  if (activeSlot.value === slot) return;

  cleanupInteractiveState();
  snapshotCurrentTab();

  if (!openTabs.value.includes(slot)) {
    openTabs.value.push(slot);
  }

  const cached = tabCache.value.get(slot);
  if (cached) {
    activeSlot.value = slot;
    restoreTabSnapshot(slot);
  } else {
    await macroStore.startEditing(slot);
    activeSlot.value = slot;
    editorMode.value = "visual";
    codeText.value = "";
    syncCodeFromCards();
  }

  requestAnimationFrame(syncCodeEditorDecorations);
}

function switchTab(slot: number): void {
  if (activeSlot.value === slot) return;
  void openTab(slot);
}

function closeTab(slot: number): void {
  cleanupInteractiveState();
  const idx = openTabs.value.indexOf(slot);
  if (idx < 0) return;

  tabCache.value.delete(slot);
  openTabs.value.splice(idx, 1);

  if (activeSlot.value === slot) {
    if (openTabs.value.length > 0) {
      const nextSlot = openTabs.value[Math.min(idx, openTabs.value.length - 1)];
      activeSlot.value = -1;
      void openTab(nextSlot);
    } else {
      activeSlot.value = -1;
      macroStore.cancelEditing();
    }
  }
}

function applyCodeToCards(): boolean {
  const compiled = codeCompileState.value;
  if (compiled.diagnostics.length > 0) {
    return false;
  }

  macroStore.editingCards = parseActions(compiled.actions);
  return true;
}

function setEditorMode(mode: "visual" | "code"): void {
  if (mode === editorMode.value) {
    return;
  }

  if (mode === "code") {
    cleanupInteractiveState();
    syncCodeFromCards();
    editorMode.value = "code";
    requestAnimationFrame(syncCodeEditorDecorations);
    return;
  }

  if (!applyCodeToCards()) {
    showToast("error", "脚本无法切回可视模式", "请先修复 MeowMacro 代码中的错误");
    return;
  }

  editorMode.value = "visual";
}

function formatDiagnosticLocation(diagnostic: MacroDiagnostic): string {
  if (diagnostic.source?.mode === "code" && diagnostic.source.line) {
    return `第 ${diagnostic.source.line} 行`;
  }

  if (diagnostic.source?.mode === "visual" && diagnostic.source.cardIndex !== undefined) {
    return `第 ${diagnostic.source.cardIndex + 1} 张卡片`;
  }

  return "编译器";
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
      return "修饰";
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
      return "pi pi-bolt";
    case MacroActionType.MOUSE_DOWN:
    case MacroActionType.MOUSE_UP:
    case MacroActionType.WHEEL:
      return "pi pi-desktop";
    case MacroActionType.CONSUMER:
      return "pi pi-volume-up";
    default:
      return "pi pi-code";
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
      return action.param === 0
        ? "按下 未设置"
        : `按下 ${KEYCODE_NAMES[action.param] || `0x${action.param.toString(16)}`}`;
    case MacroActionType.KEY_UP:
      return action.param === 0
        ? "释放 未设置"
        : `释放 ${KEYCODE_NAMES[action.param] || `0x${action.param.toString(16)}`}`;
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
  const pairIdx = findMatchingPairIndex(idx);
  if (capturingIdx.value === idx || capturingIdx.value === pairIdx) {
    cancelCapture();
  }
  resetDelayDrafts();

  const removeIndexes = pairIdx >= 0 && pairIdx !== idx
    ? [idx, pairIdx].sort((a, b) => b - a)
    : [idx];

  for (const removeIndex of removeIndexes) {
    macroStore.editingCards.splice(removeIndex, 1);
  }
}

function normalizeDelay(delayMs: number): number {
  if (delayMs <= 0) {
    return 0;
  }
  return Math.max(10, Math.round(delayMs / 10) * 10);
}

function handleCardMouseEnter(idx: number): void {
  const pairIdx = findMatchingPairIndex(idx);
  hoveredCardIndexes.value = pairIdx >= 0 && pairIdx !== idx ? [idx, pairIdx] : [idx];
}

function handleCardMouseLeave(): void {
  hoveredCardIndexes.value = [];
}

function isHoveredCard(idx: number): boolean {
  return hoveredCardIndexes.value.includes(idx);
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

function getDelayInputValue(idx: number, delayMs: number): string | number {
  return Object.prototype.hasOwnProperty.call(delayDrafts.value, idx)
    ? delayDrafts.value[idx]
    : delayMs;
}

function clearDelayDraft(idx: number): void {
  if (Object.prototype.hasOwnProperty.call(delayDrafts.value, idx)) {
    delete delayDrafts.value[idx];
  }
}

function resetDelayDrafts(): void {
  delayDrafts.value = {};
}

function startDelayEdit(idx: number): void {
  const card = macroStore.editingCards[idx];
  if (!card) {
    return;
  }
  delayDrafts.value[idx] = String(card.delayMs);
}

function handleDelayDraftInput(idx: number, event: Event): void {
  const target = event.target as HTMLInputElement | null;
  if (!target) {
    return;
  }
  delayDrafts.value[idx] = target.value;
}

function commitDelayInput(idx: number, event: Event): void {
  const target = event.target as HTMLInputElement | null;
  const card = macroStore.editingCards[idx];
  if (!target || !card) {
    clearDelayDraft(idx);
    return;
  }

  const rawValue = target.value.trim();
  if (rawValue === "") {
    clearDelayDraft(idx);
    target.value = String(card.delayMs);
    return;
  }

  const nextDelay = Number(rawValue);
  if (!Number.isFinite(nextDelay) || nextDelay < 0) {
    clearDelayDraft(idx);
    target.value = String(card.delayMs);
    return;
  }

  const normalized = normalizeDelay(nextDelay);
  setDelayMs(idx, normalized);
  clearDelayDraft(idx);
  target.value = String(normalized);
}

function revertDelayInput(idx: number, event: Event): void {
  const target = event.target as HTMLInputElement | null;
  const card = macroStore.editingCards[idx];
  clearDelayDraft(idx);
  if (!target || !card) {
    return;
  }
  target.value = String(card.delayMs);
  target.blur();
}

function handleDelayWheel(event: WheelEvent): void {
  const target = event.target as HTMLInputElement | null;
  target?.blur();
}

function addCard(action: MacroAction, delayMs = 0): void {
  macroStore.editingCards.push({
    action: { ...action },
    delayMs,
  });
}

function addKeyAction(): void {
  showModifierMenu.value = false;
  showMouseMenu.value = false;
  showConsumerMenu.value = false;

  const baseIdx = macroStore.editingCards.length;
  addCard({ type: MacroActionType.KEY_DOWN, param: 0 }, 50);
  addCard({ type: MacroActionType.KEY_UP, param: 0 }, 0);
  scrollToBottom();
  startCaptureForCard(baseIdx);
}

function addModifier(modifier: number): void {
  addCard({ type: MacroActionType.MOD_DOWN, param: modifier }, 50);
  addCard({ type: MacroActionType.MOD_UP, param: modifier }, 0);
  scrollToBottom();
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

function pairOrderKey(action: MacroAction): string | null {
  const pair = getPairTypes(action.type);
  if (!pair) {
    return null;
  }

  return `${pair.press}:${pair.release}:${action.param}`;
}

function hasInvalidPairOrder(cards: { action: MacroAction }[]): boolean {
  const openPairs = new Map<string, number>();

  for (const card of cards) {
    const key = pairOrderKey(card.action);
    if (!key) {
      continue;
    }

    const current = openPairs.get(key) ?? 0;
    if (isPressAction(card.action.type)) {
      openPairs.set(key, current + 1);
      continue;
    }

    if (current <= 0) {
      return true;
    }

    openPairs.set(key, current - 1);
  }

  return false;
}

function cloneCards(cards: MacroCard[]): MacroCard[] {
  return cards.map((card) => ({
    action: { ...card.action },
    delayMs: card.delayMs,
  }));
}

function simulateMovedCards(draggedIndex: number, futureIndex: number): typeof macroStore.editingCards {
  const nextCards = [...macroStore.editingCards];
  const [draggedCard] = nextCards.splice(draggedIndex, 1);
  if (!draggedCard) {
    return nextCards;
  }

  const targetIndex = Math.max(0, Math.min(futureIndex, nextCards.length));
  nextCards.splice(targetIndex, 0, draggedCard);
  return nextCards;
}

function getCardIndexFromElement(element: HTMLElement | null): number {
  const cardElement = element?.closest(".action-card") as HTMLElement | null;
  const rawIndex = cardElement?.dataset.cardIndex;
  const index = rawIndex !== undefined ? Number(rawIndex) : -1;
  return Number.isInteger(index) ? index : -1;
}

function buildNextCardsFromMoveEvent(event: MoveEvent): typeof macroStore.editingCards | null {
  const draggedIndex = getCardIndexFromElement(event.dragged);
  const relatedIndex = getCardIndexFromElement(event.related);

  if (draggedIndex < 0 || relatedIndex < 0) {
    return null;
  }

  let targetIndex = relatedIndex + (event.willInsertAfter ? 1 : 0);
  if (draggedIndex < targetIndex) {
    targetIndex--;
  }

  return simulateMovedCards(draggedIndex, targetIndex);
}

function canMoveCard(event: MoveEvent): boolean {
  const nextCards = buildNextCardsFromMoveEvent(event);
  return nextCards ? !hasInvalidPairOrder(nextCards) : true;
}

function handleDragStart(): void {
  hoveredCardIndexes.value = [];
  resetDelayDrafts();
  dragSnapshot.value = cloneCards(macroStore.editingCards);
}

function handleDragEnd(): void {
  if (hasInvalidPairOrder(macroStore.editingCards) && dragSnapshot.value) {
    macroStore.editingCards = cloneCards(dragSnapshot.value);
  }
  resetDelayDrafts();
  dragSnapshot.value = null;
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
    if (keycode === 0) {
      return;
    }

    const nextKeycode = keycode > 0 ? keycode : card.action.param;
    const pairIdx = findMatchingKeyAction(idx);
    replaceCardAction(idx, { type: card.action.type, param: nextKeycode });

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
    preventDefault() { },
    stopPropagation() { },
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
  showModifierMenu.value = false;
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
  showModifierMenu.value = false;
  showMouseMenu.value = false;
  showConsumerMenu.value = false;
  hoveredCardIndexes.value = [];
  resetDelayDrafts();
  codeCompletions.value = [];
  codeHasFocus.value = false;
  cancelCapture();
  if (isRecording.value) {
    stopRecording();
  }
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
  if (editorMode.value === "code" && !applyCodeToCards()) {
    const firstDiagnostic = codeCompileState.value.diagnostics[0];
    showToast(
      "error",
      "脚本编译失败",
      firstDiagnostic?.message || "请先修复 MeowMacro 代码中的错误",
    );
    return;
  }

  const visualState = compileMacroCards(macroStore.editingCards);
  if (visualState.diagnostics.length > 0) {
    showToast(
      "error",
      "宏验证失败",
      visualState.diagnostics[0]?.message || "宏动作存在非法顺序",
    );
    return;
  }

  await macroStore.saveMacro();
  // Update cache after save
  snapshotCurrentTab();
}

async function handleDelete(): Promise<void> {
  cleanupInteractiveState();
  const slot = activeSlot.value;
  if (slot < 0) return;
  await macroStore.deleteMacro(slot);
  closeTab(slot);
}

function closeEditor(): void {
  cleanupInteractiveState();
  dialogVisible.value = false;
}

watch(
  () => props.visible,
  (visible) => {
    if (visible) {
      // Reset IDE state
      openTabs.value = [];
      activeSlot.value = -1;
      tabCache.value = new Map();
      // Open the initially requested slot
      void openTab(props.slot);
      return;
    }

    if (!visible) {
      cleanupInteractiveState();
      openTabs.value = [];
      activeSlot.value = -1;
      tabCache.value = new Map();
    }
  },
);

watch(codeText, () => {
  requestAnimationFrame(syncCodeEditorDecorations);
  if (codeHasFocus.value) {
    requestAnimationFrame(() => {
      void refreshCodeCompletions();
    });
  }
});

onBeforeUnmount(() => {
  cleanupInteractiveState();
});
</script>

<style scoped>
/* ====== IDE Layout ====== */
.ide-layout {
  display: grid;
  grid-template-columns: 180px minmax(0, 1fr);
  height: min(82vh, 720px);
  min-height: 400px;
  overflow: hidden;
}

.ide-sidebar {
  display: flex;
  flex-direction: column;
  border-right: 1px solid var(--c-border);
  background: color-mix(in srgb, var(--c-bg-secondary) 70%, var(--c-bg-primary));
  overflow: hidden;
}

.sidebar-header {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  padding: 0.6rem 0.75rem;
  border-bottom: 1px solid var(--c-border);
  color: var(--c-text-muted);
  font-size: 0.72rem;
  font-weight: 800;
  text-transform: uppercase;
  letter-spacing: 0.08em;
}

.sidebar-header i {
  font-size: 0.72rem;
}

.sidebar-close {
  margin-left: auto;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 1.4rem;
  height: 1.4rem;
  border: 0;
  border-radius: 4px;
  background: transparent;
  color: var(--c-text-muted);
  cursor: pointer;
  transition: all 0.12s ease;
}

.sidebar-close:hover {
  background: var(--c-bg-hover);
  color: var(--c-text-primary);
}

.sidebar-files {
  flex: 1 1 auto;
  overflow-y: auto;
  padding: 0.35rem;
}

.sidebar-file {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  width: 100%;
  padding: 0.42rem 0.6rem;
  border: 0;
  border-radius: 5px;
  background: transparent;
  color: var(--c-text-secondary);
  font-size: 0.8rem;
  font-weight: 600;
  cursor: pointer;
  text-align: left;
  transition: all 0.12s ease;
}

.sidebar-file:hover {
  background: var(--c-bg-hover);
  color: var(--c-text-primary);
}

.sidebar-file.active {
  background: var(--c-accent-soft);
  color: var(--c-accent);
}

.sidebar-file.has-data {
  color: var(--c-text-primary);
}

.sidebar-file.has-data.active {
  color: var(--c-accent);
}

.sidebar-file i {
  font-size: 0.82rem;
  flex-shrink: 0;
}

.sidebar-file-name {
  flex: 1;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.sidebar-file-idx {
  flex-shrink: 0;
  color: var(--c-text-muted);
  font-size: 0.66rem;
  font-weight: 700;
  opacity: 0.6;
}

.ide-main {
  display: flex;
  flex-direction: column;
  min-height: 0;
  min-width: 0;
}

.ide-tabbar {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  border-bottom: 1px solid var(--c-border);
  background: var(--c-bg-secondary);
  padding-right: 0.5rem;
  flex-shrink: 0;
}

.ide-tabs {
  display: flex;
  flex: 1;
  min-width: 0;
  overflow-x: auto;
  scrollbar-width: none;
}

.ide-tabs::-webkit-scrollbar {
  display: none;
}

.ide-tab {
  display: inline-flex;
  align-items: center;
  gap: 0.45rem;
  padding: 0.5rem 0.55rem 0.5rem 0.75rem;
  border: 0;
  border-bottom: 2px solid transparent;
  background: transparent;
  color: var(--c-text-secondary);
  font-size: 0.78rem;
  font-weight: 700;
  cursor: pointer;
  white-space: nowrap;
  transition: all 0.12s ease;
}

.ide-tab:hover {
  background: var(--c-bg-hover);
  color: var(--c-text-primary);
}

.ide-tab.active {
  color: var(--c-text-primary);
  border-bottom-color: var(--c-accent);
  background: var(--c-bg-primary);
}

.ide-tab-label {
  max-width: 120px;
  overflow: hidden;
  text-overflow: ellipsis;
}

.ide-tab-close {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 1.2rem;
  height: 1.2rem;
  border-radius: 3px;
  opacity: 0;
  transition: opacity 0.12s, background 0.12s;
}

.ide-tab:hover .ide-tab-close,
.ide-tab.active .ide-tab-close {
  opacity: 1;
}

.ide-tab-close:hover {
  background: var(--c-bg-tertiary);
}

.ide-tab-close i {
  font-size: 0.58rem;
}

.ide-tabbar-actions {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  flex-shrink: 0;
}

.ide-close-btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 1.8rem;
  height: 1.8rem;
  border: 0;
  border-radius: 5px;
  background: transparent;
  color: var(--c-text-muted);
  font-size: 0.82rem;
  cursor: pointer;
  flex-shrink: 0;
  margin-left: 0.25rem;
  transition: all 0.12s ease;
}

.ide-close-btn:hover {
  background: color-mix(in srgb, var(--c-danger) 14%, transparent);
  color: var(--c-danger);
}

.ide-name-wrap {
  display: flex;
  align-items: center;
  gap: 0.35rem;
  padding: 0.2rem;
  border: 1px solid var(--c-border);
  border-radius: 5px;
  background: var(--c-bg-tertiary);
}

.ide-name-input {
  width: 140px;
  padding: 0.3rem 0.5rem !important;
  border: 0 !important;
  background: transparent !important;
  font-size: 0.78rem !important;
  font-weight: 700 !important;
}

.ide-name-input:focus {
  box-shadow: none !important;
}

.ide-name-bytes {
  color: var(--c-text-muted);
  font-size: 0.62rem;
  font-weight: 700;
  white-space: nowrap;
  padding-right: 0.35rem;
}

.ide-welcome {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  flex: 1;
  gap: 0.5rem;
  color: var(--c-text-muted);
}

.ide-welcome .watermark-letter {
  font-size: 8rem;
}

.ide-welcome .watermark-hint {
  font-size: 0.88rem;
  font-weight: 600;
  opacity: 0.6;
}

.ide-statusbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 0.5rem;
  padding: 0.35rem 0.6rem;
  border-top: 1px solid var(--c-border);
  background: var(--c-bg-secondary);
  flex-shrink: 0;
}

.statusbar-left {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  min-width: 0;
  overflow: hidden;
}

.statusbar-right {
  display: flex;
  align-items: center;
  gap: 0.35rem;
  flex-shrink: 0;
}

/* ====== Mode switch (reused) ====== */
.editor-mode-switch {
  display: inline-flex;
  gap: 0.2rem;
  padding: 0.18rem;
  border: 1px solid var(--c-border);
  border-radius: 5px;
  background: var(--c-bg-tertiary);
  width: fit-content;
}

.mode-btn {
  display: inline-flex;
  align-items: center;
  gap: 0.3rem;
  padding: 0.3rem 0.55rem;
  border: 0;
  border-radius: 4px;
  background: transparent;
  color: var(--c-text-secondary);
  font-size: 0.74rem;
  font-weight: 700;
  cursor: pointer;
  transition: background 0.12s ease, color 0.12s ease;
}

.mode-btn:hover {
  background: var(--c-bg-tertiary);
  color: var(--c-text-primary);
}

.mode-btn.active {
  background: var(--c-accent-soft);
  color: var(--c-accent);
}

.meowmacro-doc-link {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 1.6rem;
  height: 1.6rem;
  border-radius: 4px;
  color: var(--c-text-muted);
  font-size: 0.88rem;
  transition: color 0.12s ease, background 0.12s ease;
  text-decoration: none;
}

.meowmacro-doc-link:hover {
  color: var(--c-accent);
  background: var(--c-accent-soft);
}

.code-editor-panel {
  --macro-code-font-size: 0.92rem;
  --macro-code-line-height: 1.65;
  --macro-code-padding-top: 0.6rem;
  --macro-code-padding-x: 0.75rem;
  --macro-code-padding-bottom: 0.6rem;
  --macro-code-row-height: calc(var(--macro-code-font-size) * var(--macro-code-line-height));
  display: flex;
  flex-direction: column;
  flex: 1 1 auto;
  min-height: 0;
  gap: 0;
  background: var(--c-bg-primary);
  overflow: hidden;
}

.code-editor-shell {
  display: grid;
  grid-template-columns: auto minmax(0, 1fr);
  flex: 1 1 auto;
  min-height: 0;
  background: var(--c-bg-primary);
  overflow: hidden;
}

.code-editor-gutter {
  position: relative;
  width: 3.25rem;
  border-right: 1px solid color-mix(in srgb, var(--c-border) 88%, transparent);
  background: color-mix(in srgb, var(--c-bg-tertiary) 82%, var(--c-bg-primary));
  overflow: hidden;
  user-select: none;
}

.code-editor-gutter-track {
  display: flex;
  flex-direction: column;
  padding: var(--macro-code-padding-top) 0.55rem var(--macro-code-padding-bottom);
  will-change: transform;
}

.code-editor-line-number {
  height: var(--macro-code-row-height);
  color: var(--c-text-muted);
  text-align: right;
  font: 700 var(--macro-code-font-size)/var(--macro-code-line-height) "JetBrains Mono", "Cascadia Code", monospace;
  font-variant-numeric: tabular-nums;
}

.code-editor-stage {
  position: relative;
  min-width: 0;
  min-height: 0;
  overflow: hidden;
  background: var(--c-bg-primary);
}

.code-editor-measure {
  position: absolute;
  top: 0;
  left: 0;
  min-width: 100%;
  padding: var(--macro-code-padding-top) var(--macro-code-padding-x) var(--macro-code-padding-bottom);
  visibility: hidden;
  pointer-events: none;
  white-space: pre;
  font: 700 var(--macro-code-font-size)/var(--macro-code-line-height) "JetBrains Mono", "Cascadia Code", monospace;
}

.code-editor-caret-marker {
  display: inline-block;
  width: 1px;
  height: var(--macro-code-row-height);
}

.code-editor-highlight {
  position: absolute;
  top: 0;
  left: 0;
  min-width: 100%;
  margin: 0;
  padding: var(--macro-code-padding-top) var(--macro-code-padding-x) var(--macro-code-padding-bottom);
  color: var(--c-text-primary);
  font: 700 var(--macro-code-font-size)/var(--macro-code-line-height) "JetBrains Mono", "Cascadia Code", monospace;
  white-space: pre;
  pointer-events: none;
  will-change: transform;
}

.code-editor-placeholder {
  position: absolute;
  inset: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  pointer-events: none;
}

.watermark-letter {
  font-size: 8rem;
  font-weight: 900;
  font-family: "JetBrains Mono", "Cascadia Code", monospace;
  color: color-mix(in srgb, var(--c-text-muted) 12%, transparent);
  filter: blur(1.5px);
  user-select: none;
  line-height: 1;
}

.code-editor-input {
  position: relative;
  z-index: 1;
  width: 100%;
  height: 100%;
  min-height: 100px;
  resize: none;
  overflow: auto;
  border: 0;
  background: transparent;
  color: transparent;
  -webkit-text-fill-color: transparent;
  caret-color: var(--c-text-primary);
  padding: var(--macro-code-padding-top) var(--macro-code-padding-x) var(--macro-code-padding-bottom);
  font: 700 var(--macro-code-font-size)/var(--macro-code-line-height) "JetBrains Mono", "Cascadia Code", monospace;
  outline: none;
  white-space: pre;
  tab-size: 2;
}

.code-editor-input:focus {
  box-shadow: inset 0 0 0 1px var(--c-accent);
}

.code-toolbar-btn {
  display: inline-flex;
  align-items: center;
  gap: 0.4rem;
  padding: 0.32rem 0.72rem;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-md);
  background: var(--c-bg-primary);
  color: var(--c-text-secondary);
  font-size: 0.78rem;
  font-weight: 700;
  cursor: pointer;
  transition: all 0.15s ease;
}

.code-toolbar-btn:hover:not(:disabled) {
  background: var(--c-bg-hover);
  color: var(--c-accent);
  border-color: var(--c-accent);
}

.code-toolbar-btn:disabled {
  opacity: 0.45;
  cursor: not-allowed;
}

.code-toolbar-btn i {
  font-size: 0.72rem;
}

.code-editor-line-number.has-error {
  color: var(--c-danger);
}

.code-editor-highlight :deep(.token-keyword) {
  color: #f38ad5;
}

.code-editor-highlight :deep(.token-symbol) {
  color: #7fb3ff;
}

.code-editor-highlight :deep(.token-number) {
  color: #7fd8b4;
}

.code-editor-highlight :deep(.token-operator) {
  color: var(--c-text-muted);
}

.code-editor-highlight :deep(.token-comment) {
  color: #7e7590;
}

.code-editor-highlight :deep(.token-error-lens) {
  margin-left: 2em;
  padding: 0.1rem 0.5rem;
  background: color-mix(in srgb, var(--c-danger) 12%, transparent);
  color: var(--c-danger);
  font-size: 0.78em;
  font-weight: 600;
  border-radius: 3px;
  pointer-events: none;
}

.code-completion {
  position: absolute;
  z-index: 6;
  display: flex;
  flex-direction: column;
  min-width: 240px;
  max-width: min(380px, calc(100% - 20px));
  max-height: 200px;
  padding: 3px;
  border: 1px solid color-mix(in srgb, var(--c-accent) 12%, var(--c-border));
  border-radius: 6px;
  background: var(--c-bg-secondary);
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.28);
  overflow-y: auto;
  overscroll-behavior: contain;
}

.code-completion-item {
  display: flex;
  align-items: center;
  gap: 0.55rem;
  width: 100%;
  padding: 0.3rem 0.5rem;
  border: 0;
  border-radius: 4px;
  background: transparent;
  color: var(--c-text-primary);
  cursor: pointer;
  text-align: left;
  line-height: 1.45;
}

.code-completion-item:hover,
.code-completion-item.active {
  background: var(--c-accent);
  color: #fff;
}

.code-completion-item.active .code-completion-detail,
.code-completion-item:hover .code-completion-detail {
  color: rgba(255, 255, 255, 0.72);
}

.code-completion-item.active .code-completion-kind,
.code-completion-item:hover .code-completion-kind {
  background: rgba(255, 255, 255, 0.18);
  color: #fff;
}

.code-completion-main {
  display: flex;
  align-items: baseline;
  min-width: 0;
  gap: 0.6rem;
  flex: 1;
  overflow: hidden;
}

.code-completion-label {
  color: inherit;
  font-size: 0.82rem;
  font-weight: 700;
  white-space: nowrap;
}

.code-completion-detail {
  color: var(--c-text-muted);
  font-size: 0.72rem;
  font-weight: 500;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.code-completion-kind {
  flex: 0 0 auto;
  padding: 0.1rem 0.35rem;
  border-radius: 3px;
  background: var(--c-accent-soft);
  color: var(--c-accent);
  font-size: 0.62rem;
  font-weight: 700;
  letter-spacing: 0.02em;
}

.diagnostic-panel {
  display: flex;
  flex-direction: column;
  gap: 0.35rem;
  padding: 0.5rem 0.65rem;
  border-top: 1px solid color-mix(in srgb, var(--c-danger) 30%, var(--c-border));
  background: color-mix(in srgb, var(--c-danger) 6%, var(--c-bg-secondary));
  flex: 0 0 auto;
  max-height: 6rem;
  overflow-y: auto;
}

.diagnostic-item {
  display: flex;
  gap: 0.5rem;
  align-items: flex-start;
  color: var(--c-text-primary);
  font-size: 0.76rem;
  line-height: 1.45;
}

.diagnostic-location {
  flex: 0 0 auto;
  color: var(--c-danger);
  font-weight: 800;
}

.diagnostic-message {
  color: var(--c-text-primary);
}

.code-toolbar-spacer {
  flex: 1;
}

.editor-body {
  display: flex;
  flex-direction: column;
  flex: 1 1 auto;
  gap: 0;
  min-height: 0;
  overflow-y: auto;
  padding: 0.5rem;
}

.editor-body.is-code-mode {
  overflow: hidden;
  padding: 0;
}

.editor-body::-webkit-scrollbar {
  width: 8px;
}

.editor-body::-webkit-scrollbar-thumb {
  background: var(--c-border);
  border-radius: 999px;
}

.pipeline-container {
  display: flex;
  flex-direction: column;
  flex: 1 1 auto;
  min-height: 0;
  overflow: visible;
  padding: 0.2rem 0.28rem 0.2rem 0;
}

.pipeline-empty {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 0.5rem;
  padding: 2rem 1rem;
  flex: 1;
  color: var(--c-text-muted);
  text-align: center;
}

.pipeline-empty .watermark-letter {
  font-size: 6rem;
}

.pipeline-empty .watermark-hint {
  font-size: 0.82rem;
  font-weight: 600;
  opacity: 0.6;
}

.pipeline-list {
  display: flex;
  flex-direction: column;
  gap: 0.3rem;
  padding: 0.05rem 0.1rem 0.2rem 0;
}

.action-card {
  position: relative;
}

.card-body {
  display: grid;
  grid-template-columns: 1.5rem minmax(0, 1fr) minmax(130px, 150px) 1.5rem;
  align-items: center;
  gap: 0.45rem;
  padding: 0.4rem 0.6rem 0.4rem 0.5rem;
  border: 1px solid var(--c-border);
  border-radius: 6px;
  background: var(--c-bg-tertiary);
  transition: border-color var(--transition-fast), background var(--transition-fast);
}

.action-card:hover .card-body,
.action-card.is-hover-linked .card-body {
  border-color: var(--c-accent);
  background: var(--c-bg-hover);
}

.cat-keyboard .card-category {
  color: #2f7deb;
  background: rgba(79, 157, 255, 0.1);
  border-color: rgba(79, 157, 255, 0.16);
}

.cat-mouse .card-category {
  color: #149e73;
  background: rgba(31, 184, 138, 0.1);
  border-color: rgba(31, 184, 138, 0.16);
}

.cat-consumer .card-category {
  color: #c48208;
  background: rgba(230, 160, 26, 0.1);
  border-color: rgba(230, 160, 26, 0.16);
}

.card-drag {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 1.5rem;
  height: 1.5rem;
  border-radius: 3px;
  border: 0;
  background: transparent;
  color: var(--c-text-muted);
  cursor: grab;
  transition: all var(--transition-fast);
  flex-shrink: 0;
  opacity: 0.5;
}

.card-drag:hover {
  opacity: 1;
  color: var(--c-accent);
}

.card-drag:active {
  cursor: grabbing;
}

.drag-dots {
  display: grid;
  grid-template-columns: repeat(2, 2.5px);
  gap: 2.5px;
}

.drag-dots span {
  width: 2.5px;
  height: 2.5px;
  border-radius: 999px;
  background: currentColor;
}

.card-content {
  min-width: 0;
  display: inline-flex;
  align-items: center;
  gap: 0.4rem;
}

.card-category {
  display: inline-flex;
  align-items: center;
  gap: 0.3rem;
  padding: 0.12rem 0.42rem;
  border: 1px solid transparent;
  border-radius: 999px;
  font-size: 0.64rem;
  font-weight: 800;
  white-space: nowrap;
  letter-spacing: 0.02em;
  align-self: center;
}

.card-category i {
  font-size: 0.64rem;
}

.card-desc {
  display: inline-flex;
  align-items: center;
  gap: 0.3rem;
  min-width: 0;
  max-width: 100%;
  color: var(--c-text-primary);
  font-size: 0.86rem;
  line-height: 1.1;
  font-weight: 700;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.card-desc-btn {
  display: inline-flex;
  align-items: center;
  min-width: 0;
  padding: 0;
  border: 0;
  background: transparent;
  cursor: pointer;
  transition: color 0.18s ease;
  text-align: left;
  max-width: 100%;
}

.card-desc-btn:hover {
  color: var(--c-accent);
}

.card-desc-btn.capturing {
  color: var(--c-danger);
  font-weight: 800;
  animation: pulse-text 1.1s ease-in-out infinite;
}

@keyframes pulse-text {

  0%,
  100% {
    opacity: 1;
  }

  50% {
    opacity: 0.58;
  }
}

.card-edit-hint {
  color: var(--c-text-muted);
  font-size: 0.74rem;
  opacity: 0;
  transition: opacity 0.18s ease;
}

.card-desc-btn:hover .card-edit-hint,
.action-card.is-hover-linked .card-edit-hint {
  opacity: 0.76;
}

.card-delay {
  display: flex;
  align-items: center;
  justify-content: flex-end;
  justify-self: end;
  width: 100%;
  gap: 0.3rem;
  margin-left: auto;
}

.delay-label {
  color: var(--c-text-muted);
  font-size: 0.62rem;
  white-space: nowrap;
  font-weight: 800;
  letter-spacing: 0.04em;
}

.delay-input-wrap {
  display: inline-flex;
  align-items: center;
  gap: 0.3rem;
  padding: 0.22rem 0.38rem;
  border: 1px solid var(--c-border);
  border-radius: 4px;
  background: var(--c-bg-secondary);
  transition: all var(--transition-fast);
}

.delay-input-wrap:focus-within {
  border-color: var(--c-accent);
  box-shadow: 0 0 0 2px var(--c-accent-soft);
  background: var(--c-bg-secondary);
}

.delay-input {
  width: 52px;
  padding: 0;
  border: 0;
  outline: 0;
  background: transparent;
  color: var(--c-text-primary);
  text-align: right;
  font-size: 0.74rem;
  font-weight: 800;
  font-family: "JetBrains Mono", "Fira Code", monospace;
}

.delay-input::-webkit-outer-spin-button,
.delay-input::-webkit-inner-spin-button {
  margin: 0;
}

.delay-unit {
  color: var(--c-text-muted);
  font-size: 0.62rem;
  font-weight: 800;
  white-space: nowrap;
}

.card-remove {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 1.5rem;
  height: 1.5rem;
  border: 0;
  border-radius: 3px;
  background: transparent;
  color: var(--c-text-muted);
  font-size: 0.78rem;
  cursor: pointer;
  transition: all var(--transition-fast);
  flex-shrink: 0;
}

.card-remove:hover {
  background: color-mix(in srgb, var(--c-danger) 14%, transparent);
  color: var(--c-danger);
}

.pipeline-stats {
  display: inline-flex;
  align-items: center;
  gap: 0.3rem;
  flex: 0 0 auto;
  color: var(--c-text-muted);
  font-size: 0.7rem;
  font-weight: 700;
  white-space: nowrap;
}

.pipeline-stats.over {
  color: var(--c-danger);
  border-color: var(--c-danger);
  background: color-mix(in srgb, var(--c-danger) 10%, var(--c-bg-tertiary));
}

.sub-menu {
  padding: 0.85rem;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-md);
  background: var(--c-bg-tertiary);
  animation: slideDown 0.18s ease;
}

@keyframes slideDown {
  from {
    opacity: 0;
    transform: translateY(-8px);
  }

  to {
    opacity: 1;
    transform: translateY(0);
  }
}

.sub-menu-title {
  margin-bottom: 0.65rem;
  color: var(--c-accent);
  font-size: 0.78rem;
  font-weight: 900;
  text-transform: uppercase;
  letter-spacing: 0.08em;
}

.sub-menu-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(96px, 1fr));
  gap: 0.55rem;
}

.sub-menu-grid.cols-3 {
  grid-template-columns: repeat(3, minmax(0, 1fr));
}

.sub-menu-item {
  padding: 0.66rem 0.72rem;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  background: var(--c-bg-secondary);
  color: var(--c-text-primary);
  font-size: 0.82rem;
  font-weight: 700;
  cursor: pointer;
  transition: all var(--transition-fast);
  text-align: center;
}

.sub-menu-item:hover {
  border-color: var(--c-accent);
  background: var(--c-bg-hover);
  color: var(--c-accent);
}

/* (footer-toolbar unused in IDE layout) */

.add-bar {
  display: flex;
  align-items: center;
  flex-wrap: nowrap;
  gap: 0.25rem;
  overflow-x: auto;
  scrollbar-width: none;
  width: fit-content;
  max-width: 100%;
}

.add-bar::-webkit-scrollbar {
  display: none;
}

.add-btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 0.3rem;
  min-width: auto;
  padding: 0.28rem 0.5rem;
  border: 1px solid var(--c-border);
  border-radius: 4px;
  background: var(--c-bg-primary);
  color: var(--c-text-secondary);
  font-size: 0.7rem;
  font-weight: 700;
  white-space: nowrap;
  cursor: pointer;
  transition: all 0.12s ease;
}

.add-btn:hover {
  border-color: var(--c-accent);
  background: var(--c-bg-hover);
  color: var(--c-accent);
}

.add-btn i {
  font-size: 0.72rem;
}

.add-btn.recording {
  border-color: var(--c-danger);
  color: white;
  background: var(--c-danger);
  animation: recordPulse 1.2s ease-in-out infinite;
}

.add-btn.recording:hover {
  border-color: var(--c-danger);
  box-shadow: 0 0 0 3px color-mix(in srgb, var(--c-danger) 18%, transparent);
}

@keyframes recordPulse {

  0%,
  100% {
    box-shadow: 0 0 0 0 rgba(239, 68, 68, 0.38);
  }

  50% {
    box-shadow: 0 0 0 5px rgba(239, 68, 68, 0);
  }
}

/* (footer-actions unused in IDE layout) */

.card-ghost {
  opacity: 0.55;
}

.card-chosen .card-body {
  border-color: var(--c-accent);
  background: var(--c-bg-hover);
}

@media (max-width: 768px) {
  .ide-layout {
    grid-template-columns: 140px minmax(0, 1fr);
  }

  .ide-tabbar-actions {
    display: none;
  }

  .add-btn span {
    display: none;
  }
}

@media (max-width: 560px) {
  .ide-layout {
    grid-template-columns: 1fr;
  }

  .ide-sidebar {
    display: none;
  }

  .card-body {
    grid-template-columns: 2rem minmax(0, 1fr) 2rem;
    padding: 0.72rem;
  }

  .card-delay {
    grid-column: 2 / 3;
    flex-wrap: wrap;
    justify-self: start;
  }
}

:deep(.p-dialog) {
  max-height: 92vh !important;
  display: flex !important;
  flex-direction: column !important;
  overflow: hidden !important;
  border-radius: 0.75rem !important;
}

:deep(.p-dialog-header) {
  display: none !important;
}

:deep(.p-dialog-content) {
  padding: 0 !important;
  overflow: hidden !important;
  display: flex !important;
  flex-direction: column !important;
  min-height: 0 !important;
  gap: 0 !important;
  flex: 1 !important;
}

:deep(.p-dialog-footer) {
  display: none !important;
}

:deep(.p-dialog-content > div) {
  display: flex !important;
  min-height: 0 !important;
  flex: 1 !important;
}
</style>
