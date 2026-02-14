/**
 * HID 协议帧解析与翻译
 * 将原始字节解码为人类可读格式
 */

import {
  Command,
  ResponseCode,
  ActionType,
  RgbMode,
  FnAction,
  KeyboardType,
  LayerOp,
  MouseButton,
  WheelDirection,
  LogCategory,
  SystemLogEvent,
} from '@/types/protocol';
import { getKeycodeName } from '@/utils/keycodes';

// ============================================================================
// 命令名称映射
// ============================================================================

const COMMAND_NAMES: Record<number, string> = {
  [Command.SYS_INFO]: 'SYS_INFO',
  [Command.SYS_STATUS]: 'SYS_STATUS',
  [Command.CFG_SAVE]: 'CFG_SAVE',
  [Command.CFG_LOAD]: 'CFG_LOAD',
  [Command.CFG_RESET]: 'CFG_RESET',
  [Command.KEYMAP_GET]: 'KEYMAP_GET',
  [Command.KEYMAP_SET]: 'KEYMAP_SET',
  [Command.LAYER_GET]: 'LAYER_GET',
  [Command.LAYER_SET]: 'LAYER_SET',
  [Command.RGB_GET]: 'RGB_GET',
  [Command.RGB_SET]: 'RGB_SET',
  [Command.MACRO_INFO]: 'MACRO_INFO',
  [Command.MACRO_GET]: 'MACRO_GET',
  [Command.MACRO_SET]: 'MACRO_SET',
  [Command.MACRO_DEL]: 'MACRO_DEL',
  [Command.FNKEY_GET]: 'FNKEY_GET',
  [Command.FNKEY_SET]: 'FNKEY_SET',
  [Command.BATTERY]: 'BATTERY',
  [Command.LOG]: 'LOG',
  [Command.LOG_GET]: 'LOG_GET',
  [Command.LOG_SET]: 'LOG_SET',
};

const COMMAND_LABELS: Record<number, string> = {
  [Command.SYS_INFO]: '获取设备信息',
  [Command.SYS_STATUS]: '获取系统状态',
  [Command.CFG_SAVE]: '保存配置到 Flash',
  [Command.CFG_LOAD]: '从 Flash 加载配置',
  [Command.CFG_RESET]: '恢复出厂设置',
  [Command.KEYMAP_GET]: '获取按键映射',
  [Command.KEYMAP_SET]: '设置按键映射',
  [Command.LAYER_GET]: '获取当前层',
  [Command.LAYER_SET]: '设置当前层',
  [Command.RGB_GET]: '获取 RGB 配置',
  [Command.RGB_SET]: '设置 RGB 配置',
  [Command.MACRO_INFO]: '获取宏信息',
  [Command.MACRO_GET]: '读取宏数据',
  [Command.MACRO_SET]: '写入宏数据',
  [Command.MACRO_DEL]: '删除宏',
  [Command.FNKEY_GET]: '获取 FN 键配置',
  [Command.FNKEY_SET]: '设置 FN 键配置',
  [Command.BATTERY]: '获取电池信息',
  [Command.LOG]: '设备日志',
  [Command.LOG_GET]: '获取日志配置',
  [Command.LOG_SET]: '设置日志配置',
};

const RESPONSE_CODE_NAMES: Record<number, string> = {
  [ResponseCode.OK]: 'OK',
  [ResponseCode.ERR_INVALID]: 'ERR_INVALID',
  [ResponseCode.ERR_PARAM]: 'ERR_PARAM',
  [ResponseCode.ERR_BUSY]: 'ERR_BUSY',
  [ResponseCode.ERR_FLASH]: 'ERR_FLASH',
  [ResponseCode.ERR_TOO_LARGE]: 'ERR_TOO_LARGE',
  [ResponseCode.ERR_NO_SPACE]: 'ERR_NO_SPACE',
  [ResponseCode.ERR_NOT_FOUND]: 'ERR_NOT_FOUND',
};

const RESPONSE_CODE_LABELS: Record<number, string> = {
  [ResponseCode.OK]: '成功',
  [ResponseCode.ERR_INVALID]: '无效命令',
  [ResponseCode.ERR_PARAM]: '参数错误',
  [ResponseCode.ERR_BUSY]: '设备忙',
  [ResponseCode.ERR_FLASH]: 'Flash 操作失败',
  [ResponseCode.ERR_TOO_LARGE]: '数据过大',
  [ResponseCode.ERR_NO_SPACE]: '存储空间不足',
  [ResponseCode.ERR_NOT_FOUND]: '未找到目标',
};

const ACTION_TYPE_NAMES: Record<number, string> = {
  [ActionType.NONE]: '无',
  [ActionType.KEYBOARD]: '键盘',
  [ActionType.MOUSE_BTN]: '鼠标按键',
  [ActionType.MOUSE_WHEEL]: '鼠标滚轮',
  [ActionType.CONSUMER]: '多媒体',
  [ActionType.MACRO]: '宏',
  [ActionType.LAYER]: '层切换',
};

const RGB_MODE_NAMES: Record<number, string> = {
  [RgbMode.OFF]: '关闭',
  [RgbMode.STATIC]: '静态',
  [RgbMode.BREATHING]: '呼吸',
  [RgbMode.BLINK]: '闪烁',
  [RgbMode.RAINBOW]: '彩虹',
  [RgbMode.INDICATOR]: '仅指示灯',
};

const KEYBOARD_TYPE_NAMES: Record<number, string> = {
  [KeyboardType.BASIC]: '基础款',
  [KeyboardType.FIVE_KEYS]: '五键款',
  [KeyboardType.KNOB]: '旋钮款',
};

const FN_ACTION_NAMES: Record<number, string> = {
  [FnAction.NONE]: '无',
  [FnAction.MODE_TOGGLE]: '切换模式',
  [FnAction.BLE_ADV]: '蓝牙广播',
  [FnAction.BLE_DISCONNECT]: '蓝牙断开',
  [FnAction.BLE_CLEAR_BONDS]: '清除配对',
  [FnAction.RGB_TOGGLE]: 'RGB 开关',
  [FnAction.RGB_MODE_NEXT]: 'RGB 下一模式',
  [FnAction.RGB_MODE_PREV]: 'RGB 上一模式',
  [FnAction.RGB_BRIGHT_UP]: '亮度+',
  [FnAction.RGB_BRIGHT_DOWN]: '亮度-',
  [FnAction.LAYER_NEXT]: '下一层',
  [FnAction.LAYER_PREV]: '上一层',
  [FnAction.LAYER_SET]: '设置层',
  [FnAction.MACRO]: '执行宏',
  [FnAction.SLEEP]: '休眠',
  [FnAction.BOOTLOADER]: '进入 Bootloader',
};

const LAYER_OP_NAMES: Record<number, string> = {
  [LayerOp.MOMENTARY]: '按住',
  [LayerOp.TOGGLE]: '切换',
  [LayerOp.SET]: '设置',
};

// ============================================================================
// 工具函数
// ============================================================================

/** Uint8Array 转 hex 字符串 */
export function toHexDump(data: Uint8Array, maxBytes = 64): string {
  const bytes = Array.from(data.slice(0, maxBytes));
  return bytes.map(b => b.toString(16).padStart(2, '0').toUpperCase()).join(' ');
}

/** 单字节转 hex */
function hex(n: number): string {
  return '0x' + n.toString(16).padStart(2, '0').toUpperCase();
}

/** 格式化修饰键 */
function formatModifiers(mod: number): string {
  const parts: string[] = [];
  if (mod & 0x01) parts.push('LCtrl');
  if (mod & 0x02) parts.push('LShift');
  if (mod & 0x04) parts.push('LAlt');
  if (mod & 0x08) parts.push('LGui');
  if (mod & 0x10) parts.push('RCtrl');
  if (mod & 0x20) parts.push('RShift');
  if (mod & 0x40) parts.push('RAlt');
  if (mod & 0x80) parts.push('RGui');
  return parts.length ? parts.join('+') : '无';
}

/** 格式化按键动作 */
function formatKeyAction(type: number, mod: number, param1: number, param2: number): string {
  const typeName = ACTION_TYPE_NAMES[type] || `未知(${hex(type)})`;

  switch (type) {
    case ActionType.NONE:
      return '无动作';
    case ActionType.KEYBOARD: {
      const keyName = getKeycodeName(param1, mod);
      return `键盘: ${keyName}`;
    }
    case ActionType.MOUSE_BTN: {
      const btns: string[] = [];
      if (param1 & MouseButton.LEFT) btns.push('左键');
      if (param1 & MouseButton.RIGHT) btns.push('右键');
      if (param1 & MouseButton.MIDDLE) btns.push('中键');
      if (param1 & MouseButton.BACK) btns.push('后退');
      if (param1 & MouseButton.FORWARD) btns.push('前进');
      return `鼠标按键: ${btns.join('+')}`;
    }
    case ActionType.MOUSE_WHEEL: {
      const dir = param1 === WheelDirection.UP ? '上' : param1 === WheelDirection.DOWN ? '下' : param1 === WheelDirection.CLICK ? '点击' : `未知(${param1})`;
      return `鼠标滚轮: ${dir}`;
    }
    case ActionType.CONSUMER: {
      const usageId = param1 | (param2 << 8);
      return `多媒体键: ${hex(usageId)}`;
    }
    case ActionType.MACRO:
      return `执行宏 #${param1}`;
    case ActionType.LAYER: {
      const op = LAYER_OP_NAMES[mod] || `未知(${mod})`;
      return `层切换: ${op} → 层${param1 + 1}`;
    }
    default:
      return `${typeName}: mod=${hex(mod)} p1=${hex(param1)} p2=${hex(param2)}`;
  }
}

// ============================================================================
// 发送帧解析
// ============================================================================

export function getCommandName(cmd: number): string {
  return COMMAND_NAMES[cmd] || `UNKNOWN_${hex(cmd)}`;
}

export function getCommandLabel(cmd: number): string {
  return COMMAND_LABELS[cmd] || `未知命令 ${hex(cmd)}`;
}

/** 解析发送帧为人类可读文本 */
export function parseSendFrame(frame: Uint8Array): {
  command: string;
  cmdHex: string;
  sub: number;
  dataLen: number;
  rawHex: string;
  parsed: string;
} {
  const cmd = frame[0];
  const sub = frame[1];
  const len = frame[2];
  const data = frame.slice(3, 3 + len);

  const command = getCommandName(cmd);
  const label = getCommandLabel(cmd);
  const rawHex = toHexDump(frame, 3 + len);
  let parsed = `${label}`;

  // 详细解析各命令
  switch (cmd) {
    case Command.KEYMAP_GET:
      parsed += ` | 层 ${sub + 1}`;
      break;

    case Command.KEYMAP_SET: {
      parsed += ` | 层 ${sub + 1}`;
      if (len >= 35) {
        const numLayers = data[0];
        const defaultLayer = data[2];
        parsed += ` | 总层数=${numLayers}, 默认层=${defaultLayer + 1}`;
        // 解析按键
        const keys: string[] = [];
        for (let i = 0; i < 8 && (3 + i * 4) < len; i++) {
          const offset = 3 + i * 4;
          const t = data[offset], m = data[offset + 1], p1 = data[offset + 2], p2 = data[offset + 3];
          if (t !== ActionType.NONE) {
            keys.push(`K${i}=[${formatKeyAction(t, m, p1, p2)}]`);
          }
        }
        if (keys.length) parsed += ` | ${keys.join(', ')}`;
      }
      break;
    }

    case Command.LAYER_SET:
      parsed += ` | 切换到层 ${sub + 1}`;
      break;

    case Command.RGB_SET:
      if (len >= 9) {
        const enabled = data[0] ? '开' : '关';
        const mode = RGB_MODE_NAMES[data[1]] || `未知(${data[1]})`;
        const brightness = Math.round(data[2] * 100 / 255);
        const r = data[4], g = data[5], b = data[6];
        parsed += ` | ${enabled}, 模式=${mode}, 亮度=${brightness}%, 颜色=#${r.toString(16).padStart(2, '0')}${g.toString(16).padStart(2, '0')}${b.toString(16).padStart(2, '0')}`;
      }
      break;

    case Command.FNKEY_SET:
      if (len >= 32) {
        const fnDescs: string[] = [];
        for (let i = 0; i < 4; i++) {
          const offset = i * 8;
          const click = FN_ACTION_NAMES[data[offset]] || hex(data[offset]);
          const long = FN_ACTION_NAMES[data[offset + 2]] || hex(data[offset + 2]);
          if (data[offset] !== 0 || data[offset + 2] !== 0) {
            fnDescs.push(`FN${i + 1}[单击=${click}, 长按=${long}]`);
          }
        }
        if (fnDescs.length) parsed += ` | ${fnDescs.join(', ')}`;
      }
      break;

    case Command.MACRO_SET: {
      const seq = len > 0 ? data[0] : -1;
      if (seq === 0) parsed += ` | 槽位 ${sub}, 开始写入`;
      else if (seq === 0xFF) parsed += ` | 槽位 ${sub}, 写入完成`;
      else parsed += ` | 槽位 ${sub}, 块 #${seq}`;
      break;
    }

    case Command.MACRO_DEL:
      parsed += ` | 删除槽位 ${sub}`;
      break;
  }

  return { command, cmdHex: cmd.toString(16).padStart(2, '0'), sub, dataLen: len, rawHex, parsed };
}

// ============================================================================
// 响应帧解析
// ============================================================================

/** 解析响应帧为人类可读文本 */
export function parseReceiveFrame(frame: Uint8Array): {
  command: string;
  cmdHex: string;
  sub: number;
  dataLen: number;
  rawHex: string;
  parsed: string;
  statusCode: string;
  isError: boolean;
} {
  const cmdByte = frame[0];
  const sub = frame[1];
  const len = frame[2];
  const data = frame.slice(3, 3 + Math.max(len, 1));

  // 响应帧 CMD 可能是原始命令码直接回传
  const status = data[0];
  const statusName = RESPONSE_CODE_NAMES[status] || hex(status);
  const statusLabel = RESPONSE_CODE_LABELS[status] || '未知状态';
  const isError = status !== ResponseCode.OK;

  // 响应的命令码通常与请求相同
  const command = getCommandName(cmdByte);
  const label = getCommandLabel(cmdByte);
  const rawHex = toHexDump(frame, 3 + len);

  let parsed = `${label} → ${statusName}(${statusLabel})`;

  if (isError) {
    return {
      command, cmdHex: cmdByte.toString(16).padStart(2, '0'),
      sub, dataLen: len, rawHex, parsed, statusCode: statusName, isError,
    };
  }

  // 详细解析响应数据
  switch (cmdByte) {
    case Command.SYS_INFO:
      if (len >= 14) {
        const vid = (data[1] << 8) | data[2];
        const pid = (data[3] << 8) | data[4];
        const ver = `${data[5]}.${data[6]}.${data[7]}`;
        const maxLayers = data[8];
        const maxKeys = data[9];
        const macros = data[10];
        const kbType = KEYBOARD_TYPE_NAMES[data[11]] || `未知(${data[11]})`;
        const keyCount = data[12];
        const fnCount = data[13];
        parsed += ` | VID=${hex(vid)} PID=${hex(pid)} v${ver} | ${kbType} ${keyCount}键 ${fnCount}FN | ${maxLayers}层 ${macros}宏`;
      }
      break;

    case Command.SYS_STATUS:
      if (len >= 6) {
        const mode = data[1] === 0 ? 'USB' : 'BLE';
        const connState = data[2];
        let conn = '未连接';
        if (mode === 'USB') {
          conn = '已连接';
        } else if (connState === 2) {
          conn = '已连接';
        } else if (connState === 1) {
          conn = '广播中';
        } else if (connState === 3) {
          conn = '挂起';
        }
        const layer = data[3];
        const battery = data[4];
        const charging = data[5] ? '充电中' : '未充电';
        parsed += ` | ${mode} ${conn} | 层${layer + 1} | 电量 ${battery}% ${charging}`;
      }
      break;

    case Command.KEYMAP_GET:
      if (len >= 36) {
        const numLayers = data[1];
        const curLayer = data[2];
        const defLayer = data[3];
        parsed += ` | 层 ${sub + 1}/${numLayers} (当前=${curLayer + 1}, 默认=${defLayer + 1})`;
        const keys: string[] = [];
        for (let i = 0; i < 8 && (4 + i * 4) < len; i++) {
          const offset = 4 + i * 4;
          const t = data[offset], m = data[offset + 1], p1 = data[offset + 2], p2 = data[offset + 3];
          if (t !== ActionType.NONE) {
            keys.push(`K${i}=[${formatKeyAction(t, m, p1, p2)}]`);
          }
        }
        if (keys.length) parsed += ` | ${keys.join(', ')}`;
      }
      break;

    case Command.RGB_GET:
      if (len >= 9) {
        const enabled = data[1] ? '开' : '关';
        const mode = RGB_MODE_NAMES[data[2]] || `未知(${data[2]})`;
        const brightness = Math.round(data[3] * 100 / 255);
        const r = data[5], g = data[6], b = data[7];
        const indicator = data[8] ? '指示灯开' : '指示灯关';
        parsed += ` | ${enabled}, ${mode}, 亮度=${brightness}%, #${r.toString(16).padStart(2, '0')}${g.toString(16).padStart(2, '0')}${b.toString(16).padStart(2, '0')}, ${indicator}`;
      }
      break;

    case Command.FNKEY_GET:
      if (len >= 33) {
        const fnDescs: string[] = [];
        for (let i = 0; i < 4; i++) {
          const offset = 1 + i * 8;
          const click = FN_ACTION_NAMES[data[offset]] || hex(data[offset]);
          const long = FN_ACTION_NAMES[data[offset + 2]] || hex(data[offset + 2]);
          const longMs = data[offset + 4] | (data[offset + 5] << 8);
          if (data[offset] !== 0 || data[offset + 2] !== 0) {
            fnDescs.push(`FN${i + 1}[单击=${click}, 长按=${long}, ${longMs}ms]`);
          }
        }
        if (fnDescs.length) parsed += ` | ${fnDescs.join(', ')}`;
      }
      break;

    case Command.BATTERY:
      if (len >= 4) {
        const battery = data[1];
        const charging = data[2] ? '充电中' : '未充电';
        const voltage = (data[3] / 10).toFixed(1);
        parsed += ` | ${battery}% ${charging} ${voltage}V`;
      }
      break;

    case Command.LAYER_GET:
      if (len >= 3) {
        parsed += ` | 当前层=${data[1] + 1}, 默认层=${data[2] + 1}`;
      }
      break;
  }

  return {
    command, cmdHex: cmdByte.toString(16).padStart(2, '0'),
    sub, dataLen: len, rawHex, parsed, statusCode: statusName, isError,
  };
}

// ============================================================================
// 固件日志帧解析 (CMD=0x70)
// ============================================================================

const LOG_CATEGORY_NAMES: Record<number, string> = {
  [LogCategory.KEY_EVENT]: 'KEY',
  [LogCategory.FN_EVENT]: 'FN',
  [LogCategory.LAYER_EVENT]: 'LAYER',
  [LogCategory.MODE_EVENT]: 'MODE',
  [LogCategory.BLE_EVENT]: 'BLE',
  [LogCategory.RGB_EVENT]: 'RGB',
  [LogCategory.SYSTEM_EVENT]: 'SYSTEM',
};

const LOG_CATEGORY_LABELS: Record<number, string> = {
  [LogCategory.KEY_EVENT]: '按键事件',
  [LogCategory.FN_EVENT]: 'FN 键事件',
  [LogCategory.LAYER_EVENT]: '层切换',
  [LogCategory.MODE_EVENT]: '模式切换',
  [LogCategory.BLE_EVENT]: '蓝牙事件',
  [LogCategory.RGB_EVENT]: 'RGB 事件',
  [LogCategory.SYSTEM_EVENT]: '系统事件',
};

type LogCategoryKey = 'key' | 'fn' | 'layer' | 'mode' | 'ble' | 'rgb' | 'system';

const LOG_CATEGORY_KEYS: Record<number, LogCategoryKey> = {
  [LogCategory.KEY_EVENT]: 'key',
  [LogCategory.FN_EVENT]: 'fn',
  [LogCategory.LAYER_EVENT]: 'layer',
  [LogCategory.MODE_EVENT]: 'mode',
  [LogCategory.BLE_EVENT]: 'ble',
  [LogCategory.RGB_EVENT]: 'rgb',
  [LogCategory.SYSTEM_EVENT]: 'system',
};

function bleStateName(state: number): string {
  switch (state) {
    case 0: return 'DISCONNECTED';
    case 1: return 'ADVERTISING';
    case 2: return 'CONNECTED';
    case 3: return 'SUSPENDED';
    default: return hex(state);
  }
}

function bleOpcodeName(opcode: number): string {
  const names: Record<number, string> = {
    0x00: 'GAP_DEVICE_INIT_DONE',
    0x01: 'GAP_DEVICE_DISCOVERY',
    0x02: 'GAP_ADV_DATA_UPDATE_DONE',
    0x03: 'GAP_MAKE_DISCOVERABLE_DONE',
    0x04: 'GAP_END_DISCOVERABLE_DONE',
    0x05: 'GAP_LINK_ESTABLISHED',
    0x06: 'GAP_LINK_TERMINATED',
    0x07: 'GAP_LINK_PARAM_UPDATE',
    0x08: 'GAP_RANDOM_ADDR_CHANGED',
    0x09: 'GAP_SIGNATURE_UPDATED',
    0xA0: 'SNV_WRITE_ENTER',
    0xA1: 'SNV_READ_DONE',
    0xA2: 'SNV_MERGE_DONE',
    0xA3: 'SNV_ERASE_DONE',
    0xA4: 'SNV_WRITE_DONE',
    0xA5: 'SNV_WRITE_EXIT',
    0xA9: 'SNV_WRITE_REJECT',
    0xB0: 'PAIRING_STARTED',
    0xB1: 'PAIRING_COMPLETE',
    0xB2: 'PAIRING_BONDED',
    0xB3: 'PAIRING_BOND_SAVED',
    0xB4: 'PASSCODE_REQUEST',
    0xB5: 'PASSCODE_RESPONSE_SENT',
    0xB6: 'CCCD_WRITE',
    0xB7: 'SECURITY_REQUEST',
    0xB8: 'SECURITY_REQUEST_RETRY',
    0xBB: 'SECURITY_REQUEST_CALL',
    0xBC: 'SECURITY_REQUEST_SKIP',
    0xB9: 'PAIR_CB_ENTER',
    0xBA: 'PAIR_CB_EXIT',
    0xBD: 'GAP_MSG_ENTER',
    0xBE: 'GAP_MSG_EXIT',
    0xC8: 'STATE_CB_ENTER',
    0xC9: 'STATE_CB_EXIT',
    0xC0: 'PEER_ADDR_PART0',
    0xC1: 'PEER_ADDR_PART1',
    0xC2: 'PEER_ADDR_PART2',
    0xD0: 'HID_NOTIFY_ENABLED',
    0xD1: 'HID_READY_TIMEOUT',
    0xD2: 'HID_NOTIFY_TOGGLE',
    0xE1: 'BLE_SEND_KEYBOARD',
    0xE2: 'BLE_SEND_MOUSE',
    0xE3: 'BLE_SEND_CONSUMER',
    0xF0: 'MAIN_HEARTBEAT',
    0xF1: 'MAIN_SLOW_STAGE',
  };
  return names[opcode] || '';
}

function bleReasonName(reason: number, opcode?: number): string {
  // 0x13 在不同上下文语义不同：
  // - GAP_LINK_TERMINATED: REMOTE_USER_TERMINATED
  // - 自定义发送诊断: BLE_MEM_ALLOC_ERROR
  if (reason === 0x13) {
    if (opcode === 0x06) return 'REMOTE_USER_TERMINATED';
    if (opcode === 0xE1 || opcode === 0xE2 || opcode === 0xE3) return 'BLE_MEM_ALLOC_ERROR';
  }

  if (opcode === 0xB6) {
    const cccdNames: Record<number, string> = {
      0x00: 'CCCD_DISABLE',
      0x01: 'CCCD_NOTIFY',
      0x02: 'CCCD_INDICATE',
    };
    return cccdNames[reason] || '';
  }

  if (opcode === 0xB5) {
    const rspNames: Record<number, string> = {
      0x00: 'SUCCESS',
      0xFE: 'APP_DELEGATED',
    };
    return rspNames[reason] || '';
  }

  if (opcode === 0xB7) {
    const secReqNames: Record<number, string> = {
      0x00: 'SUCCESS',
      0x10: 'BLE_NOT_READY',
      0x12: 'BLE_NO_RESOURCES',
      0x17: 'BLE_TIMEOUT',
      0x18: 'BLE_INVALID_RANGE',
      0x1A: 'BLE_INVALID_STATE',
    };
    return secReqNames[reason] || '';
  }

  if (opcode === 0xB8) {
    return `RETRY_${reason}`;
  }

  if (opcode === 0xBB) {
    return `CALL_RETRY_${reason}`;
  }

  if (opcode === 0xBC) {
    const skipNames: Record<number, string> = {
      0x01: 'SKIP_NOT_CONNECTED',
    };
    return skipNames[reason] || '';
  }

  if (opcode === 0xB9) {
    const pairStateNames: Record<number, string> = {
      0x00: 'PAIR_STATE_STARTED',
      0x01: 'PAIR_STATE_COMPLETE',
      0x02: 'PAIR_STATE_BONDED',
      0x03: 'PAIR_STATE_BOND_SAVED',
    };
    return pairStateNames[reason] || `PAIR_STATE_${reason}`;
  }

  if (opcode === 0xA9) {
    const rejectNames: Record<number, string> = {
      0x01: 'RANGE_INVALID',
    };
    return rejectNames[reason] || '';
  }

  if (opcode === 0xB0 || opcode === 0xB1 || opcode === 0xB2 || opcode === 0xB3 || opcode === 0xB4) {
    const pairNames: Record<number, string> = {
      0x00: 'SUCCESS',
      0x01: 'PAIR_FAIL_PASSKEY_ENTRY',
      0x02: 'PAIR_FAIL_OOB_NOT_AVAIL',
      0x03: 'PAIR_FAIL_AUTH_REQ',
      0x04: 'PAIR_FAIL_CONFIRM_VALUE',
      0x05: 'PAIR_FAIL_NOT_SUPPORTED',
      0x06: 'PAIR_FAIL_ENC_KEY_SIZE',
      0x07: 'PAIR_FAIL_CMD_NOT_SUPPORTED',
      0x08: 'PAIR_FAIL_UNSPECIFIED',
      0x09: 'PAIR_FAIL_REPEATED_ATTEMPTS',
      0x0A: 'PAIR_FAIL_INVALID_PARAMS',
      0x0B: 'PAIR_FAIL_DHKEY_CHECK',
      0x0C: 'PAIR_FAIL_NUMERIC_COMPARISON',
      0x0F: 'PAIR_FAIL_KEY_REJECTED',
    };
    return pairNames[reason] || '';
  }

  const names: Record<number, string> = {
    0x00: 'NO_ERROR',
    0x05: 'AUTHENTICATION_FAILURE',
    0x06: 'PIN_OR_KEY_MISSING',
    0x08: 'CONNECTION_TIMEOUT',
    0x14: 'REMOTE_LOW_RESOURCES',
    0x15: 'REMOTE_POWER_OFF',
    0x16: 'LOCAL_HOST_TERMINATED',
    0x1A: 'UNSUPPORTED_REMOTE_FEATURE',
    0x22: 'LMP_LL_RESPONSE_TIMEOUT',
    0x28: 'INSTANT_PASSED',
    0x3B: 'UNACCEPTABLE_CONN_PARAMS',
    0x10: 'BLE_NOT_READY',
    0x13: 'STATUS_0x13',
    0x17: 'BLE_TIMEOUT',
    0x18: 'BLE_INVALID_RANGE',
    0xFF: 'MODE_OR_STATE_REJECTED',
  };
  return names[reason] || '';
}

const blePeerAddrCache = {
  addrType: 0xFF,
  words: [-1, -1, -1] as [number, number, number],
};

function bleAddrTypeName(addrType: number): string {
  if (addrType === 0x00) return 'PUBLIC';
  if (addrType === 0x01) return 'RANDOM';
  if (addrType === 0x02) return 'PUBLIC_ID';
  if (addrType === 0x03) return 'RANDOM_ID';
  return hex(addrType);
}

function parsePeerAddrChunk(opcode: number, addrType: number, word: number): string {
  const idx = opcode - 0xC0;
  if (idx < 0 || idx > 2) return '';

  blePeerAddrCache.addrType = addrType;
  blePeerAddrCache.words[idx] = word;

  const typeText = bleAddrTypeName(addrType);
  if (blePeerAddrCache.words[0] >= 0 && blePeerAddrCache.words[1] >= 0 && blePeerAddrCache.words[2] >= 0) {
    const b0 = blePeerAddrCache.words[0] & 0xFF;
    const b1 = (blePeerAddrCache.words[0] >> 8) & 0xFF;
    const b2 = blePeerAddrCache.words[1] & 0xFF;
    const b3 = (blePeerAddrCache.words[1] >> 8) & 0xFF;
    const b4 = blePeerAddrCache.words[2] & 0xFF;
    const b5 = (blePeerAddrCache.words[2] >> 8) & 0xFF;
    const mac = [b5, b4, b3, b2, b1, b0]
      .map(v => v.toString(16).padStart(2, '0').toUpperCase())
      .join(':');
    return `peer=${mac} | type=${typeText}`;
  }

  return `peer_chunk${idx}=${word.toString(16).padStart(4, '0').toUpperCase()} | type=${typeText}`;
}

/** 解析固件日志帧 */
export function parseLogFrame(frame: Uint8Array): {
  command: string;
  cmdHex: string;
  sub: number;
  dataLen: number;
  rawHex: string;
  parsed: string;
  category: LogCategoryKey;
} {
  const sub = frame[1]; // category
  const len = frame[2];
  const data = frame.slice(3, 3 + len);

  const catName = LOG_CATEGORY_NAMES[sub] || `CAT_${sub}`;
  const catLabel = LOG_CATEGORY_LABELS[sub] || `未知类别(${sub})`;
  const category = LOG_CATEGORY_KEYS[sub] || 'system';
  const rawHex = toHexDump(frame, 3 + len);

  let parsed = catLabel;

  switch (sub) {
    case LogCategory.KEY_EVENT:
      if (len >= 4) {
        const keyIdx = data[0];
        const pressed = data[1] ? '按下' : '释放';
        const actType = ACTION_TYPE_NAMES[data[2]] || hex(data[2]);
        const param = data[3];
        const keyName = data[2] === ActionType.KEYBOARD ? getKeycodeName(param, 0) : hex(param);
        parsed += ` | 键${keyIdx} ${pressed} [${actType}: ${keyName}]`;
      }
      break;

    case LogCategory.FN_EVENT:
      if (len >= 4) {
        const fnId = data[0];
        const isLong = data[1] ? '长按' : '单击';
        const action = FN_ACTION_NAMES[data[2]] || hex(data[2]);
        parsed += ` | FN${fnId + 1} ${isLong} → ${action}`;
      }
      break;

    case LogCategory.LAYER_EVENT:
      if (len >= 2) {
        parsed += ` | 层${data[0] + 1} → 层${data[1] + 1}`;
      }
      break;

    case LogCategory.MODE_EVENT:
      if (len >= 2) {
        const oldMode = data[0] === 0 ? 'USB' : 'BLE';
        const newMode = data[1] === 0 ? 'USB' : 'BLE';
        parsed += ` | ${oldMode} → ${newMode}`;
      }
      break;

    case LogCategory.BLE_EVENT:
      if (len >= 5) {
        const stateName = bleStateName(data[0]);
        const opcode = data[1];
        const reason = data[2];
        const opName = bleOpcodeName(opcode);
        const rsName = bleReasonName(reason, opcode);
        const connHandle = data[3] | (data[4] << 8);
        const handleText = connHandle === 0xFFFF
          ? 'N/A'
          : ('0x' + connHandle.toString(16).padStart(4, '0').toUpperCase());
        const opText = opName ? `${hex(opcode)}(${opName})` : hex(opcode);
        const rsText = rsName ? `${hex(reason)}(${rsName})` : hex(reason);
        if (opcode === 0xBD) {
          const nestedName = bleOpcodeName(reason);
          const nestedText = nestedName ? `${hex(reason)}(${nestedName})` : hex(reason);
          parsed += ` | state=${stateName} | opcode=${opText} | gap=${nestedText} | handle=${handleText}`;
        } else if (opcode === 0xBE) {
          const statusName = bleReasonName(reason);
          const statusText = statusName ? `${hex(reason)}(${statusName})` : hex(reason);
          parsed += ` | state=${stateName} | opcode=${opText} | status=${statusText} | handle=${handleText}`;
        } else if (opcode >= 0xC0 && opcode <= 0xC2) {
          const peerText = parsePeerAddrChunk(opcode, reason, connHandle);
          parsed += ` | state=${stateName} | opcode=${opText} | ${peerText}`;
        } else if (opcode === 0xC8 || opcode === 0xC9) {
          const nestedName = bleOpcodeName(reason);
          const nestedText = nestedName ? `${hex(reason)}(${nestedName})` : hex(reason);
          parsed += ` | state=${stateName} | opcode=${opText} | event=${nestedText} | handle=${handleText}`;
        } else if (opcode === 0xF0) {
          const stage = reason & 0x0F;
          const mode = ((reason >> 4) & 0x01) ? 'BLE' : 'USB';
          const stageNames: Record<number, string> = {
            0: 'IDLE',
            1: 'TMOS',
            2: 'MODE',
            3: 'CORE',
            4: 'LOG',
          };
          const stageText = stageNames[stage] || `STAGE_${stage}`;
          parsed += ` | state=${stateName} | opcode=${opText} | mode=${mode} | stage=${stageText} | seq=${connHandle}`;
        } else if (opcode === 0xF1) {
          const stageNames: Record<number, string> = {
            1: 'TMOS',
            2: 'MODE',
            3: 'CORE',
            4: 'LOG',
          };
          const stageText = stageNames[reason] || `STAGE_${reason}`;
          parsed += ` | state=${stateName} | opcode=${opText} | stage=${stageText} | cost_ms=${connHandle}`;
        } else if (opcode === 0xD0) {
          parsed += ` | state=${stateName} | opcode=${opText} | report_id=${reason} | handle=${handleText}`;
        } else if (opcode === 0xD1) {
          parsed += ` | state=${stateName} | opcode=${opText} | detail=notify_not_enabled_in_time | handle=${handleText}`;
        } else if (opcode === 0xD2) {
          const operName = reason === 2 ? 'ENABLE' : reason === 3 ? 'DISABLE' : hex(reason);
          const reportId = connHandle & 0xFF;
          const reportType = (connHandle >> 8) & 0xFF;
          parsed += ` | state=${stateName} | opcode=${opText} | oper=${operName} | type=${hex(reportType)} | id=${reportId}`;
        } else if (opcode >= 0xA0 && opcode <= 0xA5) {
          if (opcode === 0xA0) {
            parsed += ` | state=${stateName} | opcode=${opText} | units=${reason} | offset=${handleText}`;
          } else {
            parsed += ` | state=${stateName} | opcode=${opText} | bytes=${handleText}`;
          }
        } else if (opcode === 0xA9) {
          parsed += ` | state=${stateName} | opcode=${opText} | reason=${rsText} | len=${handleText}`;
        } else if (opcode === 0xB9 || opcode === 0xBA || opcode === 0xBB || opcode === 0xBC) {
          parsed += ` | state=${stateName} | opcode=${opText} | reason=${rsText} | handle=${handleText}`;
        } else {
          parsed += ` | state=${stateName} | opcode=${opText} | reason=${rsText} | handle=${handleText}`;
        }
      } else if (len >= 1) {
        const states = ['未连接', '广播中', '已连接', '休眠'];
        parsed += ` | ${states[data[0]] || hex(data[0])}`;
      }
      break;

    case LogCategory.RGB_EVENT:
      if (len >= 2) {
        const mode = RGB_MODE_NAMES[data[0]] || hex(data[0]);
        const brightness = Math.round(data[1] * 100 / 255);
        parsed += ` | ${mode} 亮度=${brightness}%`;
      }
      break;

    case LogCategory.SYSTEM_EVENT:
      if (len >= 1) {
        const events: Record<number, string> = {
          [SystemLogEvent.BOOT]: '设备启动',
          [SystemLogEvent.SLEEP]: '进入休眠',
          [SystemLogEvent.WAKEUP]: '唤醒',
        };
        parsed += ` | ${events[data[0]] || hex(data[0])}`;
      }
      break;
  }

  return {
    command: catName,
    cmdHex: '70',
    sub,
    dataLen: len,
    rawHex,
    parsed,
    category,
  };
}
