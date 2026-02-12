# HID 通讯协议

MeowKeyboard 无线版通过 USB HID 与主机通信，支持标准键盘/鼠标/多媒体输入，以及专用配置通道。

## 概览

| 项目 | 说明 |
| :--- | :--- |
| 设备类型 | USB HID 复合设备 |
| VID/PID | 0x413D / 0x2107 |
| 接口数量 | 4 个 HID 接口 |
| 配置帧大小 | 64 字节 |

## USB 接口布局

| 接口 | 用途 | 端点 | 方向 | 报告大小 |
| :--- | :--- | :--- | :--- | :--- |
| 0 | 键盘 (Boot Protocol) | EP1 IN | 设备→主机 | 8B |
| 1 | 鼠标 (Boot Protocol) | EP2 IN | 设备→主机 | 4B |
| 2 | 多媒体 (Consumer) | EP3 IN | 设备→主机 | 2B |
| 3 | **配置通道** | EP4 IN/OUT | 双向 | 64B |

键盘、鼠标、多媒体接口仅做**输入**，用于向主机发送按键、点击、多媒体事件。**配置通道**为双向，主机发送命令、设备返回响应。

## HID Report 描述符

### 键盘报告 (8 字节)

| 字节 | 内容 |
| :--- | :--- |
| 0 | 修饰键 (Modifier): bit0~7 = LCtrl, LShift, LAlt, LGui, RCtrl, RShift, RAlt, RGui |
| 1 | 保留 (固定 0) |
| 2-3 | LED 输出: NumLock, CapsLock, ScrollLock, Compose, Kana |
| 4-9 | 按键数组 (6 个键码，0=无按键) |

### 鼠标报告 (4 字节)

| 字节 | 内容 |
| :--- | :--- |
| 0 | 按钮: bit0=左键, bit1=右键, bit2=中键 |
| 1 | X 位移 (-127 ~ 127) |
| 2 | Y 位移 (-127 ~ 127) |
| 3 | 滚轮 (-127 ~ 127) |

### 多媒体报告 (2 字节)

| 字节 | 内容 |
| :--- | :--- |
| 0-1 | Usage ID (16 位，小端序)，如 0x00E9=音量+, 0x00EA=音量- |

### 配置通道 (64 字节)

Usage Page: 0xFF00 (Vendor Defined)。无 Report ID，通过独立接口区分，兼容 Linux/macOS。

- **Output Report** (主机→设备): 命令帧
- **Input Report** (设备→主机): 响应帧

---

## 配置命令帧格式

所有配置命令与响应均采用 64 字节帧结构。

### 命令帧 (主机 → 设备)

| 偏移 | 大小 | 字段 | 说明 |
| :--- | :--- | :--- | :--- |
| 0 | 1 | cmd | 命令码 |
| 1 | 1 | sub | 子命令/序号 (如层号、宏槽位、分包序号) |
| 2 | 1 | len | 数据区有效字节数 (0~61) |
| 3 | 61 | data | 命令参数 |

### 响应帧 (设备 → 主机)

| 偏移 | 大小 | 字段 | 说明 |
| :--- | :--- | :--- | :--- |
| 0 | 1 | cmd | 原命令码 (高 4 位) + 响应码 (低 4 位) |
| 1 | 1 | sub | 子命令/序号 |
| 2 | 1 | len | 数据区有效字节数 |
| 3 | 61 | data | 响应数据 |

`cmd` 高 4 位为原命令，低 4 位为 `kbd_resp_t` 响应码。

---

## 命令码一览

| 分类 | 命令 | 码 | 说明 |
| :--- | :--- | :--- | :--- |
| 系统 | SYS_INFO | 0x01 | 获取设备信息 |
| | SYS_STATUS | 0x02 | 获取运行状态 |
| 配置 | CFG_SAVE | 0x10 | 保存配置到 Flash |
| | CFG_LOAD | 0x11 | 从 Flash 加载配置 |
| | CFG_RESET | 0x12 | 恢复出厂设置 |
| 按键 | KEYMAP_GET | 0x20 | 获取按键映射 |
| | KEYMAP_SET | 0x21 | 设置按键映射 |
| | LAYER_GET | 0x22 | 获取当前层 |
| | LAYER_SET | 0x23 | 设置当前层 |
| RGB | RGB_GET | 0x30 | 获取 RGB 配置 |
| | RGB_SET | 0x31 | 设置 RGB 配置 |
| 宏 | MACRO_INFO | 0x40 | 获取宏信息 |
| | MACRO_GET | 0x41 | 读取宏数据 |
| | MACRO_SET | 0x42 | 写入宏数据 (分包) |
| | MACRO_DEL | 0x43 | 删除宏 |
| FN | FNKEY_GET | 0x50 | 获取 FN 键配置 |
| | FNKEY_SET | 0x51 | 设置 FN 键配置 |
| 电源 | BATTERY | 0x60 | 获取电池信息 |

---

## 响应码

| 码 | 名称 | 说明 |
| :--- | :--- | :--- |
| 0x00 | OK | 成功 |
| 0x01 | ERR_INVALID | 无效命令 |
| 0x02 | ERR_PARAM | 参数错误 |
| 0x03 | ERR_BUSY | 设备忙 |
| 0x04 | ERR_FLASH | Flash 操作失败 |
| 0x10 | ERR_TOO_LARGE | 数据过大 |
| 0x11 | ERR_NO_SPACE | 存储空间不足 |
| 0x12 | ERR_NOT_FOUND | 未找到目标 |

---

## 典型命令与数据格式

### SYS_INFO (0x01)

- **请求**: 无数据，`len=0`
- **响应**: 14 字节
  - [0-1] vendor_id (小端)
  - [2-3] product_id (小端)
  - [4] version_major
  - [5] version_minor
  - [6] version_patch
  - [7] max_layers
  - [8] max_keys
  - [9] macro_slots
  - [10] keyboard_type (0=基础, 1=五键, 2=旋钮)
  - [11] actual_key_count
  - [12] fn_key_count
  - [13] 保留

### KEYMAP_GET (0x20)

- **请求**: `sub` = 层号 (0~4)，`len=0`
- **响应**: `sub` = 层号，`len` = 4 + 32
  - data[0-3]: 层头
  - data[4-35]: kbd_layer_t (8 键 × 4 字节)

### RGB_SET (0x31)

- **请求**: `len=9`，data 依次为: enabled, mode, brightness, speed, color_r/g/b, indicator_enabled, indicator_brightness
- **响应**: 1 字节，0=成功

### MACRO_SET (0x42)

分包写入：`sub` 为序号，0=BeginWrite/EndWrite，1~N=WriteChunk。详见 [DataFlash 布局](./dataflash.md#宏数据区-0x1000--0x4fff)。

---

## WebHID 使用

网页端通过 [WebHID API](https://developer.mozilla.org/en-US/docs/Web/API/WebHID_API) 连接键盘。需使用支持 WebHID 的浏览器（如 Chrome、Edge）。

1. 调用 `navigator.hid.requestDevice()` 并指定 VID `0x413D`、PID `0x2107`
2. 选择配置接口（通常为第 4 个接口，报告大小 64 字节）
3. 打开设备后，通过 `sendReport()` 发送 Output Report，监听 `oninputreport` 接收 Input Report

::: info 无 Report ID
配置接口采用无 Report ID 模式，Output/Input Report 均无首字节 Report ID，直接为 64 字节有效载荷。
:::

::: warning 权限
WebHID 需用户主动授权，且部分浏览器仅在安全上下文 (HTTPS) 下可用。
:::

---

## 相关文档

- [DataFlash 布局](./dataflash.md) - 配置与宏的存储结构
- [改键软件](./remap.md) - 使用网页端改键
- [低功耗蓝牙](./ble.md) - BLE 模式下的 HID 与服务
