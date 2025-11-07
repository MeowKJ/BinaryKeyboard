import { KeyboardType } from "@/types";

export const KEYBOARD_FILTER: HIDDeviceFilter = {
  vendorId: 0x1209,
  productId: 0xc55d,
};

export const REPORT_ID_SEND = 4; // 发送数据的报告 ID
export const REPORT_ID_RECEIVE = 5; // 接收数据的报告 ID

export const CMD_KEYBOARD_ACK = 0x01; // 键盘确认（ACK）
export const CMD_KEYBOARD_SEND = 0x02; // 键盘发送数据

export const CMD_HOST_REQUEST = 0x03; // 主机请求数据
export const CMD_HOST_SEND = 0x04; // 主机发送数据

// export const BASIC_KEY_COUNT = 4; // 基础键盘可配置按键数量
// export const KNOB_KEY_COUNT = 7; // 旋钮键盘可配置按键数量
// export const FIVE_KEY_COUNT = 5; // 五键键盘可配置按键数量

export const KeyboardKeyCount = {
  [KeyboardType.BASIC]: 4,
  [KeyboardType.KNOB]: 7,
  [KeyboardType.FIVE_KEYS]: 5,
};
