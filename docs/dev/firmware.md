# 固件开发

本页给出“从源码改功能/改键位”的最小路线。

## 固件目录

仓库内固件通常位于：

- `Firmware/`（或其子目录 `Firmware/CH552G`）

## 选择硬件版本

根据硬件版本，通常需要在 `config.h` 里启用对应宏：

- `USE_BASIC` / `USE_5KEYS` / `USE_KNOB`

## 常见改动点

- 默认键位（按键扫描 -> HID 输出）
- Fn / 多层（如果固件支持）
- RGB 灯效（如果硬件版本带 WS2812 等）

## 编译与下载

不同作者可能使用不同工具链（Keil / SDCC / Arduino / PlatformIO 等）。  
请以仓库中固件子目录的 README 为准。

> 如果你只想快速用，不需要编译：直接用 Releases 的 `.hex` 最省事。
