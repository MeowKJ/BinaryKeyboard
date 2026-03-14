# HID 通讯协议（USB 配置通道）

MeowKeyboard 无线版通过 USB HID 配置通道与 Studio（WebHID）通信。本文档以当前固件实现为准，给出 **逐字节** 协议定义，并补充一版可扩展的接口设计方案。

## 概览

| 项目 | 说明 |
| :--- | :--- |
| 配置通道帧长 | `64B` |
| WebHID 模式 | **无 Report ID**（`reportId=0`） |
| 通道方向 | 主机发送命令帧，设备返回响应帧 |
| 字节序 | 以字段定义为准（协议中同时存在大端/小端字段） |

::: tip 约定
除异步日志 `KBD_CMD_LOG (0x70)` 外，**响应数据区 `DATA[0]` 均为状态码**（`kbd_resp_t`）。
:::

---

## 配置帧格式（64B）

### 主机 → 设备（命令帧）

对应固件 `kbd_cmd_frame_t`（`firmware/CH592F/keyboard/include/kbd_types.h`）。

| 偏移 | 大小 | 字段 | 说明 |
| :--- | :--- | :--- | :--- |
| `0x00` | 1 | `CMD` | 命令码（`kbd_cmd_t`） |
| `0x01` | 1 | `SUB` | 子命令/序号（层号、槽位号、宏分包序号等） |
| `0x02` | 1 | `LEN` | `DATA` 有效长度（`0~61`） |
| `0x03~0x3F` | 61 | `DATA` | 请求参数 |

### 设备 → 主机（响应帧）

当前固件响应帧格式与命令帧一致（`CMD` 为原命令回显，不携带状态码）。状态码放在 `DATA[0]`。

| 偏移 | 大小 | 字段 | 说明 |
| :--- | :--- | :--- | :--- |
| `0x00` | 1 | `CMD` | 原命令码回显 |
| `0x01` | 1 | `SUB` | 原 `SUB` 回显（如层号、槽位号） |
| `0x02` | 1 | `LEN` | `DATA` 有效长度 |
| `0x03~0x3F` | 61 | `DATA` | 响应数据（通常 `DATA[0]` 为状态码） |

### Studio / WebHID 对接（当前实现）

`tools/studio/src/services/HidService.ts` 使用统一封包方法：

- `sendCommand(cmd, sub, data)` 构造 `[CMD][SUB][LEN][DATA...]`
- `sendReport(0, frame)` 发送 64B
- `inputreport` 回调读取 `event.data.buffer`（64B）
- `CMD=0x70` 作为异步日志，不进入命令响应等待队列

---

## 状态码（`kbd_resp_t`）

| 值 | 名称 | 含义 |
| :--- | :--- | :--- |
| `0x00` | `KBD_RESP_OK` | 成功 |
| `0x01` | `KBD_RESP_ERR_INVALID` | 无效命令 |
| `0x02` | `KBD_RESP_ERR_PARAM` | 参数错误 |
| `0x03` | `KBD_RESP_ERR_BUSY` | 设备忙 |
| `0x04` | `KBD_RESP_ERR_FLASH` | Flash 操作失败 |
| `0x10` | `KBD_RESP_ERR_TOO_LARGE` | 数据过大 |
| `0x11` | `KBD_RESP_ERR_NO_SPACE` | 存储空间不足 |
| `0x12` | `KBD_RESP_ERR_NOT_FOUND` | 未找到目标 |

---

## 命令码总表（当前固件）

| 分类 | 命令 | 码 | `SUB` 含义 | Studio 当前是否封装 |
| :--- | :--- | :--- | :--- | :--- |
| 系统 | `SYS_INFO` | `0x01` | `0` | 是 |
| 系统 | `SYS_STATUS` | `0x02` | `0` | 是 |
| 配置 | `CFG_SAVE` | `0x10` | `0` | 是 |
| 配置 | `CFG_LOAD` | `0x11` | `0` | 是 |
| 配置 | `CFG_RESET` | `0x12` | `0` | 是 |
| 按键 | `KEYMAP_GET` | `0x20` | 层号 | 是 |
| 按键 | `KEYMAP_SET` | `0x21` | 层号 | 是 |
| 按键 | `LAYER_GET` | `0x22` | `0` | 间接（状态获取可替代） |
| 按键 | `LAYER_SET` | `0x23` | `0` | 可扩展使用 |
| RGB | `RGB_GET` | `0x30` | `0` | 是 |
| RGB | `RGB_SET` | `0x31` | `0` | 是 |
| 宏 | `MACRO_INFO` | `0x40` | 槽位号 / `0xFF` | 协议支持，Studio 未封装 |
| 宏 | `MACRO_GET` | `0x41` | 槽位号 | 协议支持，Studio 未封装 |
| 宏 | `MACRO_SET` | `0x42` | 槽位号 | 协议支持，Studio 未封装 |
| 宏 | `MACRO_DEL` | `0x43` | 槽位号 | 协议支持，Studio 未封装 |
| FN | `FNKEY_GET` | `0x50` | `0` | 是 |
| FN | `FNKEY_SET` | `0x51` | `0` | 是 |
| 电源 | `BATTERY` | `0x60` | `0` | 是 |
| 日志 | `LOG` | `0x70` | 日志类别 | 异步接收 |
| 日志 | `LOG_GET` | `0x71` | `0` | 是 |
| 日志 | `LOG_SET` | `0x72` | `0` | 是 |

---

## 字节级协议定义（当前实现）

说明：下文 `DATA[n]` 均指帧内 `0x03` 起始的数据区偏移。

### 1. `SYS_INFO (0x01)`

**请求**

| 字段 | 值 |
| :--- | :--- |
| `SUB` | `0x00` |
| `LEN` | `0` |
| `DATA` | 无 |

**响应**（`LEN=14`）

| `DATA` 偏移 | 大小 | 字段 | 说明 |
| :--- | :--- | :--- | :--- |
| `0` | 1 | `status` | 状态码 |
| `1` | 1 | `vid_hi` | VID 高字节 |
| `2` | 1 | `vid_lo` | VID 低字节 |
| `3` | 1 | `pid_hi` | PID 高字节 |
| `4` | 1 | `pid_lo` | PID 低字节 |
| `5` | 1 | `version_major` | 固件主版本 |
| `6` | 1 | `version_minor` | 固件次版本 |
| `7` | 1 | `version_patch` | 固件补丁版本 |
| `8` | 1 | `max_layers` | 最大层数 |
| `9` | 1 | `max_keys` | 单层最大键位数 |
| `10` | 1 | `macro_slots` | 宏槽位数 |
| `11` | 1 | `keyboard_type` | 0=基础款, 1=五键款, 2=旋钮款 |
| `12` | 1 | `actual_key_count` | 当前键盘类型实际键位数 |
| `13` | 1 | `fn_key_count` | FN 键数量 |

::: info 字节序
`VID/PID` 在该响应中采用 **高字节在前（大端表示）**。
:::

### 2. `SYS_STATUS (0x02)`

**请求**：`SUB=0x00, LEN=0`

**响应**（`LEN=6`）

| `DATA` 偏移 | 大小 | 字段 | 说明 |
| :--- | :--- | :--- | :--- |
| `0` | 1 | `status` | 状态码 |
| `1` | 1 | `work_mode` | 当前模式（USB/BLE/...） |
| `2` | 1 | `conn_state` | 连接状态 |
| `3` | 1 | `current_layer` | 当前层 |
| `4` | 1 | `battery_level` | 电量（0-100） |
| `5` | 1 | `is_charging` | 充电状态（0/1） |

### 3. 配置命令 `CFG_SAVE / CFG_LOAD / CFG_RESET`（`0x10/0x11/0x12`）

**请求**：`SUB=0x00, LEN=0`

**响应**（`LEN=1`）

| `DATA` 偏移 | 大小 | 字段 |
| :--- | :--- | :--- |
| `0` | 1 | `status` |

### 4. `KEYMAP_GET (0x20)`

**请求**

| 字段 | 值 |
| :--- | :--- |
| `SUB` | `layer_index (0~4)` |
| `LEN` | `0` |

**响应（成功）**（`LEN=36`）

| `DATA` 偏移 | 大小 | 字段 | 说明 |
| :--- | :--- | :--- | :--- |
| `0` | 1 | `status` | `0x00` |
| `1` | 1 | `num_layers` | 当前层数 |
| `2` | 1 | `current_layer` | 当前激活层 |
| `3` | 1 | `default_layer` | 默认层 |
| `4~35` | 32 | `layer_data` | `kbd_layer_t`（8 键 × 4B） |

**响应（失败）**（`LEN=1`）

| `DATA[0]` | 含义 |
| :--- | :--- |
| `KBD_RESP_ERR_PARAM` | 层号超出 `num_layers` |

#### `layer_data`（32B）=`kbd_layer_t`

每层固定 8 个按键，每键 4 字节 `kbd_action_t`：

| 偏移（层内） | 大小 | 字段 |
| :--- | :--- | :--- |
| `0` | 1 | `type` |
| `1` | 1 | `modifier` |
| `2` | 1 | `param1` |
| `3` | 1 | `param2` |

第 `i` 个键偏移 = `i * 4`。

### 5. `KEYMAP_SET (0x21)`

**请求**（推荐 `LEN=35`）

| `DATA` 偏移 | 大小 | 字段 | 说明 |
| :--- | :--- | :--- | :--- |
| `0` | 1 | `num_layers` | `1~KBD_MAX_LAYERS` |
| `1` | 1 | `reserved` | 保留（Studio 当前写 `0`） |
| `2` | 1 | `default_layer` | 默认层 |
| `3~34` | 32 | `layer_data` | 目标层 `kbd_layer_t` |

`SUB = layer_index (0~4)`。

**响应**（`LEN=1`）

| `DATA[0]` | 含义 |
| :--- | :--- |
| `status` | 成功或失败 |

::: warning 兼容行为
当前固件对 `LEN < 35` 不会直接报错，仍可能返回 `OK`（仅打印日志）。Studio 应始终发送完整 `35B`。
:::

### 6. `LAYER_GET (0x22)`

**请求**：`SUB=0x00, LEN=0`

**响应**（`LEN=4`）

| `DATA` 偏移 | 大小 | 字段 |
| :--- | :--- | :--- |
| `0` | 1 | `status` |
| `1` | 1 | `current_layer` |
| `2` | 1 | `num_layers` |
| `3` | 1 | `default_layer` |

### 7. `LAYER_SET (0x23)`

**请求**（`LEN>=1`）

| `DATA` 偏移 | 大小 | 字段 |
| :--- | :--- | :--- |
| `0` | 1 | `target_layer` |

**响应**（`LEN=2`）

| `DATA` 偏移 | 大小 | 字段 | 说明 |
| :--- | :--- | :--- | :--- |
| `0` | 1 | `status` | `OK` 或 `ERR_PARAM` |
| `1` | 1 | `current_layer` | 实际切换后的层 |

### 8. `RGB_GET (0x30)`

**请求**：`SUB=0x00, LEN=0`

**响应**（`LEN=10`）

| `DATA` 偏移 | 大小 | 字段 |
| :--- | :--- | :--- |
| `0` | 1 | `status` |
| `1` | 1 | `enabled` |
| `2` | 1 | `mode` |
| `3` | 1 | `brightness` |
| `4` | 1 | `speed` |
| `5` | 1 | `color_r` |
| `6` | 1 | `color_g` |
| `7` | 1 | `color_b` |
| `8` | 1 | `indicator_enabled` |
| `9` | 1 | `indicator_brightness` |

### 9. `RGB_SET (0x31)`

**请求**（兼容 `LEN=8` 或 `LEN=9`）

| `DATA` 偏移 | 大小 | 字段 | 说明 |
| :--- | :--- | :--- | :--- |
| `0` | 1 | `enabled` | RGB 总开关 |
| `1` | 1 | `mode` | 灯效模式 |
| `2` | 1 | `brightness` | 按键灯亮度 |
| `3` | 1 | `speed` | 动画速度 |
| `4` | 1 | `color_r` | 静态颜色 R |
| `5` | 1 | `color_g` | 静态颜色 G |
| `6` | 1 | `color_b` | 静态颜色 B |
| `7` | 1 | `indicator_enabled` | 指示灯开关 |
| `8` | 1 | `indicator_brightness` | 可选；旧协议可省略 |

**响应**（`LEN=1`）：`DATA[0] = status`

### 10. `FNKEY_GET (0x50)`

**请求**：`SUB=0x00, LEN=0`

**响应**（`LEN=33`）

| `DATA` 偏移 | 大小 | 字段 |
| :--- | :--- | :--- |
| `0` | 1 | `status` |
| `1~32` | 32 | `kbd_fnkey_config_t` |

#### `kbd_fnkey_config_t`（32B）字节布局

由 4 个 `kbd_fnkey_entry_t` 组成，每项 8B。

单项（8B）布局：

| 偏移（项内） | 大小 | 字段 |
| :--- | :--- | :--- |
| `0` | 1 | `click_action` |
| `1` | 1 | `click_param` |
| `2` | 1 | `long_action` |
| `3` | 1 | `long_param` |
| `4` | 1 | `long_press_ms_lo` |
| `5` | 1 | `long_press_ms_hi` |
| `6` | 1 | `reserved0` |
| `7` | 1 | `reserved1` |

第 `i` 个 FN 项偏移 = `i * 8`。

### 11. `FNKEY_SET (0x51)`

**请求**（`LEN=32`）

| `DATA` 偏移 | 大小 | 字段 |
| :--- | :--- | :--- |
| `0~31` | 32 | `kbd_fnkey_config_t` |

**响应**（`LEN=1`）：`DATA[0] = status`

### 12. `BATTERY (0x60)`

**请求**：`SUB=0x00, LEN=0`

**响应**（`LEN=5`）

| `DATA` 偏移 | 大小 | 字段 | 说明 |
| :--- | :--- | :--- | :--- |
| `0` | 1 | `status` | 状态码 |
| `1` | 1 | `level` | 电量百分比 `0~100` |
| `2` | 1 | `charging` | `0=未充电, 1=充电中` |
| `3` | 1 | `voltage_lo` | 电压毫伏低字节 |
| `4` | 1 | `voltage_hi` | 电压毫伏高字节 |

::: info 字节序
`voltage_mV` 在 `BATTERY` 响应中使用 **小端序**（LE）。
:::

### 13. `LOG_GET (0x71)`

**请求**：`SUB=0x00, LEN=0`

**响应**（`LEN=2`）

| `DATA` 偏移 | 大小 | 字段 |
| :--- | :--- | :--- |
| `0` | 1 | `status` |
| `1` | 1 | `enabled` |

### 14. `LOG_SET (0x72)`

**请求**（`LEN=1`）

| `DATA` 偏移 | 大小 | 字段 | 说明 |
| :--- | :--- | :--- | :--- |
| `0` | 1 | `enabled` | `0=关, 非0=开` |

**响应**（`LEN=1`）：`DATA[0] = status`

### 15. 异步日志 `LOG (0x70)`

设备主动推送，不走命令应答队列。

**帧格式**

| 帧字段 | 含义 |
| :--- | :--- |
| `CMD` | `0x70` |
| `SUB` | 日志类别 `kbd_log_category_t` |
| `LEN` | 日志数据长度（`0~8`） |
| `DATA` | 日志负载（**无状态码前缀**） |

#### 日志类别与 `DATA` 字节定义

| `SUB` | 类别 | `DATA` 字节定义 |
| :--- | :--- | :--- |
| `0x01` | `KEY_EVENT` | `[0]=key_index, [1]=pressed, [2]=action_type, [3]=param` |
| `0x02` | `FN_EVENT` | `[0]=fn_id, [1]=is_long, [2]=action, [3]=param` |
| `0x03` | `LAYER_EVENT` | `[0]=old_layer, [1]=new_layer` |
| `0x04` | `MODE_EVENT` | `[0]=old_mode, [1]=new_mode` |
| `0x05` | `BLE_EVENT` | `[0]=state` |
| `0x06` | `RGB_EVENT` | `[0]=mode, [1]=brightness` |
| `0x07` | `SYSTEM_EVENT` | `[0]=event`（`BOOT/SLEEP/WAKEUP`） |

### 16. `MACRO_INFO (0x40)`

#### 16.1 宏总览（`SUB=0xFF`）

**请求**：`LEN=0`

**响应**（成功 `LEN=11`）

| `DATA` 偏移 | 大小 | 字段 |
| :--- | :--- | :--- |
| `0` | 1 | `status` |
| `1` | 1 | `macro_slots`（当前为 8） |
| `2` | 1 | `used_count` |
| `3~10` | 8 | `valid_flags[8]`（`1=有效, 0=无效`） |

#### 16.2 宏详情（`SUB=slot`）

**请求**：`LEN=0`

**响应（成功）**（`LEN=25`）

| `DATA` 偏移 | 大小 | 字段 |
| :--- | :--- | :--- |
| `0` | 1 | `status` |
| `1~24` | 24 | `kbd_macro_header_t` |

**响应（失败）**（`LEN=1`）

| `DATA[0]` | 含义 |
| :--- | :--- |
| `KBD_RESP_ERR_NOT_FOUND` | 槽位无有效宏 |

#### `kbd_macro_header_t`（24B）字节布局

| 偏移（头内） | 大小 | 字段 | 说明 |
| :--- | :--- | :--- | :--- |
| `0` | 1 | `valid` | `0xAA=有效` |
| `1` | 1 | `id` | 宏 ID（0~7） |
| `2` | 1 | `action_count_lo` | 小端 |
| `3` | 1 | `action_count_hi` | 小端 |
| `4` | 1 | `data_size_lo` | 小端 |
| `5` | 1 | `data_size_hi` | 小端 |
| `6` | 1 | `reserved0` | 保留 |
| `7` | 1 | `reserved1` | 保留 |
| `8~23` | 16 | `name` | UTF-8 名称（可含 `\0` 结尾） |

### 17. `MACRO_GET (0x41)`

**请求**（`SUB=slot`, `LEN=3`）

| `DATA` 偏移 | 大小 | 字段 | 说明 |
| :--- | :--- | :--- | :--- |
| `0` | 1 | `offset_hi` | 宏数据读取偏移高字节 |
| `1` | 1 | `offset_lo` | 宏数据读取偏移低字节 |
| `2` | 1 | `req_len` | 请求长度（固件内部会 clamp 到 `<=56`） |

**响应（成功）**（`LEN = 5 + read_len`）

| `DATA` 偏移 | 大小 | 字段 | 说明 |
| :--- | :--- | :--- | :--- |
| `0` | 1 | `status` | `OK` |
| `1` | 1 | `offset_hi` | 回显偏移高字节 |
| `2` | 1 | `offset_lo` | 回显偏移低字节 |
| `3` | 1 | `read_len` | 实际读取长度 |
| `4` | 1 | `is_last` | `1=到达末尾`, `0=仍有后续` |
| `5..` | `read_len` | `chunk` | 宏数据片段 |

**响应（失败）**（`LEN=1`）

| `DATA[0]` | 含义 |
| :--- | :--- |
| `KBD_RESP_ERR_NOT_FOUND` | 槽位无宏 |

::: info 字节序
`MACRO_GET` 的 `offset` 在请求/响应中均使用 **高字节在前（大端表示）**。
:::

### 18. `MACRO_SET (0x42)`

`SUB = slot`。`DATA[0]` 为分包序号 `seq`，定义如下：

- `0x00`：开始写入（BeginWrite）
- `0x01~0xFE`：写入数据块（WriteChunk）
- `0xFF`：结束写入（EndWrite）

#### 18.1 BeginWrite（`seq=0x00`）

**请求**（`LEN=25`）

| `DATA` 偏移 | 大小 | 字段 |
| :--- | :--- | :--- |
| `0` | 1 | `seq = 0x00` |
| `1~24` | 24 | `kbd_macro_header_t` |

**响应**（`LEN=2`）

| `DATA` 偏移 | 大小 | 字段 |
| :--- | :--- | :--- |
| `0` | 1 | `status`（`OK/ERR_TOO_LARGE/ERR_FLASH`） |
| `1` | 1 | `seq_echo = 0x00` |

#### 18.2 WriteChunk（`seq=0x01~0xFE`）

**请求**（`LEN = 4 + chunk_len`）

| `DATA` 偏移 | 大小 | 字段 | 说明 |
| :--- | :--- | :--- | :--- |
| `0` | 1 | `seq` | 分包序号 |
| `1` | 1 | `offset_hi` | 偏移高字节 |
| `2` | 1 | `offset_lo` | 偏移低字节 |
| `3` | 1 | `chunk_len` | 数据块长度 |
| `4..` | `chunk_len` | `chunk` | 数据块 |

**响应**（`LEN=2`）

| `DATA[0]` | `DATA[1]` |
| :--- | :--- |
| `status`（`OK/ERR_FLASH`） | `seq_echo` |

#### 18.3 EndWrite（`seq=0xFF`）

**请求**（最小 `LEN=1`）

| `DATA[0]` | 含义 |
| :--- | :--- |
| `0xFF` | 结束写入 |

**响应**（`LEN=2`）

| `DATA[0]` | `DATA[1]` |
| :--- | :--- |
| `status`（`OK/ERR_FLASH`） | `0xFF` |

### 19. `MACRO_DEL (0x43)`

**请求**：`SUB=slot`, `LEN=0`

**响应**（`LEN=1`）

| `DATA[0]` | 含义 |
| :--- | :--- |
| `status`（`OK` 或 `ERR_PARAM`） | 删除结果 |

---

## Studio 当前命令调用映射（`HidService`）

| Studio 方法 | 命令 | 请求字节 | 响应字节（`DATA`） |
| :--- | :--- | :--- | :--- |
| `getSysInfo()` | `SYS_INFO` | 无 | `status + 13B 信息` |
| `getSysStatus()` | `SYS_STATUS` | 无 | `status + 5B 状态` |
| `getKeymap(layer)` | `KEYMAP_GET` | `SUB=layer` | `status + 3B 层头 + 32B层数据` |
| `setKeymap(...)` | `KEYMAP_SET` | `35B` | `status` |
| `getRgbConfig()` | `RGB_GET` | 无 | `status + 9B` |
| `setRgbConfig()` | `RGB_SET` | `9B`（兼容 8B） | `status` |
| `getFnKeyConfig()` | `FNKEY_GET` | 无 | `status + 32B` |
| `setFnKeyConfig()` | `FNKEY_SET` | `32B` | `status` |
| `saveConfig()` | `CFG_SAVE` | 无 | `status` |
| `loadConfig()` | `CFG_LOAD` | 无 | `status` |
| `resetConfig()` | `CFG_RESET` | 无 | `status` |
| `getBattery()` | `BATTERY` | 无 | `status + 4B` |
| `getLogConfig()` | `LOG_GET` | 无 | `status + enabled` |
| `setLogConfig()` | `LOG_SET` | `enabled(1B)` | `status` |

---

## 可读性优先的代码设计（建议）

目标：在不破坏现有协议兼容性的前提下，提高固件与 Studio 两端代码可读性，并为按分区提交（冷热分离）预留接口。

### 固件端分层（建议）

| 文件 | 职责 |
| :--- | :--- |
| `kbd_proto_codec.h/.c` | 帧封包/解包、常用响应构造（状态码、长度检查） |
| `kbd_proto_handlers.h/.c` | 命令分发与各 `HandleXxx` 处理函数 |
| `kbd_storage_regions.h/.c` | `BASE/KEYMAP/RUNTIME/MACRO` 分区读写与 commit |
| `kbd_storage_page.h/.c` | `256B` 页日志通用逻辑（CRC、seq、valid） |

建议的最小公共接口（固件）：

```c
// 协议编解码层（建议）
bool KBD_Proto_RequireLen(const kbd_cmd_frame_t *f, uint8_t min_len);
void KBD_Proto_SendStatus(uint8_t cmd, uint8_t sub, kbd_resp_t st);
void KBD_Proto_SendData(uint8_t cmd, uint8_t sub, const void *data, uint8_t len);

// 存储分区层（建议）
typedef enum {
  KBD_CFG_REGION_BASE    = 1u << 0,
  KBD_CFG_REGION_KEYMAP  = 1u << 1,
  KBD_CFG_REGION_RUNTIME = 1u << 2,
} kbd_cfg_region_mask_t;

int KBD_Config_Commit(uint8_t region_mask);
int KBD_Config_GetDirtyMask(void);
int KBD_Runtime_SaveState(uint8_t layer /*, uint8_t mode, uint8_t reserved_slot */);
```

### Studio 端分层（建议）

| 层 | 职责 |
| :--- | :--- |
| `HidService` | 仅负责帧收发与单命令 API |
| `ConfigSessionService`（新增） | 管理 `dirtyMask`、提交策略、兼容新旧协议 |
| `deviceStore` | UI 状态与草稿数据（draft） |

建议的最小接口（Studio）：

```ts
interface ConfigCommitResult {
  requestedMask: number;
  committedMask: number;
  failedMask: number;
}

interface ConfigSessionService {
  markDirty(mask: number): void;
  getDirtyMask(): number;
  commit(mask?: number): Promise<ConfigCommitResult>;
  abort(mask?: number): Promise<void>;
}
```

---

## 协议扩展方案（规划，兼容现有 Studio）

> 以下命令为建议新增，当前固件未实现。旧版 Studio 可继续使用 `KEYMAP_SET/RGB_SET/FNKEY_SET + CFG_SAVE`。

### 扩展命令建议

| 命令 | 码（建议） | 用途 |
| :--- | :--- | :--- |
| `CFG_CAPS_GET` | `0x13` | 查询协议能力/布局能力 |
| `CFG_STATUS_GET` | `0x14` | 查询 `dirty_mask`、当前槽位、序号 |
| `CFG_COMMIT_EX` | `0x15` | 按分区提交（小配置按页写） |
| `CFG_ABORT` | `0x16` | 丢弃未提交 RAM 草稿（可选） |

### `CFG_CAPS_GET (0x13)`（建议字节布局）

**请求**：`SUB=0, LEN=0`

**响应**（建议 `LEN=16`）

| `DATA` 偏移 | 大小 | 字段 | 说明 |
| :--- | :--- | :--- | :--- |
| `0` | 1 | `status` | `OK` |
| `1` | 1 | `proto_major` | 协议主版本 |
| `2` | 1 | `proto_minor` | 协议次版本 |
| `3` | 1 | `frame_size` | 固定 `64` |
| `4` | 1 | `feature_flags_lo` | 能力位低字节 |
| `5` | 1 | `feature_flags_hi` | 能力位高字节 |
| `6` | 1 | `region_mask_supported` | 支持提交的分区掩码 |
| `7` | 1 | `config_slot_count` | 配置槽数量 |
| `8` | 1 | `runtime_page_count` | runtime 页数 |
| `9` | 1 | `cfg_page_size_log2` | 如 `8` 表示 256B |
| `10` | 1 | `macro_erase_block_log2` | 如 `12` 表示 4KB |
| `11` | 1 | `layout_version_major` | 存储布局主版本 |
| `12` | 1 | `layout_version_minor` | 存储布局次版本 |
| `13` | 1 | `reserved0` | 保留 |
| `14` | 1 | `reserved1` | 保留 |
| `15` | 1 | `reserved2` | 保留 |

### `CFG_STATUS_GET (0x14)`（建议字节布局）

**请求**：`SUB=0, LEN=0`

**响应**（建议 `LEN=16`）

| `DATA` 偏移 | 大小 | 字段 |
| :--- | :--- | :--- |
| `0` | 1 | `status` |
| `1` | 1 | `dirty_mask` |
| `2` | 1 | `active_config_slot`（`0xFF`=无） |
| `3` | 1 | `active_runtime_page`（`0xFF`=无） |
| `4` | 1 | `config_save_count_0`（LE） |
| `5` | 1 | `config_save_count_1` |
| `6` | 1 | `config_save_count_2` |
| `7` | 1 | `config_save_count_3` |
| `8` | 1 | `runtime_seq_0`（LE） |
| `9` | 1 | `runtime_seq_1` |
| `10` | 1 | `runtime_seq_2` |
| `11` | 1 | `runtime_seq_3` |
| `12` | 1 | `current_layer_ram` |
| `13` | 1 | `current_mode_ram` |
| `14` | 1 | `reserved_profile_slot`（预留未来 BLE 槽位） |
| `15` | 1 | `busy_flags` |

### `CFG_COMMIT_EX (0x15)`（建议字节布局）

**请求**（建议 `LEN=2`）

| `DATA` 偏移 | 大小 | 字段 | 说明 |
| :--- | :--- | :--- | :--- |
| `0` | 1 | `commit_mask` | 分区掩码（`BASE/KEYMAP/RUNTIME`） |
| `1` | 1 | `flags` | 提交标志（如强制/同步） |

**响应**（建议 `LEN=8`）

| `DATA` 偏移 | 大小 | 字段 |
| :--- | :--- | :--- |
| `0` | 1 | `status` |
| `1` | 1 | `requested_mask` |
| `2` | 1 | `committed_mask` |
| `3` | 1 | `failed_mask` |
| `4` | 1 | `active_config_slot` |
| `5` | 1 | `active_runtime_page` |
| `6` | 1 | `dirty_mask_after` |
| `7` | 1 | `detail_code` |

---

## 相关文档

- [DataFlash 布局](./dataflash.md) - 配置槽轮转、runtime 热数据页、宏区布局
- [无线版开发指南](./dev.md) - 构建与固件架构
- [改键软件使用](./remap.md) - Studio / WebHID 使用说明
