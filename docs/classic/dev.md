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

### 首次推荐流程

```bash
# 1. 先让脚本探测并缓存工具路径
python tools/scripts/ch552g.py status --keyboard KNOB

# 2. 再正式配置 / 构建
python tools/scripts/ch552g.py configure --keyboard KNOB
python tools/scripts/ch552g.py build --keyboard KNOB

# 3. 最后再烧录
python tools/scripts/flash.py flash --file firmware/CH552G/build/knob/CH552G-KNOB-<version>.bin
```

### 工具链查找规则

CH552G 的构建脚本和 CMake toolchain 会优先走环境变量，其次使用已缓存的工具路径，最后才回退到 `PATH` 和常见安装目录。

缓存文件位置：

- `tools/scripts/.binarykeyboard_console_state.json`

当前会缓存的工具包括：

- `cmake`
- `ninja`
- `sdcc`
- `wchisp`
- `riscv_gcc`

CH552G 实际直接依赖的查找顺序：

1. `SDCC_PATH`
2. `tools/scripts/.binarykeyboard_console_state.json`
3. `PATH`
4. 常见本机安装目录

如果你想手动指定，也可以直接传：

```bash
SDCC_PATH=/path/to/sdcc python tools/scripts/ch552g.py build --keyboard BASIC
```

### CMake 构建结构

这一轮构建链已经整理成下面四层：

- `firmware/cmake/BinaryKeyboardFirmware.cmake`
  共享版本头生成 helper，`CH552G` / `CH592F` 复用同一套逻辑
- `firmware/CH552G/CMakeLists.txt`
  CH552G 固件目标本体，按运行时 / 应用层 / SDK / WS2812 分组组织源文件
- `firmware/CH552G/cmake/toolchain-ch552.cmake`
  SDCC toolchain，负责 `sdcc / sdas8051 / packihx / makebin` 和 CH552 内存布局
- `firmware/CMakeLists.txt`
  顶层 superbuild，统一暴露 `ch552_basic`、`ch552_knob`、`ch552_5keys`、`ch552_all` 等目标

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

### CMake 选择方式

推荐通过脚本选择：

```bash
python tools/scripts/ch552g.py build --keyboard BASIC
python tools/scripts/ch552g.py build --keyboard KNOB
python tools/scripts/ch552g.py build --keyboard 5KEY
```

如果你想直接使用 CMake，也可以显式传 `KEYBOARD`：

```bash
cmake -S firmware/CH552G -B firmware/CH552G/build/basic -G Ninja -DKEYBOARD=BASIC
cmake --build firmware/CH552G/build/basic
```

如果 Windows 上 `Ninja` 不在 `PATH`，可以显式传：

```powershell
cmake -S firmware/CH552G -B firmware/CH552G/build/basic `
  -G Ninja `
  -DCMAKE_MAKE_PROGRAM=C:/path/to/ninja.exe `
  -DKEYBOARD=BASIC
```

### 顶层 Superbuild

如果你想把固件构建入口统一交给顶层 `firmware/`：

```bash
cmake -S firmware -B firmware/build/dev
cmake --build firmware/build/dev --target ch552_knob
cmake --build firmware/build/dev --target ch552_all
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

- 单层键位映射存储
- `FUNC` 本地 RGB 交互
- USB HID 改键配置读写
- Studio 侧单层配置同步
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
Report ID `4/5` 用于 BinaryKeyboard Studio 与键盘通信，负责配置读取、键位读写和元信息同步。
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

### `sdcpp.exe: fatal error: cannot execute 'cc1'`

如果你是直接手敲 `cmake --build` 或 `ninja`，而不是走 `tools/scripts/ch552g.py`，在某些 Windows SDCC 包上会撞到这个错误。

原因：

- 这类 SDCC 包里的 `sdcpp.exe` 需要 `COMPILER_PATH` 指向 `SDCC/bin`

解决方式：

- 推荐直接使用 `python tools/scripts/ch552g.py build --keyboard KNOB`
- 或在当前终端手动设置：

```powershell
$env:COMPILER_PATH='C:\App\Environment\SDCC\bin'
$env:PATH='C:\App\Environment\SDCC\bin;' + $env:PATH
```

### `ninja --version` / `operation not permitted`

如果 CMake 默认捡到了 `C:\Users\<you>\AppData\Local\Microsoft\WinGet\Links\ninja.exe`，某些环境里会出现不可执行问题。

解决方式：

- 优先使用 `tools/scripts/ch552g.py`
- 或显式指定 `CMAKE_MAKE_PROGRAM`
- 或将可执行的 `ninja.exe` 路径写入 `NINJA_PATH`

### 老 build 目录里还是 `.obj`

新的 CH552G toolchain 已经把目标文件后缀切成 `.rel`。  
如果你之前生成过旧的 `.obj` 目录，需要清理后重新 configure：

```bash
python tools/scripts/ch552g.py clean --keyboard KNOB
python tools/scripts/ch552g.py configure --keyboard KNOB
```

## 参考资料

- [CH552 数据手册](https://www.wch.cn/products/CH552.html)
- [SDCC 文档](http://sdcc.sourceforge.net/)
- [ch55xduino](https://github.com/DeqingSun/ch55xduino)
