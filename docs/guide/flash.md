# 刷写固件（WCHISPStudio）

## 你需要准备

- WCHISPStudio（沁恒的烧录工具）
- 对应版本固件 `.hex`

## 固件文件如何选

不同硬件版本固件不通用，请按版本选择：

- `CH552G_BASIC_xxxx.hex`：基础款
- `CH552G_5KEYS_xxxx.hex`：五键款
- `CH552G_KNOB_xxxx.hex`：旋钮款

> 以 GitHub Releases 的发布文件为准。

## 进入 Bootloader 模式

如果你插入 USB-C 后电脑没有识别到设备：

1. 拔掉 USB-C
2. **长按 BOOT 按钮**（通常是距离板边更近的那个小按钮）
3. 按住不放的同时插入 USB-C
4. 进入 Bootloader 模式后再用 WCHISPStudio 烧录

## 烧录流程（通用）

1. 打开 WCHISPStudio
2. 选择芯片/模式（按你的工具提示，常见为 CH55x 系列）
3. 选择固件 `.hex`
4. 点击下载/烧录
5. 完成后拔插 USB，测试键盘是否能正常输出

## 从源码编译（可选）

固件源码中通常会有 `config.h` 之类的版本开关。  
例如（示例）：

- 基础款：启用 `USE_BASIC`
- 五键款：启用 `USE_5KEYS`
- 旋钮款：启用 `USE_KNOB`

编译方式请以仓库 `Firmware/CH552G` 下说明为准。
