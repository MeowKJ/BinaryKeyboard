# 项目概览

BinaryKeyboard（可爱二进制键盘）是一个基于 **CH552G** 的迷你键盘/宏键盘项目，包含：

- **键盘固件源码**
- **Web 改键工具（Vue3）**
- **3D 外壳工程文件（Shapr3D / STL）**

> 设计参考与更多信息：  
> - OSHWHub：https://oshwhub.com/kjpig/Binary-Keyboard  
> - GitHub：https://github.com/MeowKJ/BinaryKeyboard

## 特性一览

- 设备通信基于 **HID 协议**，改键工具可自动识别多款硬件并修改键位映射
- 改键支持：**键盘键位 / 多媒体 / 鼠标** 三类映射
- 外壳支持：基础款 / 五键款 / 旋钮款（详见 3D 打印文件）

## 版本说明（常见三款）

| 版本 | 特征 | 固件文件名（示例） |
|---|---|---|
| 基础款 | 少键位（示意为 0/1/Enter/Space 等） | `CH552G_BASIC_xxxx.hex` |
| 五键款 | 5 个按键 | `CH552G_5KEYS_xxxx.hex` |
| 旋钮款 | 旋钮 + 按键 | `CH552G_KNOB_xxxx.hex` |

> 固件以 GitHub Releases 为准。

## 目录导航

- [快速开始](#/guide/quickstart)
- [硬件复刻指南](#/guide/make)
- [刷写固件](#/guide/flash)
- [改键软件使用](#/guide/remap)
- [FAQ](#/faq)
