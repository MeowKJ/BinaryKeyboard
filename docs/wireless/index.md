# 📡 无线版快速开始

基于 **CH592F** 芯片的蓝牙低功耗版本，无线自由。

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
   - 后续通过 Studio 热更新使用 `CH592F-<MODEL>-<version>.bin`

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

TODO

### Step 6：配置键位（可选）

TODO

详细步骤见 [改键软件使用](./remap)

## 完成 🎉

打开蓝牙并完成连接。

::: tip 省电
- 不用时可以关闭键盘电源开关
- 键盘会在闲置后自动进入低功耗模式
:::

::: tip 遇到问题？
查看 [常见问题](/faq) 或在 GitHub 提交 Issue。
:::
