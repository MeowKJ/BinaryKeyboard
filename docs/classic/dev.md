# 经典版固件开发

基于 **CH552G** 芯片的 USB 直连版本固件开发指南。

::: tip
如果你只是想刷现成固件，可以直接看 [经典版固件刷写](./flash.md)。这一页主要面向仓库内开发、改键协议调试和固件功能修改。
:::

**仓库流程**：推荐在功能分支或 `dev` 分支开发，验证通过后再合并到 `main`。

## 开发环境

> CH552G 这条线现在已经统一到 CMake + Python 脚本工作流，推荐直接使用仓库里的脚本入口，不要再手工切宏和复制产物。

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
4. 可选：运行 `python tools/scripts/setup.py` 下载 `wchisp`
5. 可选：运行 `python tools/scripts/console.py` 打开统一终端控制台

### 工具链查找规则

CH552G 的构建脚本和 CMake toolchain 会按下面顺序查找 SDCC：

1. `SDCC_PATH`
2. `PATH`
3. 常见本机安装目录

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
│   ├── KeysDataHandler.c  # 键位数据、多层与 EEPROM
│   ├── LightingController.c
│   ├── rgb.c              # RGB 效果与切层闪烁
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
| `KeyScanner.c` | 普通按键扫描、FUNC 抑制、按层查找动作 |
| `KeysDataHandler.c` | EEPROM 读写、多层 keymap、当前层状态 |
| `LightingController.c` | `FUNC` 单击/按住交互、切层入口 |
| `rgb.c` | RGB 灯效、切层闪烁反馈 |
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

### CMake 选择方式

推荐通过脚本选择：

```bash
python tools/scripts/ch552g.py build --keyboard BASIC
python tools/scripts/ch552g.py build --keyboard KNOB
python tools/scripts/ch552g.py build --keyboard 5KEY
```

如果你想直接使用 CMake，也可以显式传 `KEYBOARD`：

```bash
cmake -S firmware/CH552G -B firmware/CH552G/build/basic -DKEYBOARD=BASIC
cmake --build firmware/CH552G/build/basic
```

## 编译与烧录

### 一键脚本（推荐）

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

### 统一控制台

如果你不想手敲命令，可以直接用：

```bash
python tools/scripts/console.py
```

控制台里已经统一了：

- target 切换
- keyboard 切换
- build / flash / verify
- ISP / Doctor / Studio 入口

## 当前固件能力

CH552G 当前已经支持：

- 多层键位存储
- `FUNC` 单击切换 RGB 效果
- `FUNC` 按住 + 主键切层
- 切层 RGB 闪烁反馈
- Studio 侧多层读写
- `KNOB` 型号跟随当前层处理旋钮动作

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
Report ID `4/5` 用于 BinaryKeyboard Studio 与键盘通信，负责配置读取、层数据读写和元信息同步。
:::

## 本地编辑器支持

如果你使用 `clangd`，CH552G 目录下可以放一份本地兼容配置：

- `firmware/CH552G/.clangd`
- `firmware/CH552G/.clangd_intellisense.h`

它的作用只是让编辑器正确识别 `__data`、`__xdata`、`__code`、`__interrupt(...)` 这类 SDCC 关键字，**不参与真实编译**。

这两个文件当前按本地开发辅助文件处理，已加入 `.gitignore`，默认不提交。

如果你更新了这些本地配置，通常需要在 VS Code 中执行：

1. `Developer: Reload Window`
2. `clangd: Restart language server`

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

## 常见问题

### `types 'double', 'long double' not supported. Assuming 'float'`

这是 SDCC 对 8051 平台的正常警告，不是本项目新增问题。

### `internal RAM full`

CH552G 的内部 RAM 非常紧，这是当前平台限制。构建通过不代表余量很大，新增逻辑时要特别关注：

- `CH552G.ihx.mem`
- XRAM 使用量
- stack 剩余空间

### 改了 keyboard，但 build 目录还是旧型号

现在构建脚本已经会检查缓存型号，不匹配时自动重新 configure。  
如果你仍然怀疑缓存状态异常，可以直接：

```bash
python tools/scripts/ch552g.py clean --keyboard KNOB
python tools/scripts/ch552g.py build --keyboard KNOB
```

## 参考资料

- [CH552 数据手册](https://www.wch.cn/products/CH552.html)
- [SDCC 文档](http://sdcc.sourceforge.net/)
- [ch55xduino](https://github.com/DeqingSun/ch55xduino)
