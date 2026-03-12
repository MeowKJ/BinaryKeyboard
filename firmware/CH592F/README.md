# CH592F 固件

无线版（USB / 蓝牙双模）固件，基于 **CH592F** RISC-V 芯片。

## 本次大更新（构建侧）

- 完善 CMake 工作流：`CMakePresets.json` 提供 `debug` / `release` 预设。
- 新增 `CMakeUserPresets.json.example`：提供 `local-debug` / `local-release` 以及 `5KEY` / `KNOB` 变体（继承公共预设并写入本机工具链路径）。
- 新增通用 RISC-V 工具链探测：支持 `MRS_TOOLCHAIN_ROOT`、`RISCV_TOOLCHAIN_DIR`、`TOOLCHAIN_DIR` 或系统 `PATH`。
- `toolchain-ch59x.cmake` 兼容多种编译器前缀：`riscv-none-embed-`、`riscv-wch-elf-`、`riscv-none-elf-`。
- 工具链查找顺序优化：优先 `TOOLCHAIN_DIR`，找不到时回退系统 `PATH`（减少旧缓存路径导致的失败）。
- 默认使用 Ninja，并导出 `compile_commands.json`（便于 IDE 补全/跳转）。
- CH592F 键盘布局改为由 CMake / IDE 预处理宏注入，当前仅保留 `5KEY` 与 `KNOB` 两种发布配置。

## 文档

完整开发文档：[无线版开发指南](../../docs/wireless/dev.md)

如果你本来就是用 `MounRiver Studio` 开发，可以直接走它自己的工程和预处理宏，不用额外配 `CMake` 这一套。
`CMakePresets.json`、`CMakeUserPresets.json` 和下面这些命令，主要是给终端、VS Code 和其他 IDE 用的。

## 快速开始

```bash
cd firmware/CH592F

# 首次使用：配置本机工具链路径（推荐）
cp CMakeUserPresets.json.example CMakeUserPresets.json

# 首次使用：下载 wchisp 烧录工具
python tools/scripts/setup.py

# 构建
python tools/scripts/flash.py build --preset release

# 构建并烧录（先将键盘切换至 Bootloader 模式）
python tools/scripts/flash.py flash --preset release

# PY 终端控制台（固件 / 文档 / Studio / 工具链 / 链接）
python tools/scripts/console.py
```

## 手动构建（CMake）

```bash
cd firmware/CH592F
cmake --preset release
cmake --build --preset release
```

> 工具链路径优先推荐通过 `CMakeUserPresets.json` 指定（参见 `CMakeUserPresets.json.example`）。

### Presets 说明（推荐用法）

- `debug` / `release`：仓库共享预设（不包含本机工具链路径）
- `debug-5key` / `release-5key`：仓库共享 5KEY 型号预设（命名固定为 `BinaryKeyboard5KEY`）
- `debug-knob` / `release-knob`：仓库共享 KNOB 型号预设（命名固定为 `BinaryKeyboardKNOB`）
- `local-debug` / `local-release`：本机预设（在 `CMakeUserPresets.json` 中定义，继承共享预设）
- `local-debug-5key` / `local-release-5key`：本机 5KEY 型号预设（推荐日常使用）
- `local-debug-knob` / `local-release-knob`：本机 KNOB 型号预设

推荐：

```bash
cd firmware/CH592F
cmake --preset local-release-5key
cmake --build --preset local-release-5key
```

## 设备命名（USB/BLE 统一）

- 统一命名规则：`BinaryKeyboard{MODEL}`（`MODEL` 自动转大写）
- 当前 5 键款推荐：`BinaryKeyboard5KEY`
- 当前旋钮款推荐：`BinaryKeyboardKNOB`
- USB 产品字符串与 BLE 广播/GATT Device Name 使用同一名称来源

### 命名相关 CMake 变量

- `KBD_LAYOUT`：键盘布局（`5KEY` / `KNOB`）
- `KBD_MODEL`：型号后缀（默认 `AUTO`，跟随 `KBD_LAYOUT`；也可手动覆写成 `65KEY` 等）
- `KBD_NAME_PREFIX`：前缀（默认 `BinaryKeyboard`）
- `KBD_DEVICE_NAME_OVERRIDE`：完整名称覆盖（非空时优先）

示例：

```bash
# 旋钮款
cmake --preset local-release-knob
cmake --build --preset local-release-knob

# 自定义 5KEY 名称后缀
cmake --preset local-release -DKBD_LAYOUT=5KEY -DKBD_MODEL=65KEY
cmake --build --preset local-release

# 自定义完整名称（覆盖前缀+型号拼接）
cmake --preset local-release -DKBD_DEVICE_NAME_OVERRIDE=BinaryKeyboardPRO
cmake --build --preset local-release
```

### 布局宏与 IDE 兼容

- CMake 会自动注入 `KBD_LAYOUT_5KEY` 或 `KBD_LAYOUT_KNOB`
- MounRiver Studio / 其他 IDE 若不走 CMake，也可直接在预处理宏中定义同名宏
- 当前 CH592F 路径不再保留经典款宏配置

### 名称长度约束

- BLE GATT Device Name：最多 21 字节，超长自动截断
- BLE Scan Response 本地名称：最多 29 字节，超长自动短名广播
- USB Product String：最多 31 字符，超长自动截断

### VS Code CMake Tools 使用建议

- 5 键款优先选择 `local-release-5key` / `local-debug-5key` 作为 Configure Preset
- 修改预设或工具链路径后，执行 `CMake: Delete Cache and Reconfigure`
- 若提示 `RISC-V cross-compiler not found`，优先检查：
  - 当前是否选中了 `local-*` 预设
  - `CMakeUserPresets.json` 中 `MRS_TOOLCHAIN_ROOT` 是否正确
  - 或是否已通过 `TOOLCHAIN_DIR` / 系统 `PATH` 提供编译器
