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

export const KEYBOARD_MODEL = {
  UNKNOWN: 0,
  BASIC: 1,
  KNOB: 2,
  FIVE_KEY: 3,
};

export const KEY_TYPE_KETBOARD = 0x01; // 键盘
export const KEY_TYPE_MEDIA = 0x02; // 媒体
export const KEY_TYPE_MOUSE = 0x03; // 鼠标
