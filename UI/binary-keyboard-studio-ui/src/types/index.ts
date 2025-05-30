export type ButtonType =
  | "circle"
  | "square"
  | "horizontal-bar"
  | "vertical-bar";

export enum KeyboardType {
  BASIC = 0x01,
  KNOB = 0x02,
  FIVE_KEYS = 0x03,
}

export enum KeyType {
  KEBOARD = 0x01,
  MEDIA = 0x02,
  MOUSE = 0x03,
}

export interface KeyboardConfig {
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
  str?: string;
}

/** 原始 HID 数据类型：包含一个8位修饰键编码和一个8位 HID 编码 */
export interface KeyBoardRawHIDData {
  modifier: number; // 8 位修饰键编码
  hid: number; // 8 位 HID 键码
}

export interface MediaConfig {
  key: MediaHIDCode;
}

export interface MouseConfig {
  button: MouseButtonHID;
  wheel: number;
}

export type KeyMapping =
  | { type: 1; value: KeyboardConfig }
  | { type: 2; value: MediaConfig }
  | { type: 3; value: MouseConfig };

export interface ComparedKeyMappingString {
  index: number;
  oldTypeString: string;
  oldValue: string;
  newTypeString: string;
  newValue: string;
}

export enum MouseButtonHID {
  None = 0x00, // 无按键
  LeftButton = 0x01, // 左键
  RightButton = 0x02, // 右键
  MiddleButton = 0x04, // 中键
  BackButton = 0x08, // 侧键1（后退）
  ForwardButton = 0x10, // 侧键2（前进）
}

export enum MediaHIDCode {
  // 多媒体控制
  None = 0x00, // 无按键
  Play = 0xb0, // 播放
  Pause = 0xb1, // 暂停
  PlayPause = 0xcd, // 播放/暂停
  Stop = 0xb7, // 停止
  NextTrack = 0xb5, // 下一曲
  PrevTrack = 0xb6, // 上一曲
  FastForward = 0xb3, // 快进
  Rewind = 0xb4, // 倒带

  // 音量控制
  Mute = 0xe2, // 静音
  VolumeUp = 0xe9, // 音量增加
  VolumeDown = 0xea, // 音量减少

  // 频道控制（适用于电视/机顶盒）
  ChannelUp = 0x9c, // 频道增加
  ChannelDown = 0x9d, // 频道减少

  // 设备电源控制
  Power = 0x30, // 设备电源
  Sleep = 0x32, // 休眠
  WakeUp = 0x33, // 唤醒

  // 设备菜单/导航
  Menu = 0x40, // 进入菜单
  Home = 0x223, // 主页
  Back = 0x224, // 返回
  Exit = 0x94, // 退出
  Select = 0x41, // 确认/选择
  Up = 0x42, // 向上
  Down = 0x43, // 向下
  Left = 0x44, // 向左
  Right = 0x45, // 向右

  // 应用程序快捷键
  Calculator = 0x192, // 计算器
  FileExplorer = 0x194, // 文件资源管理器

  BrowserForward = 0x225, // 浏览器前进
  BrowserRefresh = 0x227, // 刷新
  BrowserFavorites = 0x22a, // 收藏夹
  Email = 0x18a, // 电子邮件应用
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
export enum KeyboardKeyModifier {
  LeftCtrl = 0x01,
  LeftShift = 0x02,
  LeftAlt = 0x04,
  LeftMeta = 0x08,
  RightCtrl = 0x10,
  RightShift = 0x20,
  RightAlt = 0x40,
  RightMeta = 0x80,
}
