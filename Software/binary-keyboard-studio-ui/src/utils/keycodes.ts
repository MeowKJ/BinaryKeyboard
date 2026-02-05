/**
 * HID 键码定义和工具函数
 */

import { Modifier } from '@/types/protocol';

/** HID 键码到名称映射 */
export const KEYCODE_NAMES: Record<number, string> = {
  0x00: '',
  0x04: 'A', 0x05: 'B', 0x06: 'C', 0x07: 'D', 0x08: 'E', 0x09: 'F',
  0x0a: 'G', 0x0b: 'H', 0x0c: 'I', 0x0d: 'J', 0x0e: 'K', 0x0f: 'L',
  0x10: 'M', 0x11: 'N', 0x12: 'O', 0x13: 'P', 0x14: 'Q', 0x15: 'R',
  0x16: 'S', 0x17: 'T', 0x18: 'U', 0x19: 'V', 0x1a: 'W', 0x1b: 'X',
  0x1c: 'Y', 0x1d: 'Z',
  0x1e: '1', 0x1f: '2', 0x20: '3', 0x21: '4', 0x22: '5',
  0x23: '6', 0x24: '7', 0x25: '8', 0x26: '9', 0x27: '0',
  0x28: 'Enter', 0x29: 'Esc', 0x2a: 'Backspace', 0x2b: 'Tab', 0x2c: 'Space',
  0x2d: '-', 0x2e: '=', 0x2f: '[', 0x30: ']', 0x31: '\\',
  0x33: ';', 0x34: "'", 0x35: '`', 0x36: ',', 0x37: '.', 0x38: '/',
  0x39: 'CapsLock',
  0x3a: 'F1', 0x3b: 'F2', 0x3c: 'F3', 0x3d: 'F4', 0x3e: 'F5', 0x3f: 'F6',
  0x40: 'F7', 0x41: 'F8', 0x42: 'F9', 0x43: 'F10', 0x44: 'F11', 0x45: 'F12',
  0x46: 'PrtSc', 0x47: 'ScrollLock', 0x48: 'Pause',
  0x49: 'Insert', 0x4a: 'Home', 0x4b: 'PageUp',
  0x4c: 'Delete', 0x4d: 'End', 0x4e: 'PageDown',
  0x4f: '→', 0x50: '←', 0x51: '↓', 0x52: '↑',
  0x53: 'NumLock', 0x54: '/', 0x55: '*', 0x56: '-', 0x57: '+', 0x58: 'Enter',
  0x59: 'Num1', 0x5a: 'Num2', 0x5b: 'Num3', 0x5c: 'Num4', 0x5d: 'Num5',
  0x5e: 'Num6', 0x5f: 'Num7', 0x60: 'Num8', 0x61: 'Num9', 0x62: 'Num0',
  0x63: 'Num.',
  0x65: 'App',
  0x87: '\\',  // 日语键盘
  0x88: 'かな', // 日语假名
  0x89: '¥',
  0x8a: '変換',
  0x8b: '無変換',
};

/** JS event.code 到 HID 键码映射 */
export const CODE_TO_HID: Record<string, number> = {
  'KeyA': 0x04, 'KeyB': 0x05, 'KeyC': 0x06, 'KeyD': 0x07, 'KeyE': 0x08, 'KeyF': 0x09,
  'KeyG': 0x0a, 'KeyH': 0x0b, 'KeyI': 0x0c, 'KeyJ': 0x0d, 'KeyK': 0x0e, 'KeyL': 0x0f,
  'KeyM': 0x10, 'KeyN': 0x11, 'KeyO': 0x12, 'KeyP': 0x13, 'KeyQ': 0x14, 'KeyR': 0x15,
  'KeyS': 0x16, 'KeyT': 0x17, 'KeyU': 0x18, 'KeyV': 0x19, 'KeyW': 0x1a, 'KeyX': 0x1b,
  'KeyY': 0x1c, 'KeyZ': 0x1d,
  'Digit1': 0x1e, 'Digit2': 0x1f, 'Digit3': 0x20, 'Digit4': 0x21, 'Digit5': 0x22,
  'Digit6': 0x23, 'Digit7': 0x24, 'Digit8': 0x25, 'Digit9': 0x26, 'Digit0': 0x27,
  'Enter': 0x28, 'Escape': 0x29, 'Backspace': 0x2a, 'Tab': 0x2b, 'Space': 0x2c,
  'Minus': 0x2d, 'Equal': 0x2e, 'BracketLeft': 0x2f, 'BracketRight': 0x30, 'Backslash': 0x31,
  'Semicolon': 0x33, 'Quote': 0x34, 'Backquote': 0x35, 'Comma': 0x36, 'Period': 0x37, 'Slash': 0x38,
  'CapsLock': 0x39,
  'F1': 0x3a, 'F2': 0x3b, 'F3': 0x3c, 'F4': 0x3d, 'F5': 0x3e, 'F6': 0x3f,
  'F7': 0x40, 'F8': 0x41, 'F9': 0x42, 'F10': 0x43, 'F11': 0x44, 'F12': 0x45,
  'PrintScreen': 0x46, 'ScrollLock': 0x47, 'Pause': 0x48,
  'Insert': 0x49, 'Home': 0x4a, 'PageUp': 0x4b,
  'Delete': 0x4c, 'End': 0x4d, 'PageDown': 0x4e,
  'ArrowRight': 0x4f, 'ArrowLeft': 0x50, 'ArrowDown': 0x51, 'ArrowUp': 0x52,
  'NumLock': 0x53, 'NumpadDivide': 0x54, 'NumpadMultiply': 0x55, 'NumpadSubtract': 0x56,
  'NumpadAdd': 0x57, 'NumpadEnter': 0x58,
  'Numpad1': 0x59, 'Numpad2': 0x5a, 'Numpad3': 0x5b, 'Numpad4': 0x5c, 'Numpad5': 0x5d,
  'Numpad6': 0x5e, 'Numpad7': 0x5f, 'Numpad8': 0x60, 'Numpad9': 0x61, 'Numpad0': 0x62,
  'NumpadDecimal': 0x63,
  'ContextMenu': 0x65,
  'IntlBackslash': 0x64,
  'IntlRo': 0x87,
  'KanaMode': 0x88,
  'IntlYen': 0x89,
  'Convert': 0x8a,
  'NonConvert': 0x8b,
};

/** 获取键码显示名称 */
export function getKeycodeName(keycode: number, modifier = 0): string {
  const parts: string[] = [];

  // 修饰键
  if (modifier & Modifier.LCTRL) parts.push('Ctrl');
  if (modifier & Modifier.LSHIFT) parts.push('Shift');
  if (modifier & Modifier.LALT) parts.push('Alt');
  if (modifier & Modifier.LGUI) parts.push('Win');
  if (modifier & Modifier.RCTRL) parts.push('RCtrl');
  if (modifier & Modifier.RSHIFT) parts.push('RShift');
  if (modifier & Modifier.RALT) parts.push('RAlt');
  if (modifier & Modifier.RGUI) parts.push('RWin');

  // 键码
  const keyName = KEYCODE_NAMES[keycode];
  if (keyName) {
    parts.push(keyName);
  } else if (keycode > 0) {
    parts.push(`0x${keycode.toString(16).toUpperCase()}`);
  }

  return parts.join('+') || '—';
}

/** 从 KeyboardEvent 获取 HID 键码 */
export function getHidFromEvent(event: KeyboardEvent): { keycode: number; modifier: number } {
  let modifier = 0;
  if (event.ctrlKey && !event.code.includes('Control')) {
    modifier |= event.location === 2 ? Modifier.RCTRL : Modifier.LCTRL;
  }
  if (event.shiftKey && !event.code.includes('Shift')) {
    modifier |= event.location === 2 ? Modifier.RSHIFT : Modifier.LSHIFT;
  }
  if (event.altKey && !event.code.includes('Alt')) {
    modifier |= event.location === 2 ? Modifier.RALT : Modifier.LALT;
  }
  if (event.metaKey && !event.code.includes('Meta')) {
    modifier |= event.location === 2 ? Modifier.RGUI : Modifier.LGUI;
  }

  const keycode = CODE_TO_HID[event.code] || 0;

  return { keycode, modifier };
}

/** 获取所有可用键码列表 */
export function getAllKeycodes(): { code: number; name: string }[] {
  return Object.entries(KEYCODE_NAMES)
    .filter(([code, name]) => Number(code) > 0 && name)
    .map(([code, name]) => ({ code: Number(code), name }));
}
