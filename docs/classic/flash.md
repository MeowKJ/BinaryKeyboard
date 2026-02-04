# 经典版固件刷写

## 准备工作

- **WCHISPStudio** - [下载地址](https://www.wch.cn/downloads/WCHISPTool_Setup_exe.html)
- 对应版本的 `.hex` 固件文件 - [GitHub Releases](https://github.com/MeowKJ/BinaryKeyboard/releases)

## 固件文件说明

| 固件文件                 | 适用外形 |
| :----------------------- | :------- |
| `ch552_basic_xxx.hex`    | 基础款   |
| `ch552_fivekeys_xxx.hex` | 五键款   |
| `ch552_knob_xxx.hex`     | 旋钮款   |

> 以 GitHub Releases 的发布文件为准。注意前缀为`ch552_`，不要下错成无线版的`ch592_`。

## 刷写步骤

### 软件配置

1. 打开 **WCHISPStudio**
2. 顶部工具栏选择 **MCU系列视图** → **E8051USB系列**
3. 芯片选择：芯片系列 **CH55x**，芯片型号 **CH552**，下载接口 **USB**
4. 下载文件：目标程序文件1 选择对应的 `.hex` 固件文件

### 硬件操作

5. **按住** PCB 上的 **BOOT** 按钮不松开
6. 保持按住的同时，将 USB-C 插入电脑
7. 松开 BOOT 按钮（此时软件应识别到设备）

::: tip
如果没有识别到设备，请检BOOT按钮是否按下，或者检查USB-C接口是否焊接良好。
:::

### 开始烧录

8. 点击"下载"按钮
9. 等待进度条完成，提示成功
10. 重新拔插 USB，测试键盘
