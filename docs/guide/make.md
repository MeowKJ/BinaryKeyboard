# 硬件复刻指南

> 目标：把 PCB + 元件 + 外壳文件组合成可用设备。

## 核心器件（简要）

BOM 在 OSHWHub 页面提供；这里仅列出复刻最关键的部分：

- MCU：CH552G（SOP-16）
- 去耦电容：100nF（常见为 0603）
- 若干电阻、按键（含 BOOT 按钮等）

> 请以 OSHWHub 工程的 BOM 为准。

## 3D 打印文件

常见文件如下（不同版本不同）：

- 基础款：`Basic.stl`（尾盖 + 1.5mm 定位板）
- 五键款：`FiveKeysBox.stl`、`FiveKeyPlateCube.stl`、`FiveKeysPlate.stl`
- 旋钮款：`KnobBox.stl`、`KnobPlateCube.stl`、`KnobPlate.stl`

## 组装建议

- 先焊 **贴片元件与接口**，再处理轴体/热插拔等
- 刷写固件前建议检查：
  - USB 供电是否正常
  - BOOT 按键是否可用（进 Bootloader 需要用到）
  - 焊点是否短路/虚焊

> 如果你复刻的是“旋钮款”，旋钮建议选常见 EC11 系列编码器，注意焊接与消抖电容。
