# 无线版固件开发

基于 **CH592F** 芯片的 USB/蓝牙双模键盘固件开发指南。

::: warning
`MounRiver Studio` 开发时，导入工程后配置编译目录和 include 即可，这一页的 CMake 环境配置可以跳过。
:::

**仓库流程**：开发在 `dev` 分支进行，完成后 PR 到 `main`。

## 开发环境

> 当前构建流程使用 CMake，可配合 VS Code、CLion 等 IDE 使用。

> 当前发布流程基于 CMake 构建，CI 会按不同布局自动编译、校验并生成发布产物。

### 常用环境

| 工具 | 说明 |
| :--- | :--- |
| CMake ≥ 3.21 + Ninja | 构建系统 |
| MRS Toolchain | RISC-V 交叉编译工具链 |
| Python 3 | 构建 / 烧录 / console 脚本入口 |
| Node.js | Studio 环境 |
| wchisp | 底层烧录工具（`tools/scripts/setup.py` 自动下载） |


### 配置开发环境

## Windows

1. 安装 [MounRiver Studio](http://www.mounriver.com/)（主要是拿工具链）
2. 任选一种方式让仓库能找到工具链：
   - 运行 `python tools/scripts/console.py`，在 CH592 页面点一次 `Configure toolchain`
   - 设置环境变量 `MRS_TOOLCHAIN_ROOT`
   - 或把 `riscv-none-embed-gcc` / `riscv-wch-elf-gcc` 放进 `PATH`
3. 下载烧录工具：`python tools/scripts/setup.py`
4. 可选：启动统一控制台：`python tools/scripts/console.py`

## MacOS/Linux
> MacOS 目前只支持 M 系列芯片，Linux 只支持 x64 架构。
> 树莓派, 香橙派, 泰山派等 ARM 设备，目前无法原生编译 CH592F 固件 - WCH没有发布对应工具链。

1. 安装 [MounRiver Studio](http://www.mounriver.com/) 。
2. 下载 [RISC-V 工具链](http://www.mounriver.com/)
3. 任选一种方式让仓库能找到工具链：
   - 运行 `python tools/scripts/console.py`，在 CH592 页面点一次 `Configure toolchain`
   - 设置环境变量 `MRS_TOOLCHAIN_ROOT`
   - 或把 `riscv-none-embed-gcc` / `riscv-wch-elf-gcc` 放进 `PATH`
4. 下载烧录工具：`python tools/scripts/setup.py`
5. 可选：启动统一控制台：`python tools/scripts/console.py`

## 通用
相关辅助工具见：

- [便捷开发工具](./dev-tools.md)

不使用 `CMake` 时，这一段可以跳过。

可选工具链配置方式（`cmake/toolchain-ch59x.cmake` 已支持）：
- `MRS_TOOLCHAIN_ROOT`（填写 MounRiver Toolchain 根目录）
- `RISCV_TOOLCHAIN_DIR` / `TOOLCHAIN_DIR`（直接指定 `bin` 目录）
- 将 `riscv-none-embed-gcc` / `riscv-wch-elf-gcc` 加入系统 `PATH`

### 首次流程

安装好工具链后，直接启动统一控制台开始开发：

```bash
python tools/scripts/console.py
```

手动命令行流程如下：

```bash
# 1. 先探测工具链与缓存路径
python tools/scripts/ch592f.py status --keyboard KNOB --profile release

# 2. 构建首刷整包
python tools/scripts/ch592f.py build-full --keyboard KNOB --profile release

# 3. 查看导出产物
python tools/scripts/ch592f.py artifact --keyboard KNOB --profile release --type full_hex

# 4. 首刷 / 恢复
python tools/scripts/flash.py flash --file firmware/CH592F/build/release-knob/CH592F-KNOB-<version>-full.hex
```

### 发布产物

`CH592F` 发布以下几类产物：

| 文件 | 用途 |
| :--- | :--- |
| `CH592F-<MODEL>-<version>-full.hex` | **首次 ISP 烧录 / 救砖恢复**。包含 `JumpIAP + app + 高地址 IAP` |
| `CH592F-<MODEL>-<version>-full.bin` | 和上面内容一致，只是二进制格式 |
| `CH592F-<MODEL>-<version>.bin` | **Studio 在线更新 / OTA** 用的 app 包 |
| `CH592F-<MODEL>-<version>.hex` | app 的 HEX 版本，调试或手动检查时用 |
| `CH592F-<MODEL>-<version>-iap.hex/.bin` | 高地址 IAP 程序单独产物 |

::: tip
- **WCHISP 首刷** 用 `-full.hex`
- **Studio 热更新** 用普通 `.bin`
:::

### IAP 结构

`CH592F` 使用高地址 IAP 结构：

- `0x00000 ~ 0x00FFF`：JumpIAP 跳板
- `0x01000 ~ 0x36FFF`：Image A，当前运行中的 app
- `0x37000 ~ 0x6CFFF`：Image B，Studio 下载的新固件暂存区
- `0x6D000 ~ 0x6FFFF`：高地址 IAP 程序

设备上电后先经过 `JumpIAP`，再决定是否执行 `B -> A` 搬运。

### 工具缓存

脚本层现在会把常用工具路径缓存到：

- `tools/scripts/.binarykeyboard_console_state.json`

当前缓存项包括：

- `cmake`
- `ninja`
- `sdcc`
- `wchisp`
- `riscv_gcc`

这意味着第一次探测完之后，后续 `console.py`、`ch592f.py`、`ch552g.py` 不需要每次重新扫磁盘。

### CMake 构建结构

当前固件构建已经整理成共享结构：

- `firmware/cmake/BinaryKeyboardFirmware.cmake`
  共享版本头生成 helper，`CH552G` / `CH592F` 复用
- `firmware/CH592F/CMakeLists.txt`
  CH592F 固件目标本体，按 hal / ble / keyboard / usb / SDK 分组
- `firmware/CH592F/cmake/toolchain-ch59x.cmake`
  MRS RISC-V toolchain 检测与平台参数
- `firmware/CMakeLists.txt`
  顶层 superbuild，可统一触发 `ch592_5key`、`ch592_knob`、`ch592_all`

## 代码架构

### 目录结构

```
Firmware/CH592F/
├── hal/                # 板级支持包 (BSP)
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
├── ble/                # 蓝牙协议栈
│   ├── core/           # BLE 核心
│   │   └── ble_mcu.c       # BLE 库初始化
│   ├── lib/            # WCH BLE 库 (预编译)
│   │   ├── CH59xBLE_LIB.h
│   │   └── libCH59xBLE.a
│   ├── hid/            # BLE HID
│   │   ├── ble_hid.c/h         # BLE HID 实现
│   │   ├── ble_hid_service.c/h # HID 服务
│   │   ├── kbd_mode.c/h        # 模式管理器
│   │   └── kbd_mode_config.h   # 模式配置
│   └── profile/        # BLE Profiles
│       ├── hiddev.c/h          # HID 设备
│       ├── battservice.c/h     # 电池服务
│       └── devinfoservice.c/h  # 设备信息服务
│
├── keyboard/           # 键盘核心逻辑
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
├── usb/                # USB HID 模块
│   ├── include/
│   │   ├── usb_hid.h       # USB HID 接口
│   │   ├── usb_device.h    # USB 设备管理
│   │   └── usb_descriptors.h
│   └── src/
│       ├── usb_hid.c       # USB HID 实现
│       ├── usb_device.c    # USB 设备初始化
│       └── usb_descriptors.c
│
├── SDK/                # 厂商SDK (只读)
│   ├── StdPeriphDriver/    # CH592 外设驱动
│   ├── RVMSIS/             # RISC-V 核心支持
│   ├── Startup/            # 启动文件
│   └── Ld/                 # 链接脚本
│
└── app/
    └── Main.c          # 主程序入口
```

### 核心模块说明

#### 1. hal - 板级支持

| 文件           | 功能                                 |
| :------------- | :----------------------------------- |
| `kbd_config.h` | GPIO 引脚定义、键盘布局配置          |
| `key.c/h`      | 中断驱动按键，lockout 去抖，事件队列 |
| `ws2812.c/h`   | PWM+DMA 驱动 WS2812，支持亮度调节    |

#### 2. keyboard - 键盘核心

| 文件            | 功能                                  |
| :-------------- | :------------------------------------ |
| `kbd_core.c`    | 按键事件处理、FN 键逻辑、HID 报告生成 |
| `kbd_types.h`   | 所有数据结构定义（动作、层、宏等）    |
| `kbd_storage.c` | DataFlash 读写、配置持久化            |
| `kbd_rgb.c`     | RGB 灯效：静态/呼吸/彩虹/状态指示     |

#### 3. ble - 蓝牙模块

| 文件                | 功能                           |
| :------------------ | :----------------------------- |
| `kbd_mode.c`        | USB/BLE/2.4G 多模切换、连接状态管理（2.4G 预留） |
| `ble_hid.c`         | 蓝牙 HID 报告发送              |
| `ble_hid_service.c` | HID GATT 服务实现              |

#### 4. usb - USB 模块

| 文件           | 功能                     |
| :------------- | :----------------------- |
| `usb_hid.c`    | USB HID 报告发送         |
| `usb_device.c` | USB 设备初始化与中断处理 |

## 键盘类型与版本配置

### 支持的键盘类型

| 类型   | 枚举值 | 物理按键 | 虚拟键位 | 说明                   |
| :----- | :----- | :------- | :------- | :--------------------- |
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

CH592F 当前只保留 `5KEY` 与 `KNOB` 两种发布布局。构建时通过统一控制台切换 keyboard 即可，无需手动传参。

如果不走 CMake，而是使用 MRS 或其他 IDE，请在预处理宏中定义其一：

```c
KBD_LAYOUT_5KEY
KBD_LAYOUT_KNOB
```

### 获取键盘信息

```c
// 获取当前键盘类型
kbd_type_t type = KBD_GetType();          // KBD_TYPE_5KEYS / KBD_TYPE_KNOB

// 获取总键位数 (用于映射)
uint8_t total = KBD_GetTotalKeyCount();   // 5 / 7

// 获取物理按键数
uint8_t physical = KBD_GetPhysicalKeyCount();  // 5 / 4
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
#define WS2812_EN_PORT GPIO_PORT_A
#define WS2812_EN_PIN  GPIO_Pin_9

// 旋钮款额外引脚
#ifdef KBD_LAYOUT_KNOB
#define KBD_K1_PORT GPIO_PORT_B
#define KBD_K1_PIN  GPIO_Pin_7
#define KBD_K2_PORT GPIO_PORT_A
#define KBD_K2_PIN  GPIO_Pin_12
#define KBD_K3_PORT GPIO_PORT_B
#define KBD_K3_PIN  GPIO_Pin_12
#define KBD_K4_PORT GPIO_PORT_A
#define KBD_K4_PIN  GPIO_Pin_8
#define KBD_ENCODER_A_PORT  GPIO_PORT_B
#define KBD_ENCODER_A_PIN   GPIO_Pin_14
#define KBD_ENCODER_B_PORT  GPIO_PORT_B
#define KBD_ENCODER_B_PIN   GPIO_Pin_15
#define KBD_ENCODER_BTN_PORT GPIO_PORT_B
#define KBD_ENCODER_BTN_PIN  GPIO_Pin_4
#endif
```

## 编译与烧录

### 统一控制台

日常开发通过统一控制台完成，所有操作都在交互菜单中提供：

```bash
python tools/scripts/console.py
```

控制台已集成：

- target 切换（CH552G / CH592F）
- keyboard 切换（5KEY / KNOB）
- profile 切换（release / debug）
- build / flash / verify
- ISP / Doctor / Studio 入口

### 烧录

```bash
python tools/scripts/flash.py flash --file firmware/CH592F/build/release-knob/CH592F-KNOB-<version>-full.hex
```

### 进入 Bootloader 模式

1. 断开键盘与电脑连接
2. 按住 **BOOT** 按钮的同时连接 USB
3. 连接后即可执行烧录

### 脚本参数

脱离控制台直接调用脚本时：

```bash
# 首次使用：下载 wchisp 烧录工具
python tools/scripts/setup.py

# 构建首刷整包
python tools/scripts/ch592f.py build-full --keyboard 5KEY --profile release

# 查看产物
python tools/scripts/ch592f.py artifact --keyboard 5KEY --profile release --type full_hex

# ISP 首刷 / 恢复
python tools/scripts/flash.py flash --file firmware/CH592F/build/release-5key/CH592F-5KEY-<version>-full.hex

# Studio 在线更新使用的 app 包
python tools/scripts/ch592f.py artifact --keyboard 5KEY --profile release --type bin
```

### 直接使用 CMake

```bash
cd firmware/CH592F
cmake --preset release-5key
cmake --build --preset release-5key
```

常用预设：
- `release`：体积优先（`MinSizeRel`）
- `debug`：调试优先（`-Og -g3`）
- `release-5key` / `debug-5key`：五键款共享预设
- `release-knob` / `debug-knob`：旋钮款共享预设

常用本机构建方式：

```bash
cd firmware/CH592F
cmake --preset release-5key
cmake --build --preset release-5key
```

### CMake 变量总览

共享构建变量：

| 变量 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `KEYBOARD` | `5KEY` | 键盘型号，支持 `5KEY` / `KNOB` |
| `KBD_MODEL` | `AUTO` | 设备名后缀；`AUTO` 时跟随 `KEYBOARD` |
| `KBD_NAME_PREFIX` | `BinaryKeyboard` | 设备名前缀 |
| `KBD_DEVICE_NAME_OVERRIDE` | 空 | 设备完整名称覆盖，优先级最高 |
| `MRS_TOOLCHAIN_ROOT` | 空 | MRS Toolchain 根目录 |
| `RISCV_TOOLCHAIN_DIR` | 空 | 工具链 `bin` 目录（环境变量） |
| `TOOLCHAIN_DIR` | 空 | 工具链 `bin` 目录（环境变量或 cache） |
| `CMAKE_BUILD_TYPE` | `MinSizeRel` | 构建类型，常见为 `Debug` / `MinSizeRel` |

编译期布局宏：

| 宏 | 说明 |
| :--- | :--- |
| `KBD_LAYOUT_5KEY` | 五键款布局 |
| `KBD_LAYOUT_KNOB` | 旋钮款布局 |
| `KBD_MODEL_NAME` | 由 CMake 注入的设备型号字符串 |
| `KBD_DEVICE_NAME` | 由 CMake 注入的统一 USB/BLE 设备名 |

### 常用 CMake 命令清单

```bash
cd firmware/CH592F

# 1. 用共享 preset 配置和构建
cmake --preset release-5key
cmake --build --preset release-5key

# 2. 在通用 preset 上覆写布局和名称
cmake --preset release -DKEYBOARD=KNOB -DKBD_MODEL=KNOB
cmake --build --preset release

# 3. 自定义完整设备名
cmake --preset release -DKBD_DEVICE_NAME_OVERRIDE=BinaryKeyboardLab
cmake --build --preset release

# 4. 直接传工具链根目录
cmake --preset release-5key -DMRS_TOOLCHAIN_ROOT=/path/to/MRS_Toolchain/Toolchain
cmake --build --preset release-5key
```

### 顶层 Superbuild

如果你想在仓库顶层统一管理多固件目标：

```bash
cmake -S firmware -B firmware/build/dev
cmake --build firmware/build/dev --target ch592_knob
cmake --build firmware/build/dev --target ch592_all
```

### CMake 常见问题

**1. VS Code CMake Tools 报 `RISC-V cross-compiler not found`**

- 先检查当前 Configure Preset 是否为 `release-*` / `debug-*`
- 执行 `CMake: Delete Cache and Reconfigure`
- 检查统一控制台缓存的工具链路径是否有效
- 或使用 `MRS_TOOLCHAIN_ROOT` / `TOOLCHAIN_DIR` / `RISCV_TOOLCHAIN_DIR` / 系统 `PATH` 提供编译器

说明：
- 推荐将构建和烧录分离：
  `tools/scripts/ch592f.py` 负责 preset 构建，`tools/scripts/flash.py` 负责烧录现成产物
- VS Code CMake Tools 直接使用当前选中的共享 preset 即可，不需要 `local-*`

**2. `CMAKE_C_COMPILER ... is not a full path to an existing compiler tool`**

在 Windows 下如果 toolchain 路径里缺少 `.exe`，旧 cache 容易留下错误的编译器路径。

处理方式：

- 删除对应 build 目录后重新 configure
- 或执行：

```bash
python tools/scripts/ch592f.py clean --keyboard KNOB --profile release
python tools/scripts/ch592f.py build --keyboard KNOB --profile release
```

**3. `ninja --version` / `operation not permitted`**

某些环境里 `cmake -G Ninja` 会捡到 `WinGet Links` 目录下那个不可执行的 `ninja.exe`。

处理方式：

- 优先用 `tools/scripts/ch592f.py`
- 或显式指定 `CMAKE_MAKE_PROGRAM`
- 或把可执行的 `ninja.exe` 写入 `NINJA_PATH`

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

> 当前准确协议定义（含 Studio/WebHID 逐字节格式、宏分包、日志异步帧）以 `docs/wireless/hid.md` 为准。本节保留为开发概览。

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
| 8      | 1    | 最大层数       | 支持的最大层数 (5)           |
| 9      | 1    | 最大键数       | 单层最大键数 (8)             |
| 10     | 1    | 宏槽位数       | 宏存储槽位数 (8)             |
| **11** | 1    | **键盘类型**   | 1=五键款, 2=旋钮款（0 保留） |
| **12** | 1    | **实际键位数** | 当前类型的虚拟键位数 (5/7)   |
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

CH592F 内置 32KB DataFlash，当前策略为“宏大擦写、配置小擦写”：

| 地址范围        | 大小 | 用途      |
| :-------------- | :--- | :-------- |
| 0x0000-0x0BFF | 3KB  | 配置槽轮转区（3 槽 × 1KB，按 256B 页差异擦写） |
| 0x0C00-0x0FFF | 1KB  | runtime 热数据区（4 页 × 256B，保存高频层号） |
| 0x1000-0x4FFF | 16KB | 宏数据区（按 4KB 块擦写） |
| 0x7E00-0x7EFF | 256B | BLE SNV（配对信息） |

说明：
- `current_layer` 高频变化仅写 runtime 热数据页，不重写整份配置
- 配置保存使用槽位轮转 + CRC 校验，降低磨损并提升掉电恢复能力
- 宏区保持块级擦写，简化大数据写入逻辑
- 高频状态建议通过 TMOS 延时事件合并写入，避免在按键路径直接擦写 Flash

详细布局与字段定义见 `docs/wireless/dataflash.md`。
如需进一步拆分为 `base/keymap/runtime` 分区日志页，可参考 `docs/wireless/dataflash.md` 末尾“推荐优化方案（规划）”。
TMOS 事件/定时/消息使用方式见 `docs/wireless/tmos.md`。

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
