import {
  MACRO_MAX_ACTIONS,
  MACRO_MAX_DATA_SIZE,
  MacroActionType,
  Modifier,
  type MacroAction,
} from "@/types/protocol";
import { CONSUMER_KEYS, isMacroConsumerCodeSupported } from "@/utils/consumer";
import { KEYCODE_NAMES } from "@/utils/keycodes";

const DELAY_ACTION_MAX_MS = 2550;

export interface MacroSourceRef {
  mode: "visual" | "code";
  cardIndex?: number;
  line?: number;
  column?: number;
}

export interface MacroDiagnostic {
  code: string;
  message: string;
  source?: MacroSourceRef;
}

/** A single combo group within a tap: modifier mask + keycodes, or a mouse button */
export interface TapGroup {
  modifier: number;
  keycodes: number[];
  repeatCount: number;
  /** When set, this group is a mouse button click (no modifier/keycodes) */
  mouseButton?: number;
}

export type MacroStep =
  | {
      kind: "tap";
      /** Space-separated groups; length=1 means simultaneous combo, >1 means sequential */
      groups: TapGroup[];
      holdMs: number;
      waitMs: number;
      source?: MacroSourceRef;
    }
  | {
      kind: "groupDown" | "groupUp";
      modifier: number;
      keycodes: number[];
      source?: MacroSourceRef;
    }
  | {
      kind: "down" | "up";
      target: "key" | "modifier" | "mouse";
      param: number;
      source?: MacroSourceRef;
    }
  | {
      kind: "delay";
      ms: number;
      source?: MacroSourceRef;
    }
  | {
      kind: "wheel";
      direction: 1 | 2;
      source?: MacroSourceRef;
    }
  | {
      kind: "consumer";
      code: number;
      source?: MacroSourceRef;
    };

export interface MacroCardLike {
  action: MacroAction;
  delayMs: number;
}

export interface MacroCompileState {
  steps: MacroStep[];
  actions: MacroAction[];
  diagnostics: MacroDiagnostic[];
  actionCount: number;
  dataSize: number;
}

export interface MacroDslCompletionItem {
  label: string;
  insertText: string;
  detail: string;
  kind: "keyword" | "snippet" | "symbol" | "value";
  replaceFrom: number;
  replaceTo: number;
}

interface MacroInstruction {
  type: MacroActionType;
  param: number;
  source?: MacroSourceRef;
}

/** Parse a duration string like "50ms", "50", "1s", "0.5s" → integer ms. Returns -1 on failure. */
function parseDurationMs(text: string): number {
  const trimmed = text.trim();
  const secMatch = trimmed.match(/^(\d+(?:\.\d+)?)\s*s$/i);
  if (secMatch) return Math.round(Number(secMatch[1]) * 1000);
  const msMatch = trimmed.match(/^(\d+)(?:\s*ms)?$/i);
  if (msMatch) return Number(msMatch[1]);
  return -1;
}

/** Check if a duration is valid for firmware encoding (must be a multiple of 10ms). Returns error string or null. */
function validateDelayGranularity(ms: number, label: string): string | null {
  if (ms > 0 && ms % 10 !== 0) {
    return `${label} 必须是 10ms 的倍数（当前 ${ms}ms），例如 10ms、20ms、50ms`;
  }
  return null;
}

const MODIFIER_ORDER = [
  Modifier.LCTRL,
  Modifier.LSHIFT,
  Modifier.LALT,
  Modifier.LGUI,
  Modifier.RCTRL,
  Modifier.RSHIFT,
  Modifier.RALT,
  Modifier.RGUI,
];

const MODIFIER_TOKEN_MAP: Record<number, string> = {
  [Modifier.LCTRL]: "Ctrl",
  [Modifier.LSHIFT]: "Shift",
  [Modifier.LALT]: "Alt",
  [Modifier.LGUI]: "Win",
  [Modifier.RCTRL]: "RCtrl",
  [Modifier.RSHIFT]: "RShift",
  [Modifier.RALT]: "RAlt",
  [Modifier.RGUI]: "RWin",
};

const MODIFIER_ALIASES: Record<string, number> = {
  CTRL: Modifier.LCTRL,
  CONTROL: Modifier.LCTRL,
  LCTRL: Modifier.LCTRL,
  SHIFT: Modifier.LSHIFT,
  LSHIFT: Modifier.LSHIFT,
  ALT: Modifier.LALT,
  LALT: Modifier.LALT,
  WIN: Modifier.LGUI,
  GUI: Modifier.LGUI,
  META: Modifier.LGUI,
  CMD: Modifier.LGUI,
  SUPER: Modifier.LGUI,
  LWIN: Modifier.LGUI,
  LGUI: Modifier.LGUI,
  RCTRL: Modifier.RCTRL,
  RCONTROL: Modifier.RCTRL,
  RSHIFT: Modifier.RSHIFT,
  RALT: Modifier.RALT,
  RWIN: Modifier.RGUI,
  RGUI: Modifier.RGUI,
  RMETA: Modifier.RGUI,
};

const MOUSE_ALIASES: Record<string, number> = {
  LEFT: 0x01,
  RIGHT: 0x02,
  MIDDLE: 0x04,
  BACK: 0x08,
  FORWARD: 0x10,
};

const MOUSE_TOKEN_MAP: Record<number, string> = {
  0x01: "left",
  0x02: "right",
  0x04: "middle",
  0x08: "back",
  0x10: "forward",
};

const WHEEL_TOKENS: Record<string, 1 | 2> = {
  UP: 1,
  DOWN: 2,
};

const CONSUMER_ALIASES: Record<string, number> = {
  PLAY_PAUSE: 0xcd,
  NEXT_TRACK: 0xb5,
  PREV_TRACK: 0xb6,
  PREVIOUS_TRACK: 0xb6,
  STOP: 0xb7,
  FAST_FORWARD: 0xb3,
  REWIND: 0xb4,
  MUTE: 0xe2,
  VOLUME_UP: 0xe9,
  VOLUME_DOWN: 0xea,
  CALCULATOR: 0x192,
  EXPLORER: 0x194,
  FILE_EXPLORER: 0x194,
  MAIL: 0x18a,
  BROWSER_HOME: 0x223,
  BROWSER_BACK: 0x224,
  BROWSER_FORWARD: 0x225,
  BROWSER_REFRESH: 0x227,
  BROWSER_FAVORITES: 0x22a,
  POWER: 0x30,
  SLEEP: 0x32,
  BRIGHTNESS_UP: 0x6f,
  BRIGHTNESS_DOWN: 0x70,
};

const KEY_ALIASES: Record<string, number> = buildKeyAliases();
const CONSUMER_TOKEN_MAP: Record<number, string> = buildConsumerTokenMap();
const DSL_KEY_COMPLETION_ITEMS = buildDslKeyCompletionItems();
const COMMAND_KEYWORDS = [
  "tap",
  "down",
  "up",
  "delay",
  "wheel",
  "consumer",
] as const;
const SNIPPET_SUGGESTIONS = [
  {
    label: "tap Ctrl+C",
    insertText: "tap Ctrl+C",
    detail: "组合键点击",
    kind: "snippet" as const,
  },
  {
    label: "tap A hold 50ms wait 100ms",
    insertText: "tap A hold 50ms wait 100ms",
    detail: "按住+释放后延时",
    kind: "snippet" as const,
  },
  {
    label: "tap A * 2 wait 50ms",
    insertText: "tap A * 2 wait 50ms",
    detail: "双击",
    kind: "snippet" as const,
  },
  {
    label: "delay 50ms",
    insertText: "delay 50ms",
    detail: "插入延时",
    kind: "snippet" as const,
  },
  {
    label: "tap mouse:left",
    insertText: "tap mouse:left",
    detail: "鼠标点击",
    kind: "snippet" as const,
  },
  {
    label: "wheel up",
    insertText: "wheel up",
    detail: "滚轮向上",
    kind: "snippet" as const,
  },
  {
    label: "consumer volume_up",
    insertText: "consumer volume_up",
    detail: "媒体键",
    kind: "snippet" as const,
  },
];

function buildKeyAliases(): Record<string, number> {
  const aliases: Record<string, number> = {};
  for (const [code, name] of Object.entries(KEYCODE_NAMES)) {
    const keycode = Number(code);
    if (!keycode || !name) {
      continue;
    }
    const token = normalizeToken(name);
    if (!(token in aliases)) {
      aliases[token] = keycode;
    }
  }

  Object.assign(aliases, {
    ESC: 0x29,
    ESCAPE: 0x29,
    ENTER: 0x28,
    RETURN: 0x28,
    SPACE: 0x2c,
    SPACEBAR: 0x2c,
    TAB: 0x2b,
    BACKSPACE: 0x2a,
    DELETE: 0x4c,
    DEL: 0x4c,
    INSERT: 0x49,
    HOME: 0x4a,
    END: 0x4d,
    PAGEUP: 0x4b,
    PAGEDOWN: 0x4e,
    LEFT: 0x50,
    RIGHT: 0x4f,
    UP: 0x52,
    DOWN: 0x51,
    PRTSC: 0x46,
    PRINTSCREEN: 0x46,
    APPS: 0x65,
    MENU: 0x65,
    MINUS: 0x2d,
    HYPHEN: 0x2d,
    DASH: 0x2d,
    EQUAL: 0x2e,
    EQUALS: 0x2e,
    NUMSLASH: 0x54,
    NUMDIVIDE: 0x54,
    NUMPADDIVIDE: 0x54,
    NUMSTAR: 0x55,
    NUMASTERISK: 0x55,
    NUMMULTIPLY: 0x55,
    NUMPADMULTIPLY: 0x55,
    NUMPLUS: 0x57,
    NUMADD: 0x57,
    NUMPADPLUS: 0x57,
    NUMMINUS: 0x56,
    NUMSUBTRACT: 0x56,
    NUMPADMINUS: 0x56,
    NUMPADSUBTRACT: 0x56,
    NUMENTER: 0x58,
    NUMPADENTER: 0x58,
    NUMDOT: 0x63,
    NUMDECIMAL: 0x63,
    NUMPADDOT: 0x63,
    NUMPADDECIMAL: 0x63,
  });

  return aliases;
}

function buildDslKeyCompletionItems(): Array<{
  label: string;
  detail: string;
  kind: "value";
}> {
  const items: Array<{ label: string; detail: string; kind: "value" }> = [];
  const seen = new Set<string>();

  const push = (label: string, detail = "普通按键") => {
    if (!label || seen.has(label)) {
      return;
    }
    seen.add(label);
    items.push({ label, detail, kind: "value" });
  };

  for (const name of Object.values(KEYCODE_NAMES)) {
    if (!name || name === "+") {
      continue;
    }
    push(formatKeycode(parseKeyToken(name) ?? 0));
  }

  push("NumPlus", "小键盘加号键");
  push("NumMinus", "小键盘减号键");
  push("NumEnter", "小键盘回车键");
  push("NumSlash", "小键盘除号键");
  push("NumStar", "小键盘乘号键");
  push("NumDot", "小键盘小数点键");

  return items;
}

function buildConsumerTokenMap(): Record<number, string> {
  const tokens: Record<number, string> = {};
  for (const [token, code] of Object.entries(CONSUMER_ALIASES)) {
    if (isMacroConsumerCodeSupported(code) && !(code in tokens)) {
      tokens[code] = token.toLowerCase();
    }
  }

  for (const item of CONSUMER_KEYS) {
    if (isMacroConsumerCodeSupported(item.code) && !(item.code in tokens)) {
      tokens[item.code] = `consumer_${item.code.toString(16)}`;
    }
  }

  return tokens;
}

function normalizeToken(value: string): string {
  const trimmed = value.trim();
  if (/^[-=[\]\\;'`,./]$/.test(trimmed)) {
    return trimmed;
  }
  return trimmed
    .replace(/[\s_-]+/g, "")
    .toUpperCase();
}

function matchToken(prefix: string, candidate: string): boolean {
  if (!prefix) {
    return true;
  }
  return normalizeToken(candidate).startsWith(normalizeToken(prefix));
}

function splitModifierMask(mask: number): number[] {
  return MODIFIER_ORDER.filter((bit) => (mask & bit) !== 0);
}

function formatModifierMask(mask: number): string {
  return splitModifierMask(mask)
    .map((bit) => MODIFIER_TOKEN_MAP[bit])
    .join("+");
}

function formatKeycode(keycode: number): string {
  const keyName = KEYCODE_NAMES[keycode];
  if (keyName === "→") return "Right";
  if (keyName === "←") return "Left";
  if (keyName === "↑") return "Up";
  if (keyName === "↓") return "Down";
  return keyName || `0x${keycode.toString(16).toUpperCase()}`;
}

function formatCombo(modifier: number, keycodes: readonly number[]): string {
  const parts: string[] = [];
  if (modifier) {
    parts.push(
      ...splitModifierMask(modifier).map((bit) => MODIFIER_TOKEN_MAP[bit]),
    );
  }
  parts.push(...keycodes.map((keycode) => formatKeycode(keycode)));
  return parts.join("+");
}

function makeCodeSource(line: number, column: number): MacroSourceRef {
  return { mode: "code", line, column };
}

function parseModifierToken(token: string): number | null {
  return MODIFIER_ALIASES[normalizeToken(token)] ?? null;
}

function parseKeyToken(token: string): number | null {
  return KEY_ALIASES[normalizeToken(token)] ?? null;
}

function parseMouseToken(token: string): number | null {
  return MOUSE_ALIASES[normalizeToken(token)] ?? null;
}

function parseConsumerToken(token: string): number | null {
  const normalized = token
    .trim()
    .replace(/[\s-]+/g, "_")
    .toUpperCase();
  const code = CONSUMER_ALIASES[normalized];
  if (!isMacroConsumerCodeSupported(code ?? -1)) {
    return null;
  }
  return code;
}

/**
 * Parse a single combo group (tokens joined by `+`).
 * Returns modifier mask + keycodes, or an error string.
 */
function parseComboGroup(text: string): TapGroup & { error?: string } {
  // mouse:xxx → mouse button group
  if (/^mouse\s*:/i.test(text)) {
    const mouseToken = text.replace(/^mouse\s*:/i, "");
    const btn = parseMouseToken(mouseToken);
    if (btn === null) {
      return {
        modifier: 0,
        keycodes: [],
        repeatCount: 1,
        error: `不认识的鼠标按键: ${mouseToken}`,
      };
    }
    return { modifier: 0, keycodes: [], repeatCount: 1, mouseButton: btn };
  }

  const parts = text.split(/\s*\+\s*/).filter(Boolean);
  if (parts.length === 0)
    return { modifier: 0, keycodes: [], repeatCount: 1, error: "缺少按键内容" };

  let modifier = 0;
  const keycodes: number[] = [];

  for (const part of parts) {
    const mod = parseModifierToken(part);
    if (mod !== null) {
      if ((modifier & mod) !== 0) {
        return {
          modifier,
          keycodes,
          repeatCount: 1,
          error: `tap 组合里不能重复使用同一个修饰键: ${MODIFIER_TOKEN_MAP[mod] ?? part}`,
        };
      }
      modifier |= mod;
      continue;
    }

    const key = parseKeyToken(part);
    if (key !== null) {
      if (keycodes.includes(key)) {
        return {
          modifier,
          keycodes,
          repeatCount: 1,
          error: `tap 组合里不能重复使用同一个按键: ${formatKeycode(key)}`,
        };
      }
      keycodes.push(key);
      continue;
    }

    return {
      modifier,
      keycodes,
      repeatCount: 1,
      error: `不认识的按键或修饰键: ${part}`,
    };
  }

  if (!modifier && keycodes.length === 0) {
    return {
      modifier: 0,
      keycodes: [],
      repeatCount: 1,
      error: "tap 需要至少一个修饰键或按键",
    };
  }

  return { modifier, keycodes, repeatCount: 1 };
}

/**
 * Parse tap operand.
 *
 * - `tap A+B+C hold 50ms` → 1 group (simultaneous combo)
 * - `tap A B C hold 50ms` → 3 groups (sequential individual taps)
 * - `tap Ctrl+C Ctrl+V`   → 2 groups (sequential combos)
 */
function parseTapExpression(text: string): {
  groups: TapGroup[];
  holdMs: number;
  waitMs: number;
  error?: string;
} {
  const fail = (error: string) => ({
    groups: [] as TapGroup[],
    holdMs: 0,
    waitMs: 0,
    error,
  });

  const raw = text.trim();
  if (!raw) return fail("缺少组合键内容");

  // Extract `hold Xms`, `wait Xms` clauses (global, still single each)
  const holdMatches = [
    ...raw.matchAll(/\bhold\s+(\d+(?:\.\d+)?)\s*(?:ms|s)?/gi),
  ];
  if (holdMatches.length > 1) return fail("`hold` 只能出现一次");

  const waitMatches = [
    ...raw.matchAll(/\bwait\s+(\d+(?:\.\d+)?)\s*(?:ms|s)?/gi),
  ];
  if (waitMatches.length > 1) return fail("`wait` 只能出现一次");

  let holdMs = 0;
  let waitMs = 0;
  let comboText = raw;

  if (holdMatches[0]) {
    holdMs = parseDurationMs(holdMatches[0][0].replace(/^hold\s+/i, ""));
    const holdGranErr = validateDelayGranularity(holdMs, "hold");
    if (holdGranErr) return fail(holdGranErr);
    comboText = comboText.replace(holdMatches[0][0], " ");
  }

  if (waitMatches[0]) {
    waitMs = parseDurationMs(waitMatches[0][0].replace(/^wait\s+/i, ""));
    const waitGranErr = validateDelayGranularity(waitMs, "wait");
    if (waitGranErr) return fail(waitGranErr);
    comboText = comboText.replace(waitMatches[0][0], " ");
  }

  comboText = comboText.replace(/\s+/g, " ").trim();
  // Collapse spaces around `+` so "Ctrl + C" tokenizes as one combo group
  comboText = comboText.replace(/\s*\+\s*/g, "+");

  if (/\bhold\b/i.test(comboText)) {
    return fail("`hold` 后需要时间值，例如 tap A hold 50ms 或 hold 1s");
  }

  if (/\bwait\b/i.test(comboText)) {
    return fail("`wait` 后需要时间值，例如 tap A wait 100ms 或 wait 0.5s");
  }

  if (!comboText) return fail("缺少组合键内容");

  // Split by space → tokens may be group, `*`, or number
  const tokens = comboText.split(/\s+/).filter(Boolean);
  const groups: TapGroup[] = [];

  let i = 0;
  while (i < tokens.length) {
    const token = tokens[i];

    // `*` without preceding group
    if (token === "*") {
      return fail("`*` 前需要按键，例如 tap A * 3");
    }

    // Parse a combo group
    const parsed = parseComboGroup(token);
    if (parsed.error) {
      return { groups, holdMs, waitMs, error: parsed.error };
    }

    let repeatCount = 1;
    // Check if next tokens are `* N`
    if (i + 1 < tokens.length && tokens[i + 1] === "*") {
      if (i + 2 >= tokens.length) {
        return fail("`*` 后需要整数，例如 tap A * 3");
      }
      const n = Number(tokens[i + 2]);
      if (!Number.isInteger(n) || n <= 0) {
        return fail("重复次数必须是大于 0 的整数");
      }
      repeatCount = n;
      i += 2; // skip `*` and the number
    }

    groups.push({
      modifier: parsed.modifier,
      keycodes: parsed.keycodes,
      repeatCount,
      ...(parsed.mouseButton != null ? { mouseButton: parsed.mouseButton } : {}),
    });
    i++;
  }

  if (groups.length === 0) return fail("缺少组合键内容");

  // Check for stray `*` in remaining text (shouldn't happen after tokenization)
  if (/\*/.test(comboText.replace(/\*\s*\d+/g, ""))) {
    return fail("`*` 后需要整数，例如 tap A * 3");
  }

  return { groups, holdMs, waitMs };
}

function hasSingleBit(mask: number): boolean {
  return mask !== 0 && (mask & (mask - 1)) === 0;
}

function parseDownUpOperand(
  text: string,
):
  | { kind: "single"; target: "key" | "modifier" | "mouse"; param: number }
  | { kind: "group"; modifier: number; keycodes: number[] }
  | { error: string } {
  const trimmed = text.trim();
  if (!trimmed) {
    return { error: "缺少按键内容" };
  }

  if (/^mouse\s*:/i.test(trimmed)) {
    const mouseToken = trimmed.replace(/^mouse\s*:/i, "");
    const mouse = parseMouseToken(mouseToken);
    if (mouse === null) {
      return { error: `不认识的鼠标按键: ${mouseToken}` };
    }
    return { kind: "single", target: "mouse", param: mouse };
  }

  const parts = trimmed.split(/\s*\+\s*/).filter(Boolean);
  let modifier = 0;
  const keycodes: number[] = [];

  for (const part of parts) {
    const mod = parseModifierToken(part);
    if (mod !== null) {
      if ((modifier & mod) !== 0) {
        return {
          error: `组合里不能重复使用同一个修饰键: ${MODIFIER_TOKEN_MAP[mod] ?? part}`,
        };
      }
      modifier |= mod;
      continue;
    }

    const key = parseKeyToken(part);
    if (key !== null) {
      if (keycodes.includes(key)) {
        return { error: `组合里不能重复使用同一个按键: ${formatKeycode(key)}` };
      }
      keycodes.push(key);
      continue;
    }

    return { error: `不认识的按键: ${part}` };
  }

  if (modifier === 0 && keycodes.length === 0) {
    return { error: "缺少按键内容" };
  }

  if (modifier === 0 && keycodes.length === 1) {
    return { kind: "single", target: "key", param: keycodes[0] };
  }

  if (keycodes.length === 0 && hasSingleBit(modifier)) {
    return { kind: "single", target: "modifier", param: modifier };
  }

  return { kind: "group", modifier, keycodes };
}

function getTapOnlyOperatorError(command: string, rest: string): string | null {
  if (command === "tap") {
    return null;
  }

  if (/\*/.test(rest)) {
    return "`*` 只能用于 tap，例如 tap A * 3";
  }

  if (/\bhold\b/i.test(rest)) {
    return "`hold` 只能用于 tap，例如 tap A hold 50ms";
  }

  if (/\bwait\b/i.test(rest)) {
    return "`wait` 只能用于 tap，例如 tap A wait 100ms";
  }

  return null;
}

export function parseMacroDsl(source: string): {
  steps: MacroStep[];
  diagnostics: MacroDiagnostic[];
} {
  const steps: MacroStep[] = [];
  const diagnostics: MacroDiagnostic[] = [];
  const lines = source.replace(/\r\n/g, "\n").split("\n");

  lines.forEach((rawLine, index) => {
    const lineNumber = index + 1;
    const code = rawLine.replace(/#.*$/, "").trim();
    if (!code) {
      return;
    }

    const [commandToken] = code.split(/\s+/);
    const command = commandToken.toLowerCase();
    const rest = code.slice(commandToken.length).trim();
    const sourceRef = makeCodeSource(lineNumber, 1);
    const tapOnlyOperatorError = getTapOnlyOperatorError(command, rest);

    if (tapOnlyOperatorError) {
      diagnostics.push({
        code: `dsl.${command}.operator.invalid`,
        message: tapOnlyOperatorError,
        source: sourceRef,
      });
      return;
    }

    switch (command) {
      case "tap": {
        const combo = parseTapExpression(rest);
        if (combo.error) {
          diagnostics.push({
            code: "dsl.tap.invalid",
            message: combo.error,
            source: sourceRef,
          });
          return;
        }
        steps.push({
          kind: "tap",
          groups: combo.groups,
          holdMs: combo.holdMs,
          waitMs: combo.waitMs,
          source: sourceRef,
        });
        return;
      }

      case "down":
      case "up": {
        const operand = parseDownUpOperand(rest);
        if ("error" in operand) {
          diagnostics.push({
            code: `dsl.${command}.invalid`,
            message: operand.error,
            source: sourceRef,
          });
          return;
        }

        if (operand.kind === "group") {
          steps.push({
            kind: command === "down" ? "groupDown" : "groupUp",
            modifier: operand.modifier,
            keycodes: operand.keycodes,
            source: sourceRef,
          });
        } else {
          steps.push({
            kind: command,
            target: operand.target,
            param: operand.param,
            source: sourceRef,
          });
        }
        return;
      }

      case "delay": {
        const ms = parseDurationMs(rest);
        if (ms < 0) {
          diagnostics.push({
            code: "dsl.delay.invalid",
            message: "delay 需要时间值，例如 delay 50ms 或 delay 1s",
            source: sourceRef,
          });
          return;
        }
        const delayGranErr = validateDelayGranularity(ms, "delay");
        if (delayGranErr) {
          diagnostics.push({
            code: "dsl.delay.granularity",
            message: delayGranErr,
            source: sourceRef,
          });
          return;
        }
        steps.push({
          kind: "delay",
          ms,
          source: sourceRef,
        });
        return;
      }

      case "mouse": {
        // mouse down <button> / mouse up <button> / mouse <button>
        const downMatch = rest.match(/^down\s+(.+)$/i);
        const upMatch = rest.match(/^up\s+(.+)$/i);

        if (downMatch) {
          const btn = parseMouseToken(downMatch[1]);
          if (btn === null) {
            diagnostics.push({
              code: "dsl.mouse.invalid",
              message: `不认识的鼠标按键: ${downMatch[1]}`,
              source: sourceRef,
            });
            return;
          }
          steps.push({
            kind: "down",
            target: "mouse",
            param: btn,
            source: sourceRef,
          });
          return;
        }

        if (upMatch) {
          const btn = parseMouseToken(upMatch[1]);
          if (btn === null) {
            diagnostics.push({
              code: "dsl.mouse.invalid",
              message: `不认识的鼠标按键: ${upMatch[1]}`,
              source: sourceRef,
            });
            return;
          }
          steps.push({
            kind: "up",
            target: "mouse",
            param: btn,
            source: sourceRef,
          });
          return;
        }

        const mouse = parseMouseToken(rest);
        if (mouse === null) {
          diagnostics.push({
            code: "dsl.mouse.invalid",
            message: `不认识的鼠标按键: ${rest}`,
            source: sourceRef,
          });
          return;
        }
        steps.push({
          kind: "tap",
          groups: [
            { modifier: 0, keycodes: [], repeatCount: 1, mouseButton: mouse },
          ],
          holdMs: 0,
          waitMs: 0,
          source: sourceRef,
        });
        return;
      }

      case "wheel": {
        const direction = WHEEL_TOKENS[normalizeToken(rest)];
        if (!direction) {
          diagnostics.push({
            code: "dsl.wheel.invalid",
            message: "wheel 只支持 up 或 down",
            source: sourceRef,
          });
          return;
        }
        steps.push({
          kind: "wheel",
          direction,
          source: sourceRef,
        });
        return;
      }

      case "consumer": {
        const consumer = parseConsumerToken(rest);
        if (consumer === null) {
          diagnostics.push({
            code: "dsl.consumer.invalid",
            message: `不认识的媒体键: ${rest}`,
            source: sourceRef,
          });
          return;
        }
        steps.push({
          kind: "consumer",
          code: consumer,
          source: sourceRef,
        });
        return;
      }

      default:
        diagnostics.push({
          code: "dsl.command.unknown",
          message: `不认识的指令: ${commandToken}`,
          source: sourceRef,
        });
    }
  });

  return { steps, diagnostics };
}

function getLineRange(
  source: string,
  cursor: number,
): {
  lineStart: number;
  lineEnd: number;
  lineText: string;
  beforeCursor: string;
} {
  const safeCursor = Math.max(0, Math.min(source.length, cursor));
  const lineStart = source.lastIndexOf("\n", safeCursor - 1) + 1;
  const lineEndIndex = source.indexOf("\n", safeCursor);
  const lineEnd = lineEndIndex === -1 ? source.length : lineEndIndex;
  const lineText = source.slice(lineStart, lineEnd);
  return {
    lineStart,
    lineEnd,
    lineText,
    beforeCursor: source.slice(lineStart, safeCursor),
  };
}

function buildCompletionItems(
  suggestions: Array<{
    label: string;
    insertText?: string;
    detail: string;
    kind: MacroDslCompletionItem["kind"];
  }>,
  fragment: string,
  replaceFrom: number,
  replaceTo: number,
): MacroDslCompletionItem[] {
  const seen = new Set<string>();
  return suggestions
    .filter((item) => matchToken(fragment, item.label))
    .filter((item) => {
      const key = `${item.kind}:${item.label}`;
      if (seen.has(key)) {
        return false;
      }
      seen.add(key);
      return true;
    })
    .map((item) => ({
      label: item.label,
      insertText: item.insertText ?? item.label,
      detail: item.detail,
      kind: item.kind,
      replaceFrom,
      replaceTo,
    }));
}

function completeCommand(
  source: string,
  cursor: number,
): MacroDslCompletionItem[] {
  const { lineStart, beforeCursor } = getLineRange(source, cursor);
  const leadingWhitespaceLength = beforeCursor.match(/^\s*/)?.[0].length ?? 0;
  const fragment = beforeCursor.trimStart();
  const replaceFrom = lineStart + leadingWhitespaceLength;
  const replaceTo = cursor;

  const keywordItems = buildCompletionItems(
    COMMAND_KEYWORDS.map((keyword) => ({
      label: keyword,
      insertText: `${keyword} `,
      detail: "MeowMacro 指令",
      kind: "keyword" as const,
    })),
    fragment,
    replaceFrom,
    replaceTo,
  );

  const snippetItems = buildCompletionItems(
    SNIPPET_SUGGESTIONS.map((item) => ({
      ...item,
      detail: `模板 · ${item.detail}`,
    })),
    fragment,
    replaceFrom,
    replaceTo,
  );

  return [...snippetItems, ...keywordItems];
}

function completeTapOperand(
  source: string,
  cursor: number,
  commandStart: number,
): MacroDslCompletionItem[] {
  const { beforeCursor } = getLineRange(source, cursor);
  const textAfterCommand = beforeCursor
    .slice(commandStart)
    .replace(/^tap\s+/i, "");
  const trimmedOperand = textAfterCommand.trim();

  // After a valid combo expression with trailing space, suggest hold/wait
  if (/\s$/.test(textAfterCommand) && trimmedOperand) {
    const parsed = parseTapExpression(trimmedOperand);
    if (!parsed.error) {
      const items: MacroDslCompletionItem[] = [];
      if (!/\bhold\b/i.test(trimmedOperand)) {
        items.push({
          label: "hold 50ms",
          insertText: "hold 50ms",
          detail: "按住 50ms 再释放",
          kind: "snippet",
          replaceFrom: cursor,
          replaceTo: cursor,
        });
      }
      if (!/\bwait\b/i.test(trimmedOperand)) {
        items.push({
          label: "wait 100ms",
          insertText: "wait 100ms",
          detail: "释放后等待 100ms",
          kind: "snippet",
          replaceFrom: cursor,
          replaceTo: cursor,
        });
      }
      if (items.length > 0) return items;
    }
  }

  // Inside hold value
  const holdMatch = textAfterCommand.match(/\bhold\s+([^\n]*)$/i);
  if (holdMatch) {
    const holdValue = holdMatch[1];
    const replaceFrom = cursor - holdValue.length;
    return buildCompletionItems(
      [
        { label: "20ms", detail: "按住 20ms", kind: "value" as const },
        { label: "50ms", detail: "按住 50ms", kind: "value" as const },
        { label: "100ms", detail: "按住 100ms", kind: "value" as const },
        { label: "0.5s", detail: "按住 500ms", kind: "value" as const },
        { label: "1s", detail: "按住 1000ms", kind: "value" as const },
      ],
      holdValue.trim(),
      replaceFrom,
      cursor,
    );
  }

  // Inside wait value
  const waitMatch = textAfterCommand.match(/\bwait\s+([^\n]*)$/i);
  if (waitMatch) {
    const waitValue = waitMatch[1];
    const replaceFrom = cursor - waitValue.length;
    return buildCompletionItems(
      [
        { label: "50ms", detail: "等待 50ms", kind: "value" as const },
        { label: "100ms", detail: "等待 100ms", kind: "value" as const },
        { label: "200ms", detail: "等待 200ms", kind: "value" as const },
        { label: "0.5s", detail: "等待 500ms", kind: "value" as const },
        { label: "1s", detail: "等待 1000ms", kind: "value" as const },
      ],
      waitValue.trim(),
      replaceFrom,
      cursor,
    );
  }

  // Check if the last word after a valid combo could be a prefix of hold/wait/*
  const lastSpaceIdx = textAfterCommand.lastIndexOf(" ");
  if (lastSpaceIdx > 0) {
    const comboPart = textAfterCommand.slice(0, lastSpaceIdx).trim();
    const trailingWord = textAfterCommand.slice(lastSpaceIdx + 1);
    if (trailingWord && comboPart) {
      const parsed = parseTapExpression(comboPart);
      if (!parsed.error) {
        const trailingLower = trailingWord.toLowerCase();
        const suffixItems: MacroDslCompletionItem[] = [];
        const suffixFrom = cursor - trailingWord.length;
        if ("hold".startsWith(trailingLower) && !/\bhold\b/i.test(comboPart)) {
          suffixItems.push({
            label: "hold 50ms",
            insertText: "hold 50ms",
            detail: "按住 50ms 再释放",
            kind: "snippet",
            replaceFrom: suffixFrom,
            replaceTo: cursor,
          });
        }
        if ("wait".startsWith(trailingLower) && !/\bwait\b/i.test(comboPart)) {
          suffixItems.push({
            label: "wait 100ms",
            insertText: "wait 100ms",
            detail: "释放后等待 100ms",
            kind: "snippet",
            replaceFrom: suffixFrom,
            replaceTo: cursor,
          });
        }
        if ("*".startsWith(trailingLower) && !/\*/.test(comboPart)) {
          suffixItems.push({
            label: "* 2",
            insertText: "* 2",
            detail: "重复 2 次",
            kind: "snippet",
            replaceFrom: suffixFrom,
            replaceTo: cursor,
          });
        }
        if (suffixItems.length > 0) return suffixItems;
      }
    }
  }

  // Completing key/modifier tokens in the combo
  const segmentStartInOperand = Math.max(
    textAfterCommand.lastIndexOf("+") + 1,
    0,
  );
  const replaceFrom =
    cursor - (textAfterCommand.length - segmentStartInOperand);
  const fragment = textAfterCommand.slice(segmentStartInOperand).trim();
  const replaceTo = cursor;

  const modifierItems = buildCompletionItems(
    Object.values(MODIFIER_TOKEN_MAP).map((label) => ({
      label,
      detail: "修饰键",
      kind: "symbol" as const,
    })),
    fragment,
    replaceFrom,
    replaceTo,
  );

  const keyItems = buildCompletionItems(
    DSL_KEY_COMPLETION_ITEMS,
    fragment,
    replaceFrom,
    replaceTo,
  );

  const mouseItems = buildCompletionItems(
    Object.values(MOUSE_TOKEN_MAP).map((label) => ({
      label: `mouse:${label}`,
      detail: "鼠标按键",
      kind: "value" as const,
    })),
    fragment,
    replaceFrom,
    replaceTo,
  );

  return [...modifierItems, ...keyItems, ...mouseItems];
}

function completeDownUpOperand(
  source: string,
  cursor: number,
  commandStart: number,
): MacroDslCompletionItem[] {
  const { beforeCursor } = getLineRange(source, cursor);
  const operandText = beforeCursor
    .slice(commandStart)
    .replace(/^(down|up)\s+/i, "");
  const replaceTo = cursor;

  const segmentStart = Math.max(operandText.lastIndexOf("+") + 1, 0);
  const replaceFrom = cursor - (operandText.length - segmentStart);
  const fragment = operandText.slice(segmentStart).trim();

  const snippetItems: MacroDslCompletionItem[] = [];

  const modItems = buildCompletionItems(
    Object.values(MODIFIER_TOKEN_MAP).map((label) => ({
      label,
      detail: "修饰键",
      kind: "symbol" as const,
    })),
    fragment,
    replaceFrom,
    replaceTo,
  );

  const keyItems = buildCompletionItems(
    DSL_KEY_COMPLETION_ITEMS,
    fragment,
    replaceFrom,
    replaceTo,
  );

  const mouseItems = buildCompletionItems(
    Object.values(MOUSE_TOKEN_MAP).map((label) => ({
      label: `mouse:${label}`,
      detail: "鼠标按键",
      kind: "value" as const,
    })),
    fragment,
    replaceFrom,
    replaceTo,
  );

  return [...snippetItems, ...modItems, ...keyItems, ...mouseItems];
}

function completeSimpleValue(
  source: string,
  cursor: number,
  commandStart: number,
  command: "delay" | "mouse" | "wheel" | "consumer",
): MacroDslCompletionItem[] {
  const { beforeCursor } = getLineRange(source, cursor);
  const operandText = beforeCursor
    .slice(commandStart)
    .replace(new RegExp(`^${command}\\s+`, "i"), "");
  const replaceFrom = cursor - operandText.length;
  const replaceTo = cursor;
  const fragment = operandText.trim();

  if (command === "delay") {
    return buildCompletionItems(
      [
        { label: "50ms", detail: "短延时", kind: "value" as const },
        { label: "100ms", detail: "常用延时", kind: "value" as const },
        { label: "500ms", detail: "长延时", kind: "value" as const },
      ],
      fragment,
      replaceFrom,
      replaceTo,
    );
  }

  if (command === "mouse") {
    // Legacy mouse command: only suggest button names for click
    // For press/release, use `down mouse:left` / `up mouse:left` instead
    return buildCompletionItems(
      Object.values(MOUSE_TOKEN_MAP).map((label) => ({
        label,
        detail: "鼠标按键（推荐用 tap mouse:xxx）",
        kind: "value" as const,
      })),
      fragment,
      replaceFrom,
      replaceTo,
    );
  }

  if (command === "wheel") {
    return buildCompletionItems(
      [
        { label: "up", detail: "滚轮向上", kind: "value" as const },
        { label: "down", detail: "滚轮向下", kind: "value" as const },
      ],
      fragment,
      replaceFrom,
      replaceTo,
    );
  }

  return buildCompletionItems(
    Object.values(CONSUMER_TOKEN_MAP).map((label) => ({
      label,
      detail: "媒体键",
      kind: "value" as const,
    })),
    fragment,
    replaceFrom,
    replaceTo,
  );
}

export function getMacroDslCompletions(
  source: string,
  cursor: number,
): MacroDslCompletionItem[] {
  const { beforeCursor } = getLineRange(source, cursor);
  const commentIndex = beforeCursor.indexOf("#");
  if (commentIndex >= 0) {
    return [];
  }

  const trimmedLeft = beforeCursor.trimStart();
  if (!trimmedLeft) {
    return [];
  }

  const commandMatch = trimmedLeft.match(/^([a-z]+)\b/i);
  if (!commandMatch) {
    return completeCommand(source, cursor);
  }

  const command = commandMatch[1].toLowerCase();
  const commandStart = beforeCursor.length - trimmedLeft.length;

  if (beforeCursor.length <= commandStart + command.length) {
    return completeCommand(source, cursor);
  }

  switch (command) {
    case "tap":
      return completeTapOperand(source, cursor, commandStart);
    case "down":
    case "up":
      return completeDownUpOperand(source, cursor, commandStart);
    case "delay":
    case "mouse":
    case "wheel":
    case "consumer":
      return completeSimpleValue(source, cursor, commandStart, command);
    default:
      return completeCommand(source, cursor);
  }
}

function expandStepsToInstructions(steps: MacroStep[]): MacroInstruction[] {
  const instructions: MacroInstruction[] = [];

  for (const step of steps) {
    switch (step.kind) {
      case "tap": {
        for (const group of step.groups) {
          for (let ri = 0; ri < group.repeatCount; ri++) {
            if (group.mouseButton != null) {
              // Mouse button tap
              instructions.push({
                type: MacroActionType.MOUSE_DOWN,
                param: group.mouseButton,
                source: step.source,
              });

              if (step.holdMs > 0) {
                instructions.push({
                  type: MacroActionType.DELAY,
                  param: step.holdMs,
                  source: step.source,
                });
              }

              instructions.push({
                type: MacroActionType.MOUSE_UP,
                param: group.mouseButton,
                source: step.source,
              });
            } else {
              // Keyboard combo tap
              const modifierBits = splitModifierMask(group.modifier);

              for (const bit of modifierBits) {
                instructions.push({
                  type: MacroActionType.MOD_DOWN,
                  param: bit,
                  source: step.source,
                });
              }

              for (const keycode of group.keycodes) {
                instructions.push({
                  type: MacroActionType.KEY_DOWN,
                  param: keycode,
                  source: step.source,
                });
              }

              if (step.holdMs > 0) {
                instructions.push({
                  type: MacroActionType.DELAY,
                  param: step.holdMs,
                  source: step.source,
                });
              }

              for (const keycode of [...group.keycodes].reverse()) {
                instructions.push({
                  type: MacroActionType.KEY_UP,
                  param: keycode,
                  source: step.source,
                });
              }

              for (const bit of [...modifierBits].reverse()) {
                instructions.push({
                  type: MacroActionType.MOD_UP,
                  param: bit,
                  source: step.source,
                });
              }
            }

            if (step.waitMs > 0) {
              instructions.push({
                type: MacroActionType.DELAY,
                param: step.waitMs,
                source: step.source,
              });
            }
          }
        }
        break;
      }

      case "groupDown": {
        const modifierBits = splitModifierMask(step.modifier);
        for (const bit of modifierBits) {
          instructions.push({
            type: MacroActionType.MOD_DOWN,
            param: bit,
            source: step.source,
          });
        }
        for (const keycode of step.keycodes) {
          instructions.push({
            type: MacroActionType.KEY_DOWN,
            param: keycode,
            source: step.source,
          });
        }
        break;
      }

      case "groupUp": {
        for (const keycode of [...step.keycodes].reverse()) {
          instructions.push({
            type: MacroActionType.KEY_UP,
            param: keycode,
            source: step.source,
          });
        }
        for (const bit of [...splitModifierMask(step.modifier)].reverse()) {
          instructions.push({
            type: MacroActionType.MOD_UP,
            param: bit,
            source: step.source,
          });
        }
        break;
      }

      case "down":
        instructions.push({
          type:
            step.target === "key"
              ? MacroActionType.KEY_DOWN
              : step.target === "modifier"
                ? MacroActionType.MOD_DOWN
                : MacroActionType.MOUSE_DOWN,
          param: step.param,
          source: step.source,
        });
        break;

      case "up":
        instructions.push({
          type:
            step.target === "key"
              ? MacroActionType.KEY_UP
              : step.target === "modifier"
                ? MacroActionType.MOD_UP
                : MacroActionType.MOUSE_UP,
          param: step.param,
          source: step.source,
        });
        break;

      case "delay":
        if (step.ms > 0) {
          instructions.push({
            type: MacroActionType.DELAY,
            param: step.ms,
            source: step.source,
          });
        }
        break;

      case "wheel":
        instructions.push({
          type: MacroActionType.WHEEL,
          param: step.direction,
          source: step.source,
        });
        break;

      case "consumer":
        instructions.push({
          type: MacroActionType.CONSUMER,
          param: step.code,
          source: step.source,
        });
        break;
    }
  }

  return instructions;
}

function describeAction(type: MacroActionType, param: number): string {
  switch (type) {
    case MacroActionType.KEY_DOWN:
    case MacroActionType.KEY_UP:
      return `按键 ${formatKeycode(param)}`;
    case MacroActionType.MOD_DOWN:
    case MacroActionType.MOD_UP:
      return `修饰键 ${MODIFIER_TOKEN_MAP[param] || `0x${param.toString(16)}`}`;
    case MacroActionType.MOUSE_DOWN:
    case MacroActionType.MOUSE_UP:
      return `鼠标 ${MOUSE_TOKEN_MAP[param] || `0x${param.toString(16)}`}`;
    default:
      return "动作";
  }
}

function validateAndSerializeInstructions(
  instructions: MacroInstruction[],
): Pick<
  MacroCompileState,
  "actions" | "diagnostics" | "actionCount" | "dataSize"
> {
  const diagnostics: MacroDiagnostic[] = [];
  const actions: MacroAction[] = [];

  const openKeys = new Map<number, MacroSourceRef | undefined>();
  const openModifiers = new Map<number, MacroSourceRef | undefined>();
  const openMouse = new Map<number, MacroSourceRef | undefined>();

  for (const instruction of instructions) {
    switch (instruction.type) {
      case MacroActionType.KEY_DOWN:
        if (openKeys.has(instruction.param)) {
          diagnostics.push({
            code: "macro.key.duplicate_down",
            message: `${describeAction(instruction.type, instruction.param)} 重复按下，前一次尚未释放`,
            source: instruction.source,
          });
        } else {
          openKeys.set(instruction.param, instruction.source);
          if (openKeys.size > 6) {
            diagnostics.push({
              code: "macro.key.rollover_limit",
              message: `同时按下的普通按键超过 6 个，当前协议最多只支持 6 键无修饰同时按下`,
              source: instruction.source,
            });
          }
        }
        break;

      case MacroActionType.KEY_UP:
        if (!openKeys.has(instruction.param)) {
          diagnostics.push({
            code: "macro.key.up_without_down",
            message: `${describeAction(instruction.type, instruction.param)} 在按下之前被释放`,
            source: instruction.source,
          });
        } else {
          openKeys.delete(instruction.param);
        }
        break;

      case MacroActionType.MOD_DOWN:
        if (openModifiers.has(instruction.param)) {
          diagnostics.push({
            code: "macro.mod.duplicate_down",
            message: `${describeAction(instruction.type, instruction.param)} 重复按下，前一次尚未释放`,
            source: instruction.source,
          });
        } else {
          openModifiers.set(instruction.param, instruction.source);
        }
        break;

      case MacroActionType.MOD_UP:
        if (!openModifiers.has(instruction.param)) {
          diagnostics.push({
            code: "macro.mod.up_without_down",
            message: `${describeAction(instruction.type, instruction.param)} 在按下之前被释放`,
            source: instruction.source,
          });
        } else {
          openModifiers.delete(instruction.param);
        }
        break;

      case MacroActionType.MOUSE_DOWN:
        if (openMouse.has(instruction.param)) {
          diagnostics.push({
            code: "macro.mouse.duplicate_down",
            message: `${describeAction(instruction.type, instruction.param)} 重复按下，前一次尚未释放`,
            source: instruction.source,
          });
        } else {
          openMouse.set(instruction.param, instruction.source);
        }
        break;

      case MacroActionType.MOUSE_UP:
        if (!openMouse.has(instruction.param)) {
          diagnostics.push({
            code: "macro.mouse.up_without_down",
            message: `${describeAction(instruction.type, instruction.param)} 在按下之前被释放`,
            source: instruction.source,
          });
        } else {
          openMouse.delete(instruction.param);
        }
        break;
    }

    if (instruction.type === MacroActionType.DELAY) {
      let remaining = instruction.param;
      while (remaining > 0) {
        const chunk = Math.min(remaining, DELAY_ACTION_MAX_MS);
        actions.push({
          type: MacroActionType.DELAY,
          param: Math.round(chunk / 10),
        });
        remaining -= chunk;
      }
      continue;
    }

    actions.push({
      type: instruction.type,
      param: instruction.param,
    });
  }

  for (const [keycode, source] of openKeys) {
    diagnostics.push({
      code: "macro.key.missing_up",
      message: `按键 ${formatKeycode(keycode)} 已按下，但直到宏结束都没有释放`,
      source,
    });
  }

  for (const [modifier, source] of openModifiers) {
    diagnostics.push({
      code: "macro.mod.missing_up",
      message: `修饰键 ${MODIFIER_TOKEN_MAP[modifier] || modifier} 已按下，但直到宏结束都没有释放`,
      source,
    });
  }

  for (const [button, source] of openMouse) {
    diagnostics.push({
      code: "macro.mouse.missing_up",
      message: `鼠标 ${MOUSE_TOKEN_MAP[button] || button} 已按下，但直到宏结束都没有释放`,
      source,
    });
  }

  const actionCount = actions.length + 1;
  const dataSize = actionCount * 2;

  if (actionCount > MACRO_MAX_ACTIONS) {
    diagnostics.push({
      code: "macro.limit.action_count",
      message: `编码动作数超限：${actionCount} / ${MACRO_MAX_ACTIONS}`,
    });
  }

  if (dataSize > MACRO_MAX_DATA_SIZE) {
    diagnostics.push({
      code: "macro.limit.data_size",
      message: `宏数据大小超限：${dataSize} / ${MACRO_MAX_DATA_SIZE} 字节`,
    });
  }

  actions.push({ type: MacroActionType.END, param: 0 });

  return {
    actions,
    diagnostics,
    actionCount,
    dataSize,
  };
}

export function compileMacroDsl(source: string): MacroCompileState {
  const parsed = parseMacroDsl(source);
  const serialized = validateAndSerializeInstructions(
    expandStepsToInstructions(parsed.steps),
  );

  return {
    steps: parsed.steps,
    actions: serialized.actions,
    diagnostics: [...parsed.diagnostics, ...serialized.diagnostics],
    actionCount: serialized.actionCount,
    dataSize: serialized.dataSize,
  };
}

export function compileMacroCards(
  cards: readonly MacroCardLike[],
): MacroCompileState {
  const steps: MacroStep[] = [];
  const instructions: MacroInstruction[] = [];

  cards.forEach((card, index) => {
    const source: MacroSourceRef = { mode: "visual", cardIndex: index };

    switch (card.action.type) {
      case MacroActionType.KEY_DOWN:
        steps.push({
          kind: "down",
          target: "key",
          param: card.action.param,
          source,
        });
        break;
      case MacroActionType.KEY_UP:
        steps.push({
          kind: "up",
          target: "key",
          param: card.action.param,
          source,
        });
        break;
      case MacroActionType.MOD_DOWN:
        steps.push({
          kind: "down",
          target: "modifier",
          param: card.action.param,
          source,
        });
        break;
      case MacroActionType.MOD_UP:
        steps.push({
          kind: "up",
          target: "modifier",
          param: card.action.param,
          source,
        });
        break;
      case MacroActionType.MOUSE_DOWN:
        steps.push({
          kind: "down",
          target: "mouse",
          param: card.action.param,
          source,
        });
        break;
      case MacroActionType.MOUSE_UP:
        steps.push({
          kind: "up",
          target: "mouse",
          param: card.action.param,
          source,
        });
        break;
      case MacroActionType.WHEEL:
        steps.push({
          kind: "wheel",
          direction: card.action.param === 1 ? 1 : 2,
          source,
        });
        break;
      case MacroActionType.CONSUMER:
        steps.push({ kind: "consumer", code: card.action.param, source });
        break;
    }

    instructions.push({
      type: card.action.type,
      param: card.action.param,
      source,
    });

    if (card.delayMs > 0) {
      steps.push({ kind: "delay", ms: card.delayMs, source });
      instructions.push({
        type: MacroActionType.DELAY,
        param: card.delayMs,
        source,
      });
    }
  });

  const serialized = validateAndSerializeInstructions(instructions);
  return {
    steps,
    actions: serialized.actions,
    diagnostics: serialized.diagnostics,
    actionCount: serialized.actionCount,
    dataSize: serialized.dataSize,
  };
}

// ---------------------------------------------------------------------------
// Cards → DSL: folding helpers
// ---------------------------------------------------------------------------

/** Build tap suffix: ` hold Xms`, ` wait Xms` */
function formatTapSuffix(holdMs: number, waitMs: number): string {
  let suffix = "";
  if (holdMs > 0) suffix += ` hold ${holdMs}ms`;
  if (waitMs > 0) suffix += ` wait ${waitMs}ms`;
  return suffix;
}

/** Try to fold a mouse click pair: MOUSE_DOWN + MOUSE_UP → `mouse <button>` */
function tryFoldMouseClick(
  cards: readonly MacroCardLike[],
  startIndex: number,
): { line: string; delayMs: number; nextIndex: number } | null {
  const first = cards[startIndex];
  const second = cards[startIndex + 1];
  if (!first || !second) return null;

  if (
    first.action.type === MacroActionType.MOUSE_DOWN &&
    second.action.type === MacroActionType.MOUSE_UP &&
    second.action.param === first.action.param
  ) {
    const holdMs = first.delayMs;
    const waitMs = second.delayMs;
    return {
      line: `tap mouse:${MOUSE_TOKEN_MAP[first.action.param] || first.action.param}${formatTapSuffix(holdMs, waitMs)}`,
      delayMs: 0,
      nextIndex: startIndex + 2,
    };
  }
  return null;
}

/**
 * Try to fold a tap pattern.
 * Handles: simple key/mod pair, multi-key combo, combos with hold delay.
 * Trailing delay on the last UP card becomes `wait Xms`.
 */
function tryFoldTap(
  cards: readonly MacroCardLike[],
  startIndex: number,
): {
  line: string;
  delayMs: number;
  nextIndex: number;
} | null {
  const first = cards[startIndex];
  const second = cards[startIndex + 1];
  if (!first || !second) {
    return null;
  }

  // Simple single key: KEY_DOWN + KEY_UP (same param)
  if (
    first.action.type === MacroActionType.KEY_DOWN &&
    second.action.type === MacroActionType.KEY_UP &&
    second.action.param === first.action.param
  ) {
    return {
      line: `tap ${formatKeycode(first.action.param)}${formatTapSuffix(first.delayMs, second.delayMs)}`,
      delayMs: 0,
      nextIndex: startIndex + 2,
    };
  }

  // Simple single modifier: MOD_DOWN + MOD_UP (same param)
  if (
    first.action.type === MacroActionType.MOD_DOWN &&
    second.action.type === MacroActionType.MOD_UP &&
    second.action.param === first.action.param
  ) {
    return {
      line: `tap ${formatModifierMask(first.action.param)}${formatTapSuffix(first.delayMs, second.delayMs)}`,
      delayMs: 0,
      nextIndex: startIndex + 2,
    };
  }

  // Multi-key combo: MOD_DOWN* KEY_DOWN* [hold] KEY_UP* MOD_UP*
  let cursor = startIndex;
  const modifierBits: number[] = [];
  while (
    cards[cursor]?.action.type === MacroActionType.MOD_DOWN &&
    cards[cursor].delayMs === 0
  ) {
    modifierBits.push(cards[cursor].action.param);
    cursor++;
  }

  // Collect KEY_DOWN cards — all must have delayMs===0 except the LAST one
  // (which may carry the hold delay)
  const keycodes: number[] = [];
  let holdMs = 0;
  while (cards[cursor]?.action.type === MacroActionType.KEY_DOWN) {
    const keycode = cards[cursor].action.param;
    if (keycodes.includes(keycode)) return null;

    // If previous KEY_DOWN had a delay, it's not a clean combo
    if (keycodes.length > 0 && cards[cursor - 1].delayMs !== 0) return null;

    keycodes.push(keycode);
    cursor++;
  }

  // The last KEY_DOWN may carry a hold delay
  if (keycodes.length > 0) {
    holdMs = cards[cursor - 1].delayMs;
  }

  if (modifierBits.length === 0 && keycodes.length === 0) {
    return null;
  }

  // Match KEY_UP in reverse order
  if (keycodes.length > 0) {
    for (const keycode of [...keycodes].reverse()) {
      const keyUp = cards[cursor];
      if (
        !keyUp ||
        keyUp.action.type !== MacroActionType.KEY_UP ||
        keyUp.action.param !== keycode ||
        keyUp.delayMs !== 0
      ) {
        return null;
      }
      cursor++;
    }
  }

  // Keys-only combo (no modifiers)
  if (modifierBits.length === 0) {
    const waitMs = cards[cursor - 1]?.delayMs ?? 0;
    return {
      line: `tap ${formatCombo(0, keycodes)}${formatTapSuffix(holdMs, waitMs)}`,
      delayMs: 0,
      nextIndex: cursor,
    };
  }

  // For modifier-only (no keycodes), the last MOD_DOWN must be the card before cursor
  const lastBeforeCursor = cards[cursor - 1];
  if (
    keycodes.length === 0 &&
    (!lastBeforeCursor ||
      lastBeforeCursor.action.type !== MacroActionType.MOD_DOWN)
  ) {
    return null;
  }

  // If modifier-only, pick up holdMs from last MOD_DOWN
  if (keycodes.length === 0) {
    holdMs = lastBeforeCursor.delayMs;
  }

  // Match MOD_UP cards
  const seenUps: number[] = [];
  while (seenUps.length < modifierBits.length) {
    const card = cards[cursor];
    if (!card || card.action.type !== MacroActionType.MOD_UP) {
      return null;
    }

    if (
      !modifierBits.includes(card.action.param) ||
      seenUps.includes(card.action.param)
    ) {
      return null;
    }

    if (seenUps.length < modifierBits.length - 1 && card.delayMs !== 0) {
      return null;
    }

    seenUps.push(card.action.param);
    cursor++;
  }

  const mask = modifierBits.reduce((acc, bit) => acc | bit, 0);
  const waitMs = cards[cursor - 1]?.delayMs ?? 0;

  return {
    line: `tap ${formatCombo(mask, keycodes)}${formatTapSuffix(holdMs, waitMs)}`,
    delayMs: 0,
    nextIndex: cursor,
  };
}

/** Try to fold consecutive same-direction down/up into `down Mod+Key` or `up Mod+Key` */
function tryFoldGroup(
  cards: readonly MacroCardLike[],
  startIndex: number,
): { line: string; delayMs: number; nextIndex: number } | null {
  const first = cards[startIndex];
  if (!first) return null;

  const isDown =
    first.action.type === MacroActionType.MOD_DOWN ||
    first.action.type === MacroActionType.KEY_DOWN;
  const isUp =
    first.action.type === MacroActionType.MOD_UP ||
    first.action.type === MacroActionType.KEY_UP;
  if (!isDown && !isUp) return null;

  const direction = isDown ? "down" : "up";
  const modType = isDown ? MacroActionType.MOD_DOWN : MacroActionType.MOD_UP;
  const keyType = isDown ? MacroActionType.KEY_DOWN : MacroActionType.KEY_UP;

  let cursor = startIndex;
  const modifierBits: number[] = [];
  const keycodes: number[] = [];

  // Collect consecutive mods
  while (cards[cursor]?.action.type === modType) {
    if (cursor !== startIndex && cards[cursor - 1].delayMs !== 0) break;
    modifierBits.push(cards[cursor].action.param);
    cursor++;
  }

  // Collect consecutive keys
  while (cards[cursor]?.action.type === keyType) {
    if (cursor !== startIndex && cards[cursor - 1].delayMs !== 0) break;
    keycodes.push(cards[cursor].action.param);
    cursor++;
  }

  const total = modifierBits.length + keycodes.length;
  if (total < 2) return null; // No benefit folding a single action

  const mask = modifierBits.reduce((acc, bit) => acc | bit, 0);
  const trailingDelay = cards[cursor - 1]?.delayMs ?? 0;

  return {
    line: `${direction} ${formatCombo(mask, keycodes)}`,
    delayMs: trailingDelay,
    nextIndex: cursor,
  };
}

function formatActionCard(card: MacroCardLike): string {
  switch (card.action.type) {
    case MacroActionType.KEY_DOWN:
      return `down ${formatKeycode(card.action.param)}`;
    case MacroActionType.KEY_UP:
      return `up ${formatKeycode(card.action.param)}`;
    case MacroActionType.MOD_DOWN:
      return `down ${formatModifierMask(card.action.param)}`;
    case MacroActionType.MOD_UP:
      return `up ${formatModifierMask(card.action.param)}`;
    case MacroActionType.MOUSE_DOWN:
      return `down mouse:${MOUSE_TOKEN_MAP[card.action.param] || card.action.param}`;
    case MacroActionType.MOUSE_UP:
      return `up mouse:${MOUSE_TOKEN_MAP[card.action.param] || card.action.param}`;
    case MacroActionType.WHEEL:
      return `wheel ${card.action.param === 1 ? "up" : "down"}`;
    case MacroActionType.CONSUMER:
      return `consumer ${CONSUMER_TOKEN_MAP[card.action.param] || `consumer_${card.action.param.toString(16)}`}`;
    default:
      return "";
  }
}

/** Split `tap Ctrl+C hold 50ms wait 100ms` → combo="Ctrl+C", suffix=" hold 50ms wait 100ms" */
function splitTapLine(line: string): { combo: string; suffix: string } {
  const afterTap = line.slice(4); // remove "tap "
  const holdIdx = afterTap.search(/\s+hold\s/i);
  const waitIdx = afterTap.search(/\s+wait\s/i);
  const cuts = [holdIdx, waitIdx].filter((i) => i >= 0);
  const cutAt = cuts.length > 0 ? Math.min(...cuts) : afterTap.length;
  return {
    combo: afterTap.slice(0, cutAt).trim(),
    suffix: afterTap.slice(cutAt),
  };
}

/** Check if two fold results represent the same tap (for repeat folding) */
function isSameTap(a: string, b: string): boolean {
  return a === b;
}

/** Run-length encode consecutive identical combos: [A, A, A, B, C, C, C, C] → "A * 3 B C * 4" */
function compressComboRuns(combos: string[]): string {
  const parts: string[] = [];
  let i = 0;
  while (i < combos.length) {
    let runLen = 1;
    while (i + runLen < combos.length && combos[i + runLen] === combos[i]) {
      runLen++;
    }
    parts.push(runLen >= 3 ? `${combos[i]} * ${runLen}` : combos.slice(i, i + runLen).join(" "));
    i += runLen;
  }
  return parts.join(" ");
}

export function formatMacroDslFromCards(
  cards: readonly MacroCardLike[],
): string {
  // Phase 1: fold individual patterns into line+delay pairs
  const segments: { line: string; delayMs: number }[] = [];
  let index = 0;

  while (index < cards.length) {
    // Try mouse click fold first (MOUSE_DOWN+MOUSE_UP → mouse <btn>)
    const mouse = tryFoldMouseClick(cards, index);
    if (mouse) {
      segments.push({ line: mouse.line, delayMs: mouse.delayMs });
      index = mouse.nextIndex;
      continue;
    }

    // Try tap fold (key/mod combos)
    const tap = tryFoldTap(cards, index);
    if (tap) {
      segments.push({ line: tap.line, delayMs: tap.delayMs });
      index = tap.nextIndex;
      continue;
    }

    // Try group fold (down Mod+Key / up Mod+Key)
    const group = tryFoldGroup(cards, index);
    if (group) {
      segments.push({ line: group.line, delayMs: group.delayMs });
      index = group.nextIndex;
      continue;
    }

    // Fallback: single card
    const card = cards[index];
    const line = formatActionCard(card);
    if (line) {
      segments.push({ line, delayMs: card.delayMs });
    }
    index++;
  }

  // Phase 1.5: merge consecutive single-combo taps with identical timing
  // into sequential `tap A B C D hold Xms wait Yms`
  // with run-length encoding: runs of ≥3 identical combos → `A * N`
  const merged: { line: string; delayMs: number }[] = [];
  {
    let si = 0;
    while (si < segments.length) {
      const seg = segments[si];
      // Only merge `tap <combo> [hold X] [wait Y]` segments with delayMs===0
      if (seg.line.startsWith("tap ") && seg.delayMs === 0) {
        const { combo, suffix } = splitTapLine(seg.line);
        if (combo) {
          const combos = [combo];
          let ni = si + 1;
          while (ni < segments.length && segments[ni].delayMs === 0) {
            const next = segments[ni];
            if (!next.line.startsWith("tap ")) break;
            const nextParts = splitTapLine(next.line);
            if (nextParts.suffix !== suffix || !nextParts.combo) break;
            combos.push(nextParts.combo);
            ni++;
          }
          if (combos.length > 1) {
            // Run-length encode identical consecutive combos
            const compressed = compressComboRuns(combos);
            merged.push({
              line: `tap ${compressed}${suffix}`,
              delayMs: 0,
            });
            si = ni;
            continue;
          }
        }
      }
      merged.push(seg);
      si++;
    }
  }

  // Phase 2: fold consecutive identical taps into * N (per-group)
  const lines: string[] = [];
  let si = 0;
  while (si < merged.length) {
    const seg = merged[si];
    const isFoldable = seg.line.startsWith("tap ") && !seg.line.includes(" * ");

    if (isFoldable) {
      let count = 1;
      while (
        si + count < merged.length &&
        isSameTap(merged[si + count].line, seg.line) &&
        merged[si + count].delayMs === seg.delayMs
      ) {
        count++;
      }
      if (count > 1) {
        // Per-group repeat: `tap A B` × 3 → `tap A * 3 B * 3`
        const { combo, suffix } = splitTapLine(seg.line);
        const groups = combo.split(/\s+/).filter(Boolean);
        const repeated = groups.map((g) => `${g} * ${count}`).join(" ");
        lines.push(`tap ${repeated}${suffix}`);
        if (seg.delayMs > 0) {
          lines.push(`delay ${seg.delayMs}ms`);
        }
        si += count;
        continue;
      }
    }

    lines.push(seg.line);
    if (seg.delayMs > 0) {
      lines.push(`delay ${seg.delayMs}ms`);
    }
    si++;
  }

  return lines.join("\n");
}
