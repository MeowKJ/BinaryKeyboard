type ButtonType = "circle" | "square" | "horizontal-bar" | "vertical-bar";

/** 描述键盘状态的类型 */
export interface KeyboardConfig {
  key: string; // 原始按键字符（从 HID 数据无法还原时，可置为空）
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

/** 按键映射类型定义：共三种类型 */
export type KeyMapping =
  | { type: 0x01; modifier: number; key: number } // 键盘
  | { type: 0x02; mediaData: number } // 媒体
  | { type: 0x03; mouseData: number }; // 鼠标

/** 原始 HID 数据类型：包含一个8位修饰键编码和一个8位 HID 键码 */
export interface RawHIDData {
  modifier: number; // 8 位修饰键编码
  hid: number; // 8 位 HID 键码
}

/** 修饰键的 HID 标准编码（8位，每一位代表一个修饰键）
 *  通常：
 *    bit0: LeftCtrl
 *    bit1: LeftShift
 *    bit2: LeftAlt
 *    bit3: LeftMeta
 *    bit4: RightCtrl
 *    bit5: RightShift
 *    bit6: RightAlt
 *    bit7: RightMeta
 */
export enum Modifier {
  LeftCtrl = 0x01,
  LeftShift = 0x02,
  LeftAlt = 0x04,
  LeftMeta = 0x08,
  RightCtrl = 0x10,
  RightShift = 0x20,
  RightAlt = 0x40,
  RightMeta = 0x80,
}
