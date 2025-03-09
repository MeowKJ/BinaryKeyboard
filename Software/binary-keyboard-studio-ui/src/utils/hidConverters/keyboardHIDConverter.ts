import type { KeyboardConfig, KeyBoardRawHIDData } from "@/types";
import { KeyboardKeyModifier } from "@/types";

/** 完整的 HID 映射表，将 event.code 映射为 HID 键码（8位数字） */
export const HID_KEY_MAP: Record<string, number> = {
  // 错误及保留项
  ErrorRollOver: 0x01,
  POSTFail: 0x02,
  ErrorUndefined: 0x03,
  // 字母 A-Z
  KeyA: 0x04,
  KeyB: 0x05,
  KeyC: 0x06,
  KeyD: 0x07,
  KeyE: 0x08,
  KeyF: 0x09,
  KeyG: 0x0a,
  KeyH: 0x0b,
  KeyI: 0x0c,
  KeyJ: 0x0d,
  KeyK: 0x0e,
  KeyL: 0x0f,
  KeyM: 0x10,
  KeyN: 0x11,
  KeyO: 0x12,
  KeyP: 0x13,
  KeyQ: 0x14,
  KeyR: 0x15,
  KeyS: 0x16,
  KeyT: 0x17,
  KeyU: 0x18,
  KeyV: 0x19,
  KeyW: 0x1a,
  KeyX: 0x1b,
  KeyY: 0x1c,
  KeyZ: 0x1d,
  // 数字（主键盘部分）
  Digit1: 0x1e,
  Digit2: 0x1f,
  Digit3: 0x20,
  Digit4: 0x21,
  Digit5: 0x22,
  Digit6: 0x23,
  Digit7: 0x24,
  Digit8: 0x25,
  Digit9: 0x26,
  Digit0: 0x27,
  // 控制键及标点符号
  Enter: 0x28,
  Escape: 0x29,
  Backspace: 0x2a,
  Tab: 0x2b,
  Space: 0x2c,
  Minus: 0x2d,
  Equal: 0x2e,
  BracketLeft: 0x2f,
  BracketRight: 0x30,
  Backslash: 0x31,
  NonUSHash: 0x32,
  Semicolon: 0x33,
  Quote: 0x34,
  Backquote: 0x35,
  Comma: 0x36,
  Period: 0x37,
  Slash: 0x38,
  CapsLock: 0x39,
  // 功能键 F1-F12
  F1: 0x3a,
  F2: 0x3b,
  F3: 0x3c,
  F4: 0x3d,
  F5: 0x3e,
  F6: 0x3f,
  F7: 0x40,
  F8: 0x41,
  F9: 0x42,
  F10: 0x43,
  F11: 0x44,
  F12: 0x45,
  // 系统及导航键
  PrintScreen: 0x46,
  ScrollLock: 0x47,
  Pause: 0x48,
  Insert: 0x49,
  Home: 0x4a,
  PageUp: 0x4b,
  Delete: 0x4c,
  End: 0x4d,
  PageDown: 0x4e,
  ArrowRight: 0x4f,
  ArrowLeft: 0x50,
  ArrowDown: 0x51,
  ArrowUp: 0x52,
  // 数字键盘区
  NumLock: 0x53,
  KeypadDivide: 0x54,
  KeypadMultiply: 0x55,
  KeypadSubtract: 0x56,
  KeypadAdd: 0x57,
  KeypadEnter: 0x58,
  Keypad1: 0x59,
  Keypad2: 0x5a,
  Keypad3: 0x5b,
  Keypad4: 0x5c,
  Keypad5: 0x5d,
  Keypad6: 0x5e,
  Keypad7: 0x5f,
  Keypad8: 0x60,
  Keypad9: 0x61,
  Keypad0: 0x62,
  KeypadDecimal: 0x63,
  // 国际化及扩展键
  IntlBackslash: 0x64,
  ContextMenu: 0x65,
  Power: 0x66,
  KeypadEqual: 0x67,
  F13: 0x68,
  F14: 0x69,
  F15: 0x6a,
  F16: 0x6b,
  F17: 0x6c,
  F18: 0x6d,
  F19: 0x6e,
  F20: 0x6f,
  F21: 0x70,
  F22: 0x71,
  F23: 0x72,
  F24: 0x73,
  Open: 0x74,
  Help: 0x75,
  Select: 0x76,
  Again: 0x77,
  Undo: 0x78,
  Cut: 0x79,
  Copy: 0x7a,
  Paste: 0x7b,
  Find: 0x7c,
  AudioVolumeMute: 0x7f,
  AudioVolumeUp: 0x80,
  AudioVolumeDown: 0x81,
  LockingCapsLock: 0x82,
  LockingNumLock: 0x83,
  LockingScrollLock: 0x84,
  KeypadComma: 0x85,
  KeypadEqualSign: 0x86,
  International1: 0x87,
  International2: 0x88,
  International3: 0x89,
  International4: 0x8a,
  International5: 0x8b,
  International6: 0x8c,
  International7: 0x8d,
  International8: 0x8e,
  International9: 0x8f,
  Lang1: 0x90,
  Lang2: 0x91,
  Lang3: 0x92,
  Lang4: 0x93,
  Lang5: 0x94,
  Lang6: 0x95,
  Lang7: 0x96,
  Lang8: 0x97,
  Lang9: 0x98,
  AlternateErase: 0x99,
  "SysReq/Attention": 0x9a,
  Cancel: 0x9b,
  Clear: 0x9c,
  Prior: 0x9d,
  Return: 0x9e,
  Separator: 0x9f,
  Out: 0xa0,
  Oper: 0xa1,
  "Clear/Again": 0xa2,
  "CrSel/Props": 0xa3,
  ExSel: 0xa4,
  // 修饰键（包含在内，但一般单独处理）
  ControlLeft: 0xe0,
  ShiftLeft: 0xe1,
  AltLeft: 0xe2,
  MetaLeft: 0xe3,
  ControlRight: 0xe4,
  ShiftRight: 0xe5,
  AltRight: 0xe6,
  MetaRight: 0xe7,
};

/** 生成 HID 的反向映射表，从 HID 数值到 event.code 字符串 */
const REVERSE_HID_KEY_MAP: Record<number, string> = {};
for (const code in HID_KEY_MAP) {
  const value = HID_KEY_MAP[code];
  REVERSE_HID_KEY_MAP[value] = code;
}

/**
 * 将 KeyboardConfig 转换为 RawHIDData（包含修饰键和主键 HID 码）
 * @param config KeyboardConfig 对象
 * @returns RawHIDData 对象，其中 modifier 为修饰键编码(uint8)，hid 为主键的 HID 数值
 */
export function keyboardConfigToRawHID(
  config: KeyboardConfig
): KeyBoardRawHIDData {
  let modifier = 0;
  if (config.leftCtrlKey) modifier |= KeyboardKeyModifier.LeftCtrl;
  if (config.leftShiftKey) modifier |= KeyboardKeyModifier.LeftShift;
  if (config.leftAltKey) modifier |= KeyboardKeyModifier.LeftAlt;
  if (config.leftMetaKey) modifier |= KeyboardKeyModifier.LeftMeta;
  if (config.rightCtrlKey) modifier |= KeyboardKeyModifier.RightCtrl;
  if (config.rightShiftKey) modifier |= KeyboardKeyModifier.RightShift;
  if (config.rightAltKey) modifier |= KeyboardKeyModifier.RightAlt;
  if (config.rightMetaKey) modifier |= KeyboardKeyModifier.RightMeta;

  const hid = HID_KEY_MAP[config.code] || 0;
  return { modifier, hid };
}

/**
 * 将 RawHIDData 转换为 KeyboardConfig 对象
 * 由于 HID 数据无法还原出原始按键字符，故 key 字段置为空字符串
 * @param raw RawHIDData 对象
 * @returns KeyboardConfig 对象，其中 code 根据反向映射表还原，key 为空
 */
export function rawHIDToKeyboardConfig(
  raw: KeyBoardRawHIDData
): KeyboardConfig {
  const code = REVERSE_HID_KEY_MAP[raw.hid] || "";
  return {
    key: "",
    code,
    leftCtrlKey: !!(raw.modifier & KeyboardKeyModifier.LeftCtrl),
    leftShiftKey: !!(raw.modifier & KeyboardKeyModifier.LeftShift),
    leftAltKey: !!(raw.modifier & KeyboardKeyModifier.LeftAlt),
    leftMetaKey: !!(raw.modifier & KeyboardKeyModifier.LeftMeta),
    rightCtrlKey: !!(raw.modifier & KeyboardKeyModifier.RightCtrl),
    rightShiftKey: !!(raw.modifier & KeyboardKeyModifier.RightShift),
    rightAltKey: !!(raw.modifier & KeyboardKeyModifier.RightAlt),
    rightMetaKey: !!(raw.modifier & KeyboardKeyModifier.RightMeta),
  };
}

/**
 * 将 KeyboardConfig 的修饰键部分转换为 8 位修饰键编码 (uint8)
 * @param config KeyboardConfig 对象
 * @returns 8 位修饰键编码
 */
export function keyboardConfigToKeyboardKeyModifier(
  config: KeyboardConfig
): number {
  let modifier = 0;
  if (config.leftCtrlKey) modifier |= KeyboardKeyModifier.LeftCtrl;
  if (config.leftShiftKey) modifier |= KeyboardKeyModifier.LeftShift;
  if (config.leftAltKey) modifier |= KeyboardKeyModifier.LeftAlt;
  if (config.leftMetaKey) modifier |= KeyboardKeyModifier.LeftMeta;
  if (config.rightCtrlKey) modifier |= KeyboardKeyModifier.RightCtrl;
  if (config.rightShiftKey) modifier |= KeyboardKeyModifier.RightShift;
  if (config.rightAltKey) modifier |= KeyboardKeyModifier.RightAlt;
  if (config.rightMetaKey) modifier |= KeyboardKeyModifier.RightMeta;
  return modifier;
}

/**
 * 将 8 位修饰键编码转换为 KeyboardConfig 部分（仅包含修饰键布尔值）
 * @param modifier 8 位修饰键编码 (uint8)
 * @returns 包含修饰键状态的 KeyboardConfig 部分
 */
export function modifierToKeyboardConfig(
  modifier: number
): Partial<KeyboardConfig> {
  return {
    leftCtrlKey: !!(modifier & KeyboardKeyModifier.LeftCtrl),
    leftShiftKey: !!(modifier & KeyboardKeyModifier.LeftShift),
    leftAltKey: !!(modifier & KeyboardKeyModifier.LeftAlt),
    leftMetaKey: !!(modifier & KeyboardKeyModifier.LeftMeta),
    rightCtrlKey: !!(modifier & KeyboardKeyModifier.RightCtrl),
    rightShiftKey: !!(modifier & KeyboardKeyModifier.RightShift),
    rightAltKey: !!(modifier & KeyboardKeyModifier.RightAlt),
    rightMetaKey: !!(modifier & KeyboardKeyModifier.RightMeta),
  };
}

export const getKeyNameCombination = (keyConfig: KeyboardConfig) => {
  const keys = new Set<string>(); // 使用 Set 自动去重
  let leftModifierKeysValue = new Set<string>();
  let rightModifierKeysValue = new Set<string>();

  if (keyConfig.leftCtrlKey) leftModifierKeysValue.add("Ctrl");
  if (keyConfig.leftShiftKey) leftModifierKeysValue.add("Shift");
  if (keyConfig.leftAltKey) leftModifierKeysValue.add("Alt");
  if (keyConfig.leftMetaKey) leftModifierKeysValue.add("Meta");
  if (keyConfig.rightCtrlKey) rightModifierKeysValue.add("Ctrl");
  if (keyConfig.rightShiftKey) rightModifierKeysValue.add("Shift");
  if (keyConfig.rightAltKey) rightModifierKeysValue.add("Alt");
  if (keyConfig.rightMetaKey) rightModifierKeysValue.add("Meta");

  leftModifierKeysValue.forEach((key) => keys.add(`L${key}`));
  rightModifierKeysValue.forEach((key) => keys.add(`R${key}`));

  if (keyConfig.code) {
    keys.add(keyConfig.code); // 加入主按键
  }
  return Array.from(keys).join(" + ");
};
