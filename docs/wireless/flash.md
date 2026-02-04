# 无线版固件刷写

## 准备工作

- **WCHISPStudio** - [下载地址](https://www.wch.cn/downloads/WCHISPTool_Setup_exe.html)
- 对应版本的 `.hex` 固件文件

## 固件文件说明

| 固件文件                 | 适用外形 |
| :----------------------- | :------- |
| `ch592_basic_xxx.hex`    | 基础款   |
| `ch592_fivekeys_xxx.hex` | 五键款   |
| `ch592_knob_xxx.hex`     | 旋钮款   |
| `ch592_magnetic_xxx.hex` | 磁轴款   |

> 以 GitHub Releases 的发布文件为准。

## 进入 Bootloader 模式

1. 拔掉 USB-C
2. **长按 BOOT 按钮**
3. 按住不放的同时插入 USB-C
4. 松开按钮

::: tip
CH592F 的 Bootloader 与 CH552G 类似，但芯片类型选择不同。
:::

## 烧录步骤

1. 打开 **WCHISPStudio**
2. 选择芯片类型为 **CH592**
3. 点击"打开文件"，选择 `.hex` 固件
4. 点击"下载/烧录"
5. 等待进度条完成
6. 拔插 USB，测试蓝牙配对

## 蓝牙配对测试

TODO