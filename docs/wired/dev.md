# 经典版固件开发

基于 **CH552G** 芯片的 USB 直连版本固件开发指南。

::: tip
如果你只是想刷现成固件，可以直接看 [经典版固件刷写](./flash.md)。
:::

**仓库流程**：推荐在功能分支或 `dev` 分支开发，验证通过后再合并到 `main`。

## 开发环境

> CH552G 已统一到 CMake + Python 脚本工作流，推荐直接使用仓库里的 `console.py` 作为入口。

### 常用环境

| 工具 | 说明 |
| :--- | :--- |
| CMake ≥ 3.21 | 项目生成与构建入口 |
| Ninja | 推荐生成器 |
| SDCC | CH552G 8051 交叉编译器 |
| Python 3 | 构建、烧录、console 脚本入口 |
| wchisp | 底层烧录工具，`tools/scripts/setup.py` 可自动下载 |
| Node.js | BinaryKeyboard Studio 前端开发环境 |

### 配置开发环境

1. 安装 `CMake`
2. 安装 `Ninja`
3. 安装 `SDCC`

### 工具链查找规则

CH552G 的构建脚本和 CMake toolchain 会优先走环境变量，其次使用已缓存的工具路径，最后才回退到 `PATH` 和常见安装目录。

缓存文件位置：

- `tools/scripts/.binarykeyboard_console_state.json`

当前会缓存的工具包括：

- `cmake`
- `ninja`
- `sdcc`
- `wchisp`

CH552G 实际直接依赖的查找顺序：

1. `SDCC_PATH`
2. `tools/scripts/.binarykeyboard_console_state.json`
3. `PATH`
4. 常见本机安装目录

如果你想手动指定，也可以直接传：

```bash
SDCC_PATH=/path/to/sdcc python tools/scripts/ch552g.py build --keyboard BASIC
```
## 代码架构

### 目录结构

```text
firmware/CH552G/
├── cmake/                 # CH552G toolchain 与 CMake 辅助文件
├── SDK/                   # ch55xduino SDK
│   ├── cores/             # Arduino 运行时、USB 核心、EEPROM 等
│   ├── libraries/         # WS2812 驱动
│   └── variants/ch552/    # CH552 头文件与芯片定义
├── User/
│   └── Main.c             # 主循环入口
├── src/                   # 固件核心模块
│   ├── config.h           # 键盘型号、引脚、版本与常量
│   ├── KeyScanner.c       # 按键扫描与 FUNC 抑制
│   ├── KeysDataHandler.c  # 键位数据与 EEPROM
│   ├── LightingController.c
│   ├── rgb.c              # RGB 效果与本地灯效反馈
│   ├── CustomUSBHID.c     # USB HID vendor 协议
│   ├── USBHandler.c       # 键盘/鼠标/Consumer HID
│   └── EncoderHandler.c   # 旋钮型号专用处理
└── build/                 # 按 keyboard 分开的构建目录
    ├── basic/
    ├── knob/
    └── 5keys/
```

### 核心模块说明

| 文件 | 功能 |
| :--- | :--- |
| `config.h` | 键盘型号、GPIO 映射、层数、版本字段 |
| `KeyScanner.c` | 普通按键扫描、FUNC 抑制、按键事件分发 |
| `KeysDataHandler.c` | EEPROM 读写、默认键位、运行时配置 |
| `LightingController.c` | `FUNC` 单击/组合键交互、本地 RGB 控制 |
| `rgb.c` | RGB 灯效与本地反馈 |
| `CustomUSBHID.c` | Studio 通信命令、配置读写 |
| `USBHandler.c` | 键盘、鼠标、Consumer HID 报告 |
| `EncoderHandler.c` | `KNOB` 型号下的旋钮事件处理 |

## 键盘型号

CH552G 当前统一使用 `keyboard` 作为构建参数，而不是旧的 `variant`。

### 支持的型号

| keyboard | 说明 | 物理键数 | 特殊输入 |
| :------- | :--- | :------- | :------- |
| `BASIC` | 基础款 | 4 | 无 |
| `KNOB` | 旋钮款 | 4 主键 + 1 功能键 | 旋钮左 / 右 / 按下 |
| `5KEY` | 五键款 | 5 | 无 |

构建时通过统一控制台切换 keyboard 即可，无需手动传参。

## 编译与烧录

### 统一控制台（推荐）

日常开发推荐直接使用统一控制台，所有操作都可在交互菜单中完成：

```bash
python tools/scripts/console.py
```

控制台已集成：

- target 切换（CH552G / CH592F）
- keyboard 切换（BASIC / KNOB / 5KEY）
- build / flash / verify
- ISP / Doctor / Studio 入口

### 产物命名

CH552G 的导出产物统一命名为：

```text
CH552G-BASIC-<version>.bin
CH552G-BASIC-<version>.hex
CH552G-KNOB-<version>.bin
CH552G-5KEY-<version>.bin
```

构建目录仍按型号分开：

- `firmware/CH552G/build/basic`
- `firmware/CH552G/build/knob`
- `firmware/CH552G/build/5keys`

### 烧录

```bash
python tools/scripts/flash.py flash --file firmware/CH552G/build/basic/CH552G-BASIC-<version>.bin
python tools/scripts/flash.py verify --file firmware/CH552G/build/basic/CH552G-BASIC-<version>.bin
```

### 脚本参数

如果你需要脱离控制台直接调用脚本，也可以手动传参：

```bash
# 配置
python tools/scripts/ch552g.py configure --keyboard BASIC

# 构建
python tools/scripts/ch552g.py build --keyboard BASIC

# 查看产物路径
python tools/scripts/ch552g.py artifact --keyboard BASIC --type bin

# 查看当前构建状态
python tools/scripts/ch552g.py status --keyboard BASIC
```

### 直接使用 CMake

如果你想跳过脚本，直接用 CMake：

```bash
cmake -S firmware/CH552G -B firmware/CH552G/build/basic -G Ninja -DKEYBOARD=BASIC
cmake --build firmware/CH552G/build/basic
```

### 顶层 Superbuild

如果你想把固件构建入口统一交给顶层 `firmware/`：

```bash
cmake -S firmware -B firmware/build/dev
cmake --build firmware/build/dev --target ch552_knob
cmake --build firmware/build/dev --target ch552_all
```

## 当前固件能力

CH552G 当前已经支持：

- 单层键位映射存储
- `FUNC` 本地 RGB 交互
- USB HID 改键配置读写
- Studio 侧单层配置同步

### 关键入口

- `firmware/CH552G/src/LightingController.c`
- `firmware/CH552G/src/KeyScanner.c`
- `firmware/CH552G/src/KeysDataHandler.c`
- `firmware/CH552G/src/rgb.c`
- `firmware/CH552G/src/CustomUSBHID.c`

## HID 协议

经典版使用 USB HID 协议通信：

| Report ID | 功能 | 数据长度 |
| :-------- | :--- | :------- |
| `1` | 键盘输入 | 8 字节 |
| `2` | Consumer / 控制器 | 8 字节 |
| `3` | 鼠标输入 | 5 字节 |
| `4` | 主机 → 键盘 | 31 字节 |
| `5` | 键盘 → 主机 | 31 字节 |

::: tip
Report ID `4/5` 用于 BinaryKeyboard Studio 与键盘通信，负责配置读取、键位读写和元信息同步。:::


## 常见改动点

### 1. 默认键位

修改 `firmware/CH552G/src/KeysDataHandler.c`

### 2. RGB 灯效

修改 `firmware/CH552G/src/rgb.c`

### 3. `FUNC` 逻辑

修改 `firmware/CH552G/src/LightingController.c`

### 4. 按键扫描 / 去抖 / 抑制

修改 `firmware/CH552G/src/KeyScanner.c`

### 5. USB 描述符或协议

修改：

- `firmware/CH552G/src/USBConstant.c`
- `firmware/CH552G/src/CustomUSBHID.c`

### 6. 节奏游戏延迟

修改 `firmware/CH552G/src/config.h` 中的 `DEBOUNCE_THRESHOLD`


## 参考资料

- [CH552 数据手册](https://www.wch.cn/products/CH552.html)
- [SDCC 文档](http://sdcc.sourceforge.net/)
- [ch55xduino](https://github.com/DeqingSun/ch55xduino)
