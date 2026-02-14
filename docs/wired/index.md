# ⚡ 经典版快速开始

基于 **CH552G** 芯片的 USB 直连版本，即插即用。

## 准备工作

### 你需要

| 工具/材料       | 说明                                 |
| :-------------- | :----------------------------------- |
| 🔧 电烙铁 + 焊锡 | 焊接 PCB                             |
| 🖨️ 3D 打印机     | 或者使用 3D 打印服务                 |
| 💻 电脑          | 推荐 Windows，没有测试过 macOS/Linux |
| 🔌 USB-C 数据线  | 连接键盘                             |

### 下载文件

1. **固件** - 从 [GitHub Releases](https://github.com/MeowKJ/BinaryKeyboard/releases) 下载
   - `ch552_basic_xxx.hex` - 基础款
   - `ch552_fivekeys_xxx.hex` - 五键款
   - `ch552_knob_xxx.hex` - 旋钮款

2. **外壳 STL** - 从 Releases 或 [OSHWHub](https://oshwhub.com/kjpig/Binary-Keyboard) 下载。

3. **烧录工具** - [WCHISPStudio](https://www.wch.cn/downloads/WCHISPTool_Setup_exe.html)

## 复刻流程

### Step 1：3D 打印外壳

- 普通 FDM 打印即可，没有特殊要求。
- 层高 0.2mm

### Step 2：焊接 PCB

1. 焊接 USB-C 接口
2. 按照原理图焊接元器件
3. 焊接按键

::: warning 注意
USB-C接口的焊接难度略大，需要小心。可以寻找相关教程视频学习。我个人推荐使用一些助焊剂帮助焊接。一般复刻失败的原因都是USB-C接口焊接失败。
:::

### Step 3：刷写固件

#### 软件配置

1. 打开 **WCHISPStudio**
2. 顶部工具栏选择 **MCU系列视图** → **E8051USB系列**
3. 芯片选择：芯片系列 **CH55x**，芯片型号 **CH552**，下载接口 **USB**
4. 下载文件：目标程序文件1 选择对应的 `.hex` 固件文件

#### 硬件操作

5. **按住** PCB 上的 **BOOT** 按钮不松开
6. 保持按住的同时，将 USB-C 插入电脑
7. 松开 BOOT 按钮（此时软件应识别到设备）

#### 开始烧录

8. 点击"下载"按钮
9. 等待进度条完成，提示成功

详细步骤见 [刷写固件](./flash)

### Step 4：组装

1. 将 PCB 装入外壳
2. 盖上盖板，拧紧螺丝
3. 安装轴体

### Step 5：配置键位

1. 用 Chrome / Edge 打开改键工具
2. 点击"连接设备"
3. 选择你的键盘
4. 配置想要的键位映射
5. 点击"写入"

详细步骤见 [改键软件使用](./remap)

## 完成 🎉

插上 USB，享受你的可爱二进制键盘吧！

::: tip 遇到问题？
查看 [常见问题](/faq) 或在 GitHub 提交 Issue。
:::
