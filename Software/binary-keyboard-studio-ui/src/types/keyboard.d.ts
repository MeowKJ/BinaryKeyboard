type ButtonType = "circle" | "square" | "horizontal-bar" | "vertical-bar";

interface KeyboardConfig {
  key: string; // 原始按键字符（无法通过 HID 完全还原时置为空）
  code: string; // 对应 event.code
  leftMetaKey: boolean;
  leftCtrlKey: boolean;
  leftAltKey: boolean;
  leftShiftKey: boolean;
  rightMetaKey: boolean;
  rightCtrlKey: boolean;
  rightAltKey: boolean;
  rightShiftKey: boolean;
}

/** 原始 HID 数据类型：包含一个8位修饰键编码和一个8位 HID 编码 */
interface KeyBoardRawHIDData {
  modifier: number; // 8 位修饰键编码
  hid: number; // 8 位 HID 键码
}

interface MouseRawHIDData {
  button: MouseButtonHID; // 鼠标按键
  wheel: MouseWheelUnit; // 滚轮操作
}

interface KeyboardConfig {
  key: string; // 原始按键字符（无法通过 HID 完全还原时置为空）
  code: string; // 对应 event.code
  leftMetaKey: boolean;
  leftCtrlKey: boolean;
  leftAltKey: boolean;
  leftShiftKey: boolean;
  rightMetaKey: boolean;
  rightCtrlKey: boolean;
  rightAltKey: boolean;
  rightShiftKey: boolean;
}

interface MediaConfig {
  key: MediaHIDCode;
}

type KeyMapping =
  | { type: 1; value: KeyboardConfig }
  | { type: 2; value: MediaConfig }
  | { type: 3; value: MouseConfig };
