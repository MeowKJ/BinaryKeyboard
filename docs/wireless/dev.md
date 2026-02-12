# 无线版固件开发

基于 **CH592F** 芯片的 USB/蓝牙双模键盘固件开发指南。

## 开发环境

### 推荐工具链

| 工具                | 说明                |
| :------------------ | :------------------ |
| MounRiver Studio II | WCH 官方 RISC-V IDE |
| WCHISPStudio        | 固件烧录工具        |

### 配置开发环境

1. 下载安装 [MounRiver Studio II](http://www.mounriver.com/)
2. 打开项目文件 `Firmware/CH592F/CH592F.wvproj`

## 代码架构

### 目录结构

```
Firmware/CH592F/
├── Board/              # 板级支持包 (BSP)
│   ├── include/
│   │   ├── kbd_config.h    # GPIO 引脚配置
│   │   ├── key.h           # 按键驱动接口
│   │   ├── ws2812.h        # WS2812 RGB 驱动
│   │   ├── debug.h         # 调试输出
│   │   └── hal_utils.h     # HAL 工具函数
│   └── src/
│       ├── key.c           # 中断驱动按键实现
│       ├── ws2812.c        # PWM+DMA 驱动 WS2812
│       ├── debug.c         # 串口调试
│       └── hal_utils.c     # 工具函数
│
├── MeowBLE/            # 蓝牙协议栈
│   ├── hal/            # 硬件抽象层
│   │   └── ble_mcu.c       # BLE 库初始化
│   ├── lib/            # WCH BLE 库 (预编译)
│   │   ├── CH59xBLE_LIB.h
│   │   └── libCH59xBLE.a
│   ├── meow/           # 自定义 BLE HID
│   │   ├── ble_hid.c/h         # BLE HID 实现
│   │   ├── ble_hid_service.c/h # HID 服务
│   │   ├── kbd_mode.c/h        # 模式管理器
│   │   └── kbd_mode_config.h   # 模式配置
│   └── profile/        # BLE Profiles
│       ├── hiddev.c/h          # HID 设备
│       ├── battservice.c/h     # 电池服务
│       └── devinfoservice.c/h  # 设备信息服务
│
├── MeowKeyboard/       # 键盘核心逻辑
│   ├── include/
│   │   ├── kbd_core.h      # 核心处理接口
│   │   ├── kbd_types.h     # 数据类型定义
│   │   ├── kbd_storage.h   # 配置存储接口
│   │   ├── kbd_command.h   # HID 命令处理
│   │   └── kbd_rgb.h       # RGB 灯效引擎
│   └── src/
│       ├── kbd_core.c      # 按键事件处理
│       ├── kbd_storage.c   # DataFlash 存储
│       ├── kbd_command.c   # 改键命令解析
│       └── kbd_rgb.c       # 灯效实现
│
├── MeowUSB/            # USB HID 模块
│   ├── include/
│   │   ├── usb_hid.h       # USB HID 接口
│   │   ├── usb_device.h    # USB 设备管理
│   │   └── usb_descriptors.h
│   └── src/
│       ├── usb_hid.c       # USB HID 实现
│       ├── usb_device.c    # USB 设备初始化
│       └── usb_descriptors.c
│
├── StdPeriphDriver/    # CH592 外设驱动
├── RVMSIS/             # RISC-V 核心支持
├── Startup/            # 启动文件
├── Ld/                 # 链接脚本
│
└── User/
    └── Main.c          # 主程序入口
```

### 核心模块说明

#### 1. Board - 板级支持

| 文件           | 功能                                 |
| :------------- | :----------------------------------- |
| `kbd_config.h` | GPIO 引脚定义、键盘布局配置          |
| `key.c/h`      | 中断驱动按键，lockout 去抖，事件队列 |
| `ws2812.c/h`   | PWM+DMA 驱动 WS2812，支持亮度调节    |

#### 2. MeowKeyboard - 键盘核心

| 文件            | 功能                                  |
| :-------------- | :------------------------------------ |
| `kbd_core.c`    | 按键事件处理、FN 键逻辑、HID 报告生成 |
| `kbd_types.h`   | 所有数据结构定义（动作、层、宏等）    |
| `kbd_storage.c` | DataFlash 读写、配置持久化            |
| `kbd_rgb.c`     | RGB 灯效：静态/呼吸/彩虹/状态指示     |

#### 3. MeowBLE - 蓝牙模块

| 文件                | 功能                           |
| :------------------ | :----------------------------- |
| `kbd_mode.c`        | USB/BLE/2.4G 多模切换、连接状态管理（2.4G 预留） |
| `ble_hid.c`         | 蓝牙 HID 报告发送              |
| `ble_hid_service.c` | HID GATT 服务实现              |

#### 4. MeowUSB - USB 模块

| 文件           | 功能                     |
| :------------- | :----------------------- |
| `usb_hid.c`    | USB HID 报告发送         |
| `usb_device.c` | USB 设备初始化与中断处理 |

## 键盘类型与版本配置

### 支持的键盘类型

| 类型   | 枚举值 | 物理按键 | 虚拟键位 | 说明                   |
| :----- | :----- | :------- | :------- | :--------------------- |
| 基础款 | 0      | 4 键     | 4        | 标准 4 键布局          |
| 五键款 | 1      | 5 键     | 5        | 扩展 5 键布局          |
| 旋钮款 | 2      | 4 键     | 7        | 4 键 + 旋钮 (3 虚拟键) |

::: tip 旋钮款键位说明
旋钮款的 7 个虚拟键位：
- `[0-3]` 普通按键
- `[4]` 旋钮顺时针旋转
- `[5]` 旋钮逆时针旋转  
- `[6]` 旋钮按下
:::

### 选择键盘布局

在 `Board/include/kbd_config.h` 中取消注释对应的布局：

```c
// 选择一个键盘布局 (只能启用一个)
// #define KBD_LAYOUT_BASIC    // 基础款: 4 键
#define KBD_LAYOUT_5KEY       // 五键款: 5 键
// #define KBD_LAYOUT_KNOB     // 旋钮款: 4 键 + 旋钮
```

### 获取键盘信息

```c
// 获取当前键盘类型
kbd_type_t type = KBD_GetType();          // KBD_TYPE_BASIC / 5KEYS / KNOB

// 获取总键位数 (用于映射)
uint8_t total = KBD_GetTotalKeyCount();   // 4 / 5 / 7

// 获取物理按键数
uint8_t physical = KBD_GetPhysicalKeyCount();  // 4 / 5 / 4
```

### GPIO 引脚映射

修改 `kbd_config.h` 中对应布局的引脚定义：

```c
// 普通按键引脚
#define KBD_K1_PORT GPIO_PORT_B
#define KBD_K1_PIN  GPIO_Pin_4

// FN 功能键
#define KBD_FN1_PORT GPIO_PORT_A
#define KBD_FN1_PIN  GPIO_Pin_4

// WS2812 数据引脚
#define WS2812_PORT GPIO_PORT_A
#define WS2812_PIN  GPIO_Pin_10

// 旋钮款额外引脚
#ifdef KBD_LAYOUT_KNOB
#define KBD_ENCODER_A_PORT  GPIO_PORT_A
#define KBD_ENCODER_A_PIN   GPIO_Pin_12
#define KBD_ENCODER_B_PORT  GPIO_PORT_A
#define KBD_ENCODER_B_PIN   GPIO_Pin_13
#endif
```

## 编译与烧录

### 使用 MounRiver Studio

1. 打开 `CH592F.wvproj`
2. 按 `F7` 编译
3. 编译产物在 `obj/CH592F.bin`

### 烧录固件

1. 断开键盘与电脑连接
2. 按住 **BOOT** 按钮的同时连接 USB
3. 打开 WCHISPStudio，选择 `CH592F.bin` 烧录

## 按键映射系统

按键映射定义在 `kbd_types.h`，核心结构层次为：

```
kbd_keymap_t (完整映射)
├── num_layers        // 实际层数 (1-4)
├── current_layer     // 当前激活层
├── default_layer     // 默认层
└── layers[4]         // 层数组
    └── kbd_layer_t   // 单层映射 (32 字节)
        └── keys[8]   // 按键数组
            └── kbd_action_t  // 单键动作 (4 字节)
```

### kbd_action_t - 单键动作结构

每个按键对应一个 4 字节的动作结构：

```c
typedef struct __attribute__((packed)) {
    uint8_t type;       // 动作类型 (见下表)
    uint8_t modifier;   // 修饰键或操作类型
    uint8_t param1;     // 参数 1
    uint8_t param2;     // 参数 2
} kbd_action_t;
```

#### 动作类型与字段含义

| type 值 | 动作类型 | modifier   | param1           | param2          |
| :------ | :------- | :--------- | :--------------- | :-------------- |
| 0x00    | 无动作   | -          | -                | -               |
| 0x01    | 键盘按键 | 修饰键掩码 | HID 键码         | -               |
| 0x02    | 鼠标按键 | -          | 按键掩码         | -               |
| 0x03    | 鼠标滚轮 | -          | 方向 (1=上 2=下) | -               |
| 0x04    | 多媒体键 | -          | Usage ID 低字节  | Usage ID 高字节 |
| 0x05    | 执行宏   | -          | 宏 ID (0-7)      | -               |
| 0x06    | 层切换   | 操作类型   | 层号             | -               |

#### 修饰键掩码

```c
#define KBD_MOD_LCTRL   0x01    // 左 Ctrl
#define KBD_MOD_LSHIFT  0x02    // 左 Shift
#define KBD_MOD_LALT    0x04    // 左 Alt
#define KBD_MOD_LGUI    0x08    // 左 GUI (Win/Cmd)
#define KBD_MOD_RCTRL   0x10    // 右 Ctrl
#define KBD_MOD_RSHIFT  0x20    // 右 Shift
#define KBD_MOD_RALT    0x40    // 右 Alt
#define KBD_MOD_RGUI    0x80    // 右 GUI
```

#### 鼠标按键掩码

```c
#define KBD_MOUSE_LEFT      0x01    // 左键
#define KBD_MOUSE_RIGHT     0x02    // 右键
#define KBD_MOUSE_MIDDLE    0x04    // 中键
#define KBD_MOUSE_BACK      0x08    // 后退键
#define KBD_MOUSE_FORWARD   0x10    // 前进键
```

#### 层操作类型

| 值   | 操作      | 说明               |
| :--- | :-------- | :----------------- |
| 0    | Momentary | 按住切换，松开恢复 |
| 1    | Toggle    | 切换开关           |
| 2    | Set       | 直接设置为指定层   |

### kbd_keymap_t - 完整映射结构

```c
typedef struct __attribute__((packed)) {
    uint8_t     num_layers;             // 实际使用层数 (1-4)
    uint8_t     current_layer;          // 当前激活层
    uint8_t     default_layer;          // 默认层
    uint8_t     reserved;
    kbd_layer_t layers[KBD_MAX_LAYERS]; // 4 层 × 32 字节 = 128 字节
} kbd_keymap_t;  // 总计 132 字节
```

### FN 键配置

FN 键独立于普通按键，支持短按和长按两种动作：

```c
typedef struct __attribute__((packed)) {
    uint8_t  click_action;      // 短按动作
    uint8_t  click_param;       // 短按参数
    uint8_t  long_action;       // 长按动作
    uint8_t  long_param;        // 长按参数
    uint16_t long_press_ms;     // 长按判定阈值 (毫秒)
    uint8_t  reserved[2];
} kbd_fnkey_entry_t;  // 8 字节
```

#### FN 动作类型

| 值   | 动作             | param 含义 |
| :--- | :--------------- | :--------- |
| 0x00 | 无动作           | -          |
| 0x01 | USB/BLE 模式切换 | -          |
| 0x02 | 开始蓝牙广播     | -          |
| 0x03 | 断开蓝牙连接     | -          |
| 0x04 | 清除所有配对信息 | -          |
| 0x10 | RGB 开关         | -          |
| 0x11 | 下一个灯效模式   | -          |
| 0x12 | 上一个灯效模式   | -          |
| 0x13 | 增加亮度         | -          |
| 0x14 | 降低亮度         | -          |
| 0x20 | 切换到下一层     | -          |
| 0x21 | 切换到上一层     | -          |
| 0x22 | 设置为指定层     | 层号       |
| 0x30 | 执行宏           | 宏 ID      |
| 0x40 | 进入睡眠模式     | -          |
| 0x41 | 进入 Bootloader  | -          |

### API 使用示例

```c
// 获取映射配置指针
kbd_keymap_t *keymap = KBD_GetKeymap();

// 切换层
KBD_SetCurrentLayer(1);
uint8_t layer = KBD_GetCurrentLayer();

// 获取当前层的按键动作
const kbd_action_t *action = KBD_GetKeyAction(key_index);

// 快速创建动作宏
kbd_action_t key_a = KBD_KEY(0, 0x04);                    // 按键 A
kbd_action_t ctrl_c = KBD_KEY(KBD_MOD_LCTRL, 0x06);       // Ctrl+C
kbd_action_t mouse_left = KBD_MOUSE(KBD_MOUSE_LEFT);      // 鼠标左键
kbd_action_t vol_up = KBD_CONSUMER(0xE9);                 // 音量+
kbd_action_t layer1 = KBD_LAYER(KBD_LAYER_TOGGLE, 1);     // 切换层 1
```

## RGB 灯效系统

### 灯效模式

```c
typedef enum {
    KBD_RGB_OFF         = 0,  // 关闭
    KBD_RGB_STATIC      = 1,  // 静态单色
    KBD_RGB_BREATHING   = 2,  // 呼吸效果
    KBD_RGB_BLINK       = 3,  // 闪烁效果
    KBD_RGB_RAINBOW     = 4,  // 彩虹渐变
    KBD_RGB_INDICATOR   = 5,  // 状态指示
} kbd_rgb_mode_t;
```

### 状态指示颜色

| 状态       | 颜色 | 效果   |
| :--------- | :--- | :----- |
| USB 已连接 | 白色 | 常亮   |
| 蓝牙未连接 | 红色 | 慢呼吸 |
| 蓝牙广播中 | 蓝色 | 呼吸   |
| 蓝牙已连接 | 绿色 | 常亮   |
| 低电量     | 红色 | 快闪   |

### API 使用

```c
// 设置模式
KBD_RGB_SetMode(KBD_RGB_BREATHING);

// 设置颜色
KBD_RGB_SetColor(255, 128, 0);  // 橙色

// 亮度控制
KBD_RGB_SetBrightness(128);     // 50% 亮度
KBD_RGB_BrightnessUp(16);       // 增加亮度

// 状态指示
KBD_RGB_SetState(KBD_STATE_BLE_CONNECTED);

// 临时闪烁反馈
KBD_RGB_Flash(0, 255, 0, 200);  // 绿色闪烁 200ms
```

## HID 通讯协议

### Report ID 分配

| Report ID | 功能        | 数据长度 | 说明                  |
| :-------- | :---------- | :------- | :-------------------- |
| 0         | 键盘输入    | 8 字节   | 修饰键 + 6 键同时按下 |
| 1         | 鼠标输入    | 4 字节   | 按键 + XY + 滚轮      |
| 2         | 多媒体键    | 2 字节   | Consumer Control      |
| 4         | 主机 → 键盘 | 64 字节  | 配置命令（仅 USB）    |
| 5         | 键盘 → 主机 | 64 字节  | 配置响应（仅 USB）    |

::: warning 注意
蓝牙模式下无法使用 Report ID 4/5 进行改键，需切换到 USB 模式。
:::

### 通用帧格式

所有配置命令和响应使用统一的帧格式 (64 字节)：

```
+--------+--------+--------+--------------------+
| CMD    | SUB    | LEN    | DATA (61 bytes)    |
| 1 byte | 1 byte | 1 byte | 0-61 bytes         |
+--------+--------+--------+--------------------+
```

| 字段 | 说明                                    |
| :--- | :-------------------------------------- |
| CMD  | 命令码 (请求时为命令，响应时回显)       |
| SUB  | 子命令或序号 (如层号、槽位号、分包序号) |
| LEN  | DATA 区有效数据长度                     |
| DATA | 命令参数或响应数据                      |

### 响应状态码

| 值   | 含义           |
| :--- | :------------- |
| 0x00 | 成功           |
| 0x01 | 无效命令       |
| 0x02 | 参数错误       |
| 0x03 | 设备忙         |
| 0x04 | Flash 操作失败 |
| 0x10 | 数据过大       |
| 0x11 | 存储空间不足   |
| 0x12 | 未找到目标     |

### 命令码列表

| 命令码 | 名称       | SUB 含义    | 说明               |
| :----- | :--------- | :---------- | :----------------- |
| 0x01   | SYS_INFO   | -           | 获取设备信息       |
| 0x02   | SYS_STATUS | -           | 获取运行状态       |
| 0x10   | CFG_SAVE   | -           | 保存配置到 Flash   |
| 0x11   | CFG_LOAD   | -           | 从 Flash 加载配置  |
| 0x12   | CFG_RESET  | -           | 恢复出厂设置       |
| 0x20   | KEYMAP_GET | 层号        | 获取指定层按键映射 |
| 0x21   | KEYMAP_SET | 层号        | 设置指定层按键映射 |
| 0x22   | LAYER_GET  | -           | 获取当前层信息     |
| 0x23   | LAYER_SET  | -           | 设置当前层         |
| 0x30   | RGB_GET    | -           | 获取 RGB 配置      |
| 0x31   | RGB_SET    | -           | 设置 RGB 配置      |
| 0x40   | MACRO_INFO | 槽位号/0xFF | 获取宏信息         |
| 0x41   | MACRO_GET  | 槽位号      | 读取宏数据 (分包)  |
| 0x42   | MACRO_SET  | 槽位号      | 写入宏数据 (分包)  |
| 0x43   | MACRO_DEL  | 槽位号      | 删除宏             |
| 0x50   | FNKEY_GET  | -           | 获取 FN 键配置     |
| 0x51   | FNKEY_SET  | -           | 设置 FN 键配置     |
| 0x60   | BATTERY    | -           | 获取电池信息       |

---

### 0x01 SYS_INFO - 获取设备信息

上位机连接后应首先发送此命令获取键盘硬件信息。

**请求**: 无参数

**响应** (14 字节):

| 偏移   | 长度 | 字段           | 说明                         |
| :----- | :--- | :------------- | :--------------------------- |
| 0      | 1    | 状态码         | 0x00 = 成功                  |
| 1      | 1    | VID 高字节     | USB Vendor ID                |
| 2      | 1    | VID 低字节     |                              |
| 3      | 1    | PID 高字节     | USB Product ID               |
| 4      | 1    | PID 低字节     |                              |
| 5      | 1    | 主版本         | 固件主版本号                 |
| 6      | 1    | 次版本         | 固件次版本号                 |
| 7      | 1    | 补丁版本       | 固件补丁版本                 |
| 8      | 1    | 最大层数       | 支持的最大层数 (4)           |
| 9      | 1    | 最大键数       | 单层最大键数 (8)             |
| 10     | 1    | 宏槽位数       | 宏存储槽位数 (8)             |
| **11** | 1    | **键盘类型**   | 0=基础款, 1=五键款, 2=旋钮款 |
| **12** | 1    | **实际键位数** | 当前类型的虚拟键位数 (4/5/7) |
| 13     | 1    | FN 键数量      | FN 功能键数量                |

::: tip 上位机适配
上位机应根据 `键盘类型` 和 `实际键位数` 字段动态调整 UI 显示和数据交换格式。
:::

---

### 0x02 SYS_STATUS - 获取运行状态

**请求**: 无参数

**响应** (6 字节):

| 偏移 | 字段     | 说明               |
| :--- | :------- | :----------------- |
| 0    | 状态码   | 0x00 = 成功        |
| 1    | 工作模式 | 0=USB, 1=BLE       |
| 2    | 连接状态 | 0=断开, 1=已连接   |
| 3    | 当前层   | 当前激活层号       |
| 4    | 电池电量 | 百分比 (0-100)     |
| 5    | 充电状态 | 0=未充电, 1=充电中 |

---

### 0x20 KEYMAP_GET - 获取按键映射

**请求**: SUB = 层号 (0-3)

**响应** (4 + 32 字节):

| 偏移 | 长度 | 字段   | 说明                        |
| :--- | :--- | :----- | :-------------------------- |
| 0    | 1    | 状态码 | 0x00 = 成功                 |
| 1    | 1    | 总层数 | 实际使用的层数              |
| 2    | 1    | 当前层 | 当前激活层                  |
| 3    | 1    | 默认层 | 默认层号                    |
| 4    | 32   | 层数据 | kbd_layer_t (8个按键×4字节) |

**层数据解析**:
```
偏移 4+0:  按键 0 的 kbd_action_t (4 字节)
偏移 4+4:  按键 1 的 kbd_action_t (4 字节)
...
偏移 4+28: 按键 7 的 kbd_action_t (4 字节)
```

---

### 0x21 KEYMAP_SET - 设置按键映射

**请求**: SUB = 层号 (0-3)

| 偏移 | 长度 | 字段   | 说明               |
| :--- | :--- | :----- | :----------------- |
| 0    | 1    | 总层数 | 设置实际使用的层数 |
| 1    | 1    | (保留) |                    |
| 2    | 1    | 默认层 | 设置默认层号       |
| 3    | 32   | 层数据 | kbd_layer_t 数据   |

**响应** (1 字节): 状态码

---

### 0x30 RGB_GET - 获取 RGB 配置

**请求**: 无参数

**响应** (9 字节):

| 偏移 | 字段       | 说明             |
| :--- | :--------- | :--------------- |
| 0    | 状态码     | 0x00 = 成功      |
| 1    | enabled    | RGB 总开关       |
| 2    | mode       | 灯效模式         |
| 3    | brightness | 亮度 (0-255)     |
| 4    | speed      | 动画速度 (0-255) |
| 5    | color_r    | 静态颜色 R       |
| 6    | color_g    | 静态颜色 G       |
| 7    | color_b    | 静态颜色 B       |
| 8    | indicator  | 状态指示开关     |

---

### 0x31 RGB_SET - 设置 RGB 配置

**请求** (8 字节):

| 偏移 | 字段       | 说明         |
| :--- | :--------- | :----------- |
| 0    | enabled    | RGB 总开关   |
| 1    | mode       | 灯效模式     |
| 2    | brightness | 亮度         |
| 3    | speed      | 动画速度     |
| 4    | color_r    | 静态颜色 R   |
| 5    | color_g    | 静态颜色 G   |
| 6    | color_b    | 静态颜色 B   |
| 7    | indicator  | 状态指示开关 |

**响应** (1 字节): 状态码

---

### 0x50 FNKEY_GET - 获取 FN 键配置

**请求**: 无参数

**响应** (1 + 32 字节):

| 偏移 | 长度 | 字段    | 说明                           |
| :--- | :--- | :------ | :----------------------------- |
| 0    | 1    | 状态码  | 0x00 = 成功                    |
| 1    | 32   | FN 配置 | kbd_fnkey_config_t (4个×8字节) |

---

### 0x51 FNKEY_SET - 设置 FN 键配置

**请求** (32 字节): kbd_fnkey_config_t 数据

**响应** (1 字节): 状态码

---

### 0x42 MACRO_SET - 写入宏数据 (分包传输)

宏数据可能较大，需要分包传输：

**序号 0 - 开始写入**:

| 偏移 | 长度 | 字段   | 说明               |
| :--- | :--- | :----- | :----------------- |
| 0    | 1    | seq    | 固定为 0           |
| 1    | 24   | header | kbd_macro_header_t |

**序号 1~254 - 数据块**:

| 偏移 | 长度 | 字段     | 说明           |
| :--- | :--- | :------- | :------------- |
| 0    | 1    | seq      | 包序号 (1-254) |
| 1    | 1    | offset高 | 数据偏移高字节 |
| 2    | 1    | offset低 | 数据偏移低字节 |
| 3    | 1    | len      | 数据长度       |
| 4    | N    | data     | 宏动作数据     |

**序号 0xFF - 完成写入**:

| 偏移 | 字段 | 说明        |
| :--- | :--- | :---------- |
| 0    | seq  | 固定为 0xFF |

**响应**: [状态码, seq]

## 工作模式

### 模式类型

| 值 | 模式 | 说明 |
|----|------|------|
| 0 | USB | USB 有线模式 |
| 1 | BLE | 蓝牙无线模式 |
| 2 | 2.4G | 2.4G 无线模式（预留，暂未实现） |

### 模式管理

```c
// 初始化模式管理器
KBD_Mode_Init(KBD_WORK_MODE_USB, &callbacks);

// 切换模式
KBD_Mode_Toggle();
KBD_Mode_Switch(KBD_WORK_MODE_BLE);
// KBD_Mode_Switch(KBD_WORK_MODE_2G4);  // 预留：2.4G 模式

// 获取当前模式
kbd_work_mode_t mode = KBD_Mode_Get();

// 检查连接状态
bool connected = KBD_Mode_IsConnected();
```

### 蓝牙控制

```c
// 开始广播
KBD_Mode_BLE_StartAdvertising();

// 断开连接
KBD_Mode_BLE_Disconnect();

// 清除配对信息
KBD_Mode_BLE_ClearBonds();
```

### 2.4G 模式（预留）

::: info 预留功能
2.4G 无线模式已在代码和数据结构中预留（`KBD_WORK_MODE_2G4 = 2`），但当前版本暂未实现。

**预留位置：**
- `kbd_mode_config.h`: `kbd_work_mode_t` 枚举中已注释预留
- `dataflash.md`: `default_mode` 字段支持值 2（2.4G）
- 模式管理器架构支持扩展新工作模式

**实现方案：**
CH592F 芯片内置 2.4G 射频功能，可直接实现 2.4G 无线通信，无需外部模块。计划支持两种工作方式：

1. **键盘作为接收器**：键盘本身可工作在接收器模式，通过 USB 连接电脑，接收其他 2.4G 键盘的信号
2. **独立接收器**：使用 CH592F 设计专用接收器（无按键等外设，仅无线接收功能），通过 USB 连接电脑

**未来实现方向：**
- 实现 CH592F 内置 2.4G 射频驱动
- 实现 2.4G 专用的 HID 报告发送接口
- 添加 2.4G 连接状态管理（配对、连接、断开等）
- 扩展 `KBD_Mode_Switch()` 支持 2.4G 模式切换
- 支持键盘/接收器两种角色的切换
:::

### 发送 HID 报告

```c
// 键盘报告
uint8_t keys[6] = {KEY_A, KEY_B, 0, 0, 0, 0};
KBD_Mode_SendKeyboardReport(KEY_MOD_LCTRL, keys, 2);

// 鼠标报告
KBD_Mode_SendMouseReport(MOUSE_BTN_LEFT, 10, -5, 0);

// 多媒体键
KBD_Mode_SendConsumerReport(0xCD);  // Play/Pause
```

## 配置存储

### DataFlash 布局

CH592F 内置 32KB DataFlash：

| 地址范围        | 大小 | 用途      |
| :-------------- | :--- | :-------- |
| 0x00000-0x000FF | 256B | 配置头    |
| 0x00100-0x001FF | 256B | 系统配置  |
| 0x00200-0x002FF | 256B | 按键映射  |
| 0x00300-0x0033F | 64B  | FN 键配置 |
| 0x00340-0x003FF | 192B | RGB 配置  |
| 0x04000-0x07FFF | 16KB | 宏数据区  |

### API 使用

```c
// 初始化存储
KBD_Storage_Init();

// 保存配置
KBD_Config_Save();

// 恢复出厂设置
KBD_Config_Reset();

// 获取配置指针
kbd_keymap_t *keymap = KBD_GetKeymap();
kbd_rgb_config_t *rgb = KBD_GetRgbConfig();
```

## 低功耗设计

### 睡眠配置

```c
// kbd_mode_config.h
#define KBD_LOW_POWER_ENABLE        1
#define KBD_IDLE_SLEEP_TIMEOUT_MS   30000  // 30 秒无操作进入休眠
#define KBD_WAKEUP_BY_KEY           1      // 按键唤醒
```

### 进入/退出睡眠

```c
// 手动进入睡眠
KBD_Mode_EnterSleep();

// 检查睡眠状态
if (KBD_Mode_IsInSleep()) {
    // 处理睡眠状态
}
```

## 常见改动点

### 1. 修改默认键位

编辑 `kbd_storage.c` 中的默认键位映射。

### 2. 修改 RGB 灯效

编辑 `kbd_rgb.c` 添加新的灯效模式。

### 3. 修改蓝牙名称

在 `kbd_mode_config.h` 中修改：

```c
#define KBD_BLE_DEVICE_NAME     "MyKeyboard"
#define KBD_BLE_DEVICE_NAME_LEN 10
```

### 4. 修改 USB VID/PID

在 `kbd_mode_config.h` 中修改：

```c
#define KBD_USB_VENDOR_ID   0x413D
#define KBD_USB_PRODUCT_ID  0x2107
```

## 参考资料

- [CH592 数据手册](https://www.wch.cn/products/CH592.html)
- [MounRiver Studio](http://www.mounriver.com/)
- [WCH BLE SDK 文档](https://www.wch.cn/products/CH592.html)
