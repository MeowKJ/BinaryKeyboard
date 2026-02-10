# CH592F DataFlash 使用说明

MeowKeyboard CH592F 固件在 DataFlash 中的存储布局说明。

## DataFlash 概览

- **基地址**：`0x70000`（CH592 物理地址）
- **总容量**：32KB (`0x0000` ~ `0x7FFF`)
- **擦除粒度**：4KB
- **写入推荐**：256 字节对齐

## 整体布局

| 地址范围 | 大小 | 用途 |
| :------- | :--- | :--- |
| 0x0000 ~ 0x0FFF | 4KB | 配置块（整块擦写） |
| 0x1000 ~ 0x4FFF | 16KB | 宏数据区（8 槽 × 2KB） |
| 0x5000 ~ 0x7DFF | 11.5KB | 预留 |
| 0x7E00 ~ 0x7EFF | 256B | **BLE SNV**（蓝牙配对信息） |
| 0x7F00 ~ 0x7FFF | 256B | 预留 |

> 配置块与宏区分离于不同 4KB 块；宏区前移以避开 BLE SNV，8 槽位均可安全使用。

---

## 配置区 (0x0000 ~ 0x0FFF)

配置区占据首块 4KB，整块擦除和写入。

### 块内偏移布局

| 偏移 | 大小 | 内容 |
| :--- | :--- | :--- |
| 0x000 | 32B | 配置头 kbd_config_header_t |
| 0x020 ~ 0x0FF | 224B | 填充/保留 |
| 0x100 | 64B | 系统配置 kbd_system_config_t |
| 0x140 ~ 0x1FF | 192B | 填充/保留 |
| 0x200 | 164B | 按键映射 kbd_keymap_t |
| 0x2A4 ~ 0x2FF | 92B | 填充/保留 |
| 0x300 | 32B | FN 键配置 kbd_fnkey_config_t |
| 0x320 ~ 0x33F | 32B | 填充/保留 |
| 0x340 | 32B | RGB 配置 kbd_rgb_config_t |
| 0x360 ~ 0xFFF | 3232B | 填充/保留 |

---

## 配置头 kbd_config_header_t (32 字节, 偏移 0x000)

| 字节 | 偏移 | 字段 | 类型 | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| 0-3 | 0x00 | magic | uint32_t | 魔数 `0x4D454F57` ("MEOW") |
| 4-5 | 0x04 | version | uint16_t | 配置版本 `0x0101` |
| 6-7 | 0x06 | flags | uint16_t | 标志位 |
| 8-11 | 0x08 | crc32 | uint32_t | 各配置块 CRC32 异或校验 |
| 12-15 | 0x0C | save_count | uint32_t | 保存计数 |
| 16-31 | 0x10 | reserved | uint8_t[16] | 保留 |

---

## 系统配置 kbd_system_config_t (64 字节, 偏移 0x100)

| 字节 | 偏移 | 字段 | 类型 | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| 0 | 0x100 | default_mode | uint8_t | 默认模式 (0=USB, 1=BLE, 2=2.4G预留) |
| 1 | 0x101 | auto_sleep_min | uint8_t | 自动休眠时间（分钟，0=禁用） |
| 2 | 0x102 | debounce_ms | uint8_t | 按键消抖时间（毫秒） |
| 3-63 | 0x103 | reserved | uint8_t[61] | 保留 |

---

## 按键映射 kbd_keymap_t (164 字节, 偏移 0x200)

| 字节 | 偏移 | 字段 | 类型 | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| 0 | 0x200 | num_layers | uint8_t | 层数 (1-5) |
| 1 | 0x201 | current_layer | uint8_t | 当前激活层 |
| 2 | 0x202 | default_layer | uint8_t | 默认层 |
| 3 | 0x203 | reserved | uint8_t | 保留 |
| 4-163 | 0x204 | layers[5] | kbd_layer_t[5] | 5 层 × 32 字节 |

### 单层 kbd_layer_t (32 字节)

每层 8 个按键 × 4 字节 (kbd_action_t)

| 字节 | 字段 | 说明 |
| :--- | :--- | :--- |
| 0 | type | 动作类型 |
| 1 | modifier | 修饰键/操作类型 |
| 2 | param1 | 键码/参数1 |
| 3 | param2 | 参数2 |

---

## FN 键配置 kbd_fnkey_config_t (32 字节, 偏移 0x300)

4 个 FN 键 × 8 字节 (kbd_fnkey_entry_t)

| 字节 | 偏移 | 字段 | 说明 |
| :--- | :--- | :--- | :--- |
| 0 | 0x300 | fn[0].click_action | FN1 短按动作 |
| 1 | 0x301 | fn[0].click_param | FN1 短按参数 |
| 2 | 0x302 | fn[0].long_action | FN1 长按动作 |
| 3 | 0x303 | fn[0].long_param | FN1 长按参数 |
| 4-5 | 0x304 | fn[0].long_press_ms | 长按阈值 (little-endian) |
| 6-7 | 0x306 | fn[0].reserved | 保留 |
| 8-15 | 0x308 | fn[1] | FN2 配置 |
| 16-23 | 0x310 | fn[2] | FN3 配置 |
| 24-31 | 0x318 | fn[3] | FN4 配置 |

---

## RGB 配置 kbd_rgb_config_t (32 字节, 偏移 0x340)

| 字节 | 偏移 | 字段 | 类型 | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| 0 | 0x340 | enabled | uint8_t | RGB 总开关 |
| 1 | 0x341 | mode | uint8_t | 灯效模式 (0=关,1=静态,2=呼吸,3=闪烁,4=彩虹,5=状态指示) |
| 2 | 0x342 | **brightness** | uint8_t | **RGB/按键灯亮度 (0-255)** |
| 3 | 0x343 | speed | uint8_t | 动画速度 (0-255) |
| 4 | 0x344 | color_r | uint8_t | 静态颜色 R |
| 5 | 0x345 | color_g | uint8_t | 静态颜色 G |
| 6 | 0x346 | color_b | uint8_t | 静态颜色 B |
| 7 | 0x347 | indicator_enabled | uint8_t | 状态指示开关 |
| 8 | 0x348 | **indicator_brightness** | uint8_t | **指示灯亮度 (0-255)** |
| 9-31 | 0x349 | reserved | uint8_t[23] | 保留 |

---

## 宏数据区 (0x1000 ~ 0x4FFF)

宏区前移，与 BLE SNV (0x7E00~0x7EFF) 完全分离，避免冲突。

| 槽位 | 地址范围 | 大小 | 说明 |
| :--- | :------- | :--- | :--- |
| 0 | 0x1000 ~ 0x17FF | 2KB | 宏槽位 0 |
| 1 | 0x1800 ~ 0x1FFF | 2KB | 宏槽位 1 |
| 2 | 0x2000 ~ 0x27FF | 2KB | 宏槽位 2 |
| 3 | 0x2800 ~ 0x2FFF | 2KB | 宏槽位 3 |
| 4 | 0x3000 ~ 0x37FF | 2KB | 宏槽位 4 |
| 5 | 0x3800 ~ 0x3FFF | 2KB | 宏槽位 5 |
| 6 | 0x4000 ~ 0x47FF | 2KB | 宏槽位 6 |
| 7 | 0x4800 ~ 0x4FFF | 2KB | 宏槽位 7 |

### 宏槽位布局 (每槽 2KB)

| 偏移 | 大小 | 内容 |
| :--- | :--- | :--- |
| 0 | 1 | valid (0xAA=有效) |
| 1 | 1 | id (0-7) |
| 2-3 | 2 | action_count |
| 4-5 | 2 | data_size |
| 6-7 | 2 | reserved |
| 8-23 | 16 | name (UTF-8) |
| 24~ | - | 宏动作数据 |

---

## BLE SNV 区 (0x7E00 ~ 0x7EFF)

蓝牙协议栈用于存储配对/绑定信息的非易失存储，由 WCH BLE 库管理。

| 参数 | 默认值 | 说明 |
| :--- | :----- | :--- |
| BLE_SNV_ADDR | `0x77000 - FLASH_ROM_MAX_SIZE` | 实际约为 0x7E00（Data Flash 末段） |
| BLE_SNV_BLOCK | 256 | 块大小 |
| BLE_SNV_NUM | 1 | 块数量 |

读写通过 `Lib_Read_Flash` / `Lib_Write_Flash` 回调，内部使用 `EEPROM_READ` / `EEPROM_WRITE`。CH592A 需按 4KB 块擦除，会读-改-写整块。

---

## 保存流程

1. 通过 HID 命令修改 RAM 中的配置（如 RGB_SET）
2. 发送 **CFG_SAVE** (0x10) 触发保存
3. 固件擦除 0x0000 起 4KB 块，写入整块数据

---

## 注意事项

### 配置区整体

::: warning 整块擦写
配置区 0x0000~0x0FFF 作为一个 **4KB 块**整体擦除和写入，无法单独更新某一配置块。每次 CFG_SAVE 都会重写全部配置（含头部、系统、键映射、FN、RGB）。
:::

::: tip 魔数与版本校验
加载时仅校验 `magic == 0x4D454F57` 和主版本号 `(version >> 8)`。魔数错误或主版本不匹配时，将回退到默认配置，**不会**从 Flash 读取任何配置数据。
:::

::: info CRC 校验
`crc32` 由系统、键映射、FN、RGB 四块的 CRC32 异或得出，写入时计算。当前固件**加载时不校验 CRC**，仅作存储用。
:::

### 按键映射

::: warning 层数与键盘类型
`num_layers` 须与键盘类型一致：基础款 4、五键款 5、旋钮款 5。`current_layer`、`default_layer` 必须小于 `num_layers`，否则可能出现越界访问。
:::

::: info 按键槽位
每层固定 8 个按键槽位 (KBD_MAX_KEYS)，实际有效按键数由 `deviceInfo.actualKeyCount` 决定。超出部分的槽位可保留为 NONE。
:::

### FN 键配置

::: info 实际使用
基础款与五键款仅使用 `fn[0]`、`fn[1]`，`fn[2]`、`fn[3]` 为保留字段。`long_press_ms` 为 16 位小端序。
:::

### RGB 配置

::: warning 双亮度独立保存
`brightness` (0x342) 与 `indicator_brightness` (0x348) 为**两个独立参数**，需通过 RGB_SET 传入 9 字节（含 data[8]）并执行 CFG_SAVE 才能完整保存。网页端请点击「保存 RGB」。
:::

::: tip 旧配置迁移
加载时若 `indicator_brightness == 0`，会自动设为默认值 51（20% 亮度），用于兼容未包含该字段的旧固件配置。
:::

### 宏数据区

::: warning 擦除粒度
宏槽位按 4KB 块边界对齐，写入/删除单个宏时会擦除整块。槽位 0~1 共用块 0x1000~0x1FFF，槽位 2~3 共用 0x2000~0x2FFF，以此类推。修改一个槽位可能影响同块内另一槽位。
:::

::: warning 宏写入流程
宏写入必须严格按顺序：`MACRO_SET seq=0`（BeginWrite）→ `MACRO_SET seq=1~N`（WriteChunk，可分多包）→ `MACRO_SET seq=0` 且完成（EndWrite 标记有效）。中途失败或未调用 EndWrite 时，该槽位将保持无效。
:::

::: info 宏大小限制
单宏动作数据最大 `KBD_MACRO_MAX_SIZE - 24 = 2024` 字节，动作数量最多 `KBD_MACRO_MAX_ACTIONS = 1000`。`data_size` 不含 24 字节头部。
:::

::: info 有效标记
槽位首字节 `valid == 0xAA` 表示该宏有效。删除宏时将整槽填 0xFF，`valid` 变为非 0xAA。
:::

### 保存与恢复

::: warning 网页端分按钮保存
- **保存配置**：仅保存按键映射 (KEYMAP_SET + CFG_SAVE)
- **保存 FN**：仅保存 FN 键 (FNKEY_SET + CFG_SAVE)
- **保存 RGB**：仅保存 RGB (RGB_SET + CFG_SAVE)

修改 FN 或 RGB 后需点击对应按钮，否则重启后不会生效。
:::

::: warning 恢复出厂
CFG_RESET 会调用 `KBD_Config_Reset`：加载默认配置、**清除全部 8 个宏槽位**、再执行 CFG_SAVE。所有自定义配置将丢失。
:::

### DataFlash 与 BLE SNV

::: info BLE 占用 DataFlash
蓝牙协议栈的 **SNV**（Simple Non-Volatile）用于存储配对/绑定信息，使用 DataFlash 末段 **0x7E00~0x7EFF**（256 字节），由 `ble_config.h` 配置。
:::

### 升级与迁移
