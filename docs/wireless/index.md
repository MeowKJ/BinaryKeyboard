# 无线版快速开始

基于 **CH592F** 芯片的蓝牙低功耗版本，无线自由。

::: tip 先看使用方式
如果你已经有成品键盘，优先阅读 [使用指南：无线版 5KEY / KNOB](../usage.md#无线版-5key)。这页主要覆盖复刻、首刷、配对和首次配置流程。
:::

## 支持型号

| 型号 | 输入 | 默认动作 | 层数 | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| 5KEY | 5 个普通键 | `1` / `2` / `3` / `4` / `5` | 5 | USB / BLE 双模五键键盘 |
| KNOB | 4 键 + 旋钮 | `1`~`4`，旋钮为音量加 / 音量减 / 静音 | 4 | USB / BLE 双模旋钮键盘 |

无线版通用操作：

- `FN1` 短按或长按：切换 USB / BLE 模式，切换时设备会重启进入目标模式。
- `FN2` 短按：切换到下一层。
- `FN2` 长按：在 BLE 模式下清除配对信息并重启。
- 按住任意 FN 键再按普通键：切换到对应层。
- 单击 `BOOT`：进入 IAP / Bootloader，用于恢复或调试。

## 准备工作

### 你需要

| 工具/材料              | 说明                                 |
| :--------------------- | :----------------------------------- |
| 🔧 热风枪/加热台 + 锡膏 | 焊接 PCB                             |
| 🖨️ 3D 打印机            | 或找人代打印                         |
| 💻 电脑                 | 用于连接键盘和刷写固件               |
| 🔋 锂电池               | 3.7V，根据外壳大小选择容量           |
| 🔌 USB-C 数据线         | 充电和刷固件                         |

### 下载文件

1. **固件** - 从 [GitHub Releases](https://github.com/MeowKJ/BinaryKeyboard/releases) 下载
   - 首次烧录使用 `CH592F-<MODEL>-<version>-full.hex`
   - 后续通过 Studio 热更新使用 `CH592F-<MODEL>-<version>-app.bin`

2. **外壳 STL** - 从 Releases 或 OSHWHub 下载（无线版专用，带电池仓）

3. **初次下载**
   - Windows 推荐使用 [WCHISPStudio](https://www.wch.cn/downloads/WCHISPTool_Setup_exe.html)
   - macOS / Linux 推荐使用喵喵的终端工具：`python tools/scripts/console.py`

## 复刻流程

### Step 1：3D 打印外壳

- 普通 FDM 打印即可。
- 层高 0.2mm

### Step 2：焊接 PCB

1. 按照原理图焊接元器件。
2. 焊接按键/轴体

::: danger 注意
- CH592F 芯片方向不要焊反！
- **电池正负极不要接反！** 可能导致芯片烧毁
- 焊锡不要沾到天线区域
- QFN封装的芯片焊接后仔细检查是否有虚焊或桥连
:::

### Step 3：刷写固件

#### Windows

1. 打开 **WCHISPStudio**
2. 顶部工具栏选择 **低功耗蓝牙系列** → **CH57x-CH59x**
3. 芯片系列选择 **CH59x**，芯片型号选择 **CH592**
4. 目标程序文件选择对应型号的 `-full.hex`

#### macOS / Linux

1. 在仓库根目录运行 `python tools/scripts/console.py`
2. 进入 `Home`
3. 点 `Install or update wchisp`
4. 点 `Probe ISP devices`
5. 五键款保持 `Layout = 5KEY`，旋钮款先点 `Toggle layout`

#### 硬件操作

6. **按住** PCB 上的 **BOOT** 按钮不松开
7. 保持按住的同时，将 USB-C 插入电脑
8. 松开 BOOT 按钮（此时软件应识别到设备）

#### 开始烧录

9. Windows 点击 **下载**
10. macOS / Linux 回到喵喵的终端工具，点 `Flash selected preset`
11. 等待刷写完成

详细步骤见 [刷写固件](./flash)

### Step 4：组装

1. 安装轴体/按键
2. 连接锂电池
3. 将 PCB 和电池装入外壳
4. 盖上盖板

### Step 5：蓝牙配对

1. 确认电池已连接且电量足够。
2. 短按 `FN1` 切换到 BLE 模式，键盘会重启。
3. 等待键盘进入广播状态。
4. 打开电脑或手机蓝牙设置，搜索：
   - `BinaryKeyboard5KEY`
   - `BinaryKeyboardKNOB`
5. 点击连接并等待系统完成配对。

::: tip 重新配对
如果之前配对过但现在无法连接，在 BLE 模式下长按 `FN2` 清除配对；同时在电脑 / 手机蓝牙列表里删除旧设备，再重新搜索。
:::

### Step 6：配置键位（可选）

1. 用 USB-C 连接键盘到电脑。
2. 如果键盘当前在 BLE 模式，短按 `FN1` 切回 USB 模式。
3. 用 Chrome / Edge 打开 BinaryKeyboard Studio。
4. 点击"连接设备"，选择你的键盘。
5. 配置键位、层、FN、RGB 或宏。
6. 点击保存 / 写入，等待提示成功。

详细步骤见 [改键软件使用](./remap)

## 完成 🎉

打开蓝牙并完成连接。

下一步可以继续看：

- [完整使用指南](../usage.md)
- [无线版改键软件](./remap.md)
- [无线版固件开发](./dev.md)

::: tip 省电
- 不用时可以关闭键盘电源开关
- 键盘会在闲置后自动进入低功耗模式
:::

::: tip 遇到问题？
查看 [常见问题](/faq) 或在 GitHub 提交 Issue。
:::
