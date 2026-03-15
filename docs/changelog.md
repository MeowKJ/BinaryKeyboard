---
outline: [2, 3]
---

# 📋 更新日志

::: tip 自动同步
本页内容由 GitHub Actions 自动从 [Releases](https://github.com/MeowKJ/BinaryKeyboard/releases) 同步生成。
:::

## ✨ binarykeyboard-20260314-2e0cfe0

📅 **发布时间**：2026 年 3 月 14 日

🔗 [在 GitHub 上查看](https://github.com/MeowKJ/BinaryKeyboard/releases/tag/binarykeyboard-20260314-2e0cfe0)

> **BinaryKeyboard 2026-03-14**

> 此 Release 由 GitHub Actions 自动构建并发布，基于 `main` 分支最新提交。

### 📋 版本信息

| 组件 | 版本 | 说明 |
|------|------|------|
| 🎹 Studio | v3.0.10 | 改键工具（Web） |
| 📡 CH592F | v3.0.16 | 无线固件（BLE） |
| 🔌 CH552G | v3.0.9 | 有线固件（USB） |

### 📦 固件文件

| 文件 | 大小 | 说明 |
|------|------|------|
| `CH552G-5KEY-3.0.9.bin` | 16K | 🔌 有线 · 五键 · BIN |
| `CH552G-BASIC-3.0.9.bin` | 16K | 🔌 有线 · 基础 · BIN |
| `CH552G-KNOB-3.0.9.bin` | 16K | 🔌 有线 · 旋钮 · BIN |
| `CH592F-5KEY-3.0.16.bin` | 180K | 📡 无线 · 五键 · BIN |
| `CH592F-KNOB-3.0.16.bin` | 180K | 📡 无线 · 旋钮 · BIN |
| `CH552G-5KEY-3.0.9.hex` | 40K | 🔌 有线 · 五键 · HEX |
| `CH552G-BASIC-3.0.9.hex` | 40K | 🔌 有线 · 基础 · HEX |
| `CH552G-KNOB-3.0.9.hex` | 40K | 🔌 有线 · 旋钮 · HEX |
| `CH592F-5KEY-3.0.16.hex` | 508K | 📡 无线 · 五键 · HEX |
| `CH592F-KNOB-3.0.16.hex` | 508K | 📡 无线 · 旋钮 · HEX |

### 🔧 构建环境

| 属性 | 值 |
|------|------|
| 构建日期 | 2026-03-14 |
| Commit | [`2e0cfe0`](https://github.com/MeowKJ/BinaryKeyboard/commit/2e0cfe0) |
| Runner | `ubuntu-latest` |
| CH592F 工具链 | `MRS_Toolchain_Linux_x64_V230`（RISC-V GCC 12） |
| SDCC | `4.2.0+dfsg-1` |
| CMake | `3.31.6` |
| Python | `3.12.3` |

<details><summary>🔒 SHA-256 校验值</summary>

```
fe8430f298c225c4284d938902a00bc3a49454ab6b070bd3e8ddb1e59eddcd4a  CH552G-5KEY-3.0.9.bin
8eadbe6191d7d657cb07e1613315146a6e91fb10732b1c1b4b94ba3cebb8637d  CH552G-BASIC-3.0.9.bin
75e729a425656b96ca9fe66ff068a5178d1b1c5cecd0a078447d13182d7565a4  CH552G-KNOB-3.0.9.bin
9785aa9c2084cbc3072b40a6840ea177059cae54675e8787d070d74146b216fa  CH592F-5KEY-3.0.16.bin
b8d8e0a09322faa669011f628745c85b24b25e96b3a23261acde3f26efc4057e  CH592F-KNOB-3.0.16.bin
e53a333c4c5f589af46b83347013bc1046f33bb52210c737752022698d16a087  CH552G-5KEY-3.0.9.hex
b9da47ef3ec8d96a9ca9034b8946d01bed6f442460d9e75612fc5c9a359512ea  CH552G-BASIC-3.0.9.hex
e6e91b8c157e04e9365f7fca8bc93346b4a825594f638fef12715163e75ae439  CH552G-KNOB-3.0.9.hex
7e9986bc500991f850088a7b6d51ca010f35a613704756b8d11f7fedd20c705d  CH592F-5KEY-3.0.16.hex
8e8047914c2276246503696cc69527a29f13538f520e458d827f1c78e74a397b  CH592F-KNOB-3.0.16.hex
```

</details>

<!-- Release notes generated using configuration in .github/release.yml at main -->

## What's Changed
### Other Changes
* feat: restructure firmware, enhance Studio & release pipeline by @MeowKJ in https://github.com/MeowKJ/BinaryKeyboard/pull/35
* fix: CI build failures - python3 in container, flashLayerColor params by @MeowKJ in https://github.com/MeowKJ/BinaryKeyboard/pull/36
* fix: CH592F version mismatch & release notes beautification by @MeowKJ in https://github.com/MeowKJ/BinaryKeyboard/pull/37
* style: simplify release title & add build tool versions by @MeowKJ in https://github.com/MeowKJ/BinaryKeyboard/pull/38
* style: code formatting cleanup by @MeowKJ in https://github.com/MeowKJ/BinaryKeyboard/pull/39

**Full Changelog**: https://github.com/MeowKJ/BinaryKeyboard/compare/readme-assets...binarykeyboard-20260314-2e0cfe0

---

## ✨ readme-assets

📅 **发布时间**：2026 年 3 月 13 日

🔗 [在 GitHub 上查看](https://github.com/MeowKJ/BinaryKeyboard/releases/tag/readme-assets)

> **README Assets**

Cloud-hosted assets used by README screenshots.

---

## 🧪 v20260225-841869d <Badge type="warning" text="Pre-release" />

📅 **发布时间**：2026 年 2 月 25 日

🔗 [在 GitHub 上查看](https://github.com/MeowKJ/BinaryKeyboard/releases/tag/v20260225-841869d)

> **Firmware 20260225 (841869d)**

**Toolchain:** MRS_Toolchain_Linux_x64_V230
**Compiler:** riscv-wch-elf-gcc (xPack GNU RISC-V Embedded GCC x86_64) 12.2.0


## What's Changed
* Dev by @MeowKJ in https://github.com/MeowKJ/BinaryKeyboard/pull/32
* ci: 引入固件编译检查与自动 Release 工作流 by @MeowKJ in https://github.com/MeowKJ/BinaryKeyboard/pull/33
* fix(ci): 修复容器内 git safe.directory 报错 by @MeowKJ in https://github.com/MeowKJ/BinaryKeyboard/pull/34


**Full Changelog**: https://github.com/MeowKJ/BinaryKeyboard/compare/toolchain-linux...v20260225-841869d

---

## ✨ toolchain-linux

📅 **发布时间**：2026 年 2 月 25 日

🔗 [在 GitHub 上查看](https://github.com/MeowKJ/BinaryKeyboard/releases/tag/toolchain-linux)

> **WCH RISC-V Toolchain — MRS Linux x64 V230**

MRS_Toolchain_Linux_x64_V230，供 CI Docker 镜像构建使用。请勿删除此 Release。

---

## 🧪 v3.0.0-alpha.1 <Badge type="warning" text="Pre-release" />

📅 **发布时间**：2026 年 2 月 16 日

🔗 [在 GitHub 上查看](https://github.com/MeowKJ/BinaryKeyboard/releases/tag/v3.0.0-alpha.1)

> ⚠️ **注意：这是一个 V3.0 的早期 Alpha 测试版本。**
> 核心功能仍在开发中，软硬件可能存在不稳定或未完善的情况。

---

### 🚧 1. 尚未完成的功能 (WIP)
* **低功耗模式：** 当前固件尚未加入深度休眠与功耗管理逻辑，测试时功耗可能偏高。
* **2.4G 无线通讯：** 2.4G 私有协议配对与通信模块仍在开发中，本次测试暂不可用。

### 📐 2. 硬件与 PCB 文件 (Hardware / Gerber)
* **打样文件：** 包含 **【无线-五键款】** 主板 V3.0 的完整 Gerber 生产文件，BOM与焊接辅助工具。

### 🛠️ 3. 3D 文件 (Mechanical / 3D)
* **目标外壳：** 包含 **【无线-五键款】** 的初步 3D 外壳工程文件。

### 🔌 4. 固件 (Firmware)
* **适用硬件：** 专用于上述 **【无线-五键款】** 的 V3.0 早期主板。
* **当前状态：** USB 有线通信和低功耗蓝牙（Bluetooth）广播/连接已成功调通，可正常交互。

### 💻 5. 软件/上位机 (Software)
* **当前状态：** 通信接口初步对接完成。
* **可用功能：** 目前仅包含基础终端（Terminal）面板，用于发送原始指令和接收设备日志。
暂时不支持经典有线款键盘！！！
---

### 🐛 6. 已知问题 (Known Bugs)
* **Windows 蓝牙兼容性：** 目前在 Windows 系统下进行蓝牙连接时，偶发连接不稳定或意外断开的现象（待后续协议栈优化）。
* **通信与日志冲突：** 当前版本中，蓝牙数据链路与 USB 日志输出（USB Log）功能无法同时工作。在进行调试时，请按需切换，避免共用冲突。

---

## ✨ v2.1.1

📅 **发布时间**：2025 年 11 月 15 日

🔗 [在 GitHub 上查看](https://github.com/MeowKJ/BinaryKeyboard/releases/tag/v2.1.1)

修复五键款RGB显示问题
其他两款可以下载上一个release中的固件(0x0A)
**Full Changelog**: https://github.com/MeowKJ/BinaryKeyboard/compare/v2.1...v2.1.1

---

## ✨ v2.1

📅 **发布时间**：2025 年 11 月 7 日

🔗 [在 GitHub 上查看](https://github.com/MeowKJ/BinaryKeyboard/releases/tag/v2.1)

> **带有RGB的可爱键盘固件0x0A**

* 更新6键无冲突，更新RGB6灯光，fun按键触发 by @Aaaou in https://github.com/MeowKJ/BinaryKeyboard/pull/2
* bug修复 by @Aaaou in https://github.com/MeowKJ/BinaryKeyboard/pull/4
* 合并无线款和有线款的工程 by @MeowKJ in https://github.com/MeowKJ/BinaryKeyboard/pull/7

## New Contributors
* @Aaaou made their first contribution in https://github.com/MeowKJ/BinaryKeyboard/pull/2
* @MeowKJ made their first contribution in https://github.com/MeowKJ/BinaryKeyboard/pull/7

**Full Changelog**: https://github.com/MeowKJ/BinaryKeyboard/compare/v2.0-alpha...v2.1

---

## 🧪 v2.0-alpha <Badge type="warning" text="Pre-release" />

📅 **发布时间**：2025 年 3 月 9 日

🔗 [在 GitHub 上查看](https://github.com/MeowKJ/BinaryKeyboard/releases/tag/v2.0-alpha)

> **# 🚀 CH552可爱键盘固件 v2.0 Alpha 发布！**

- `ch552_basic_0x08.hex` - 适用于 **基础款**  
- `ch552_fivekeys_0x08.hex` - 适用于 **五键款**  
- `ch552_knob_0x08.hex` - 适用于 **旋钮款**  

## 📌 3D打印文件

### **基础款**
- `Basic.stl` - **尾盖 + 1.5mm 定位板**  

### **五键款**
- `FiveKeysBox.stl` - **尾盖**  
- `FiveKeyPlateCube.stl` - **一体化上盖** (无需定位板)  
- `FiveKeysPlate.stl` - **1.5mm 定位板**  

### **旋钮款**
- `KnobBox.stl` - **尾盖**  
- `KnobPlateCube.stl` - **一体化上盖** (无需定位板)  
- `KnobPlate.stl` - **1.5mm 定位板**  

## 📌 软件工具
- `binary-keyboard-studio-web.zip` - **编译好的 Web 版改键工具**  

## 🔸 当前版本：`v2.0-alpha`
⚠️ **Alpha 测试版**，可能存在 bug，欢迎在 [Issue](https://github.com/MeowKJ/BinaryKeyboard/issues) 页面提交问题和建议！🚀

---

## 🎉 v1.1

📅 **发布时间**：2025 年 3 月 9 日

🔗 [在 GitHub 上查看](https://github.com/MeowKJ/BinaryKeyboard/releases/tag/v1.1)

- 修复Knob一个键卡口缺失的问题

---

## 🎉 v1

📅 **发布时间**：2025 年 3 月 6 日

🔗 [在 GitHub 上查看](https://github.com/MeowKJ/BinaryKeyboard/releases/tag/v1)

> **v1.0**

这个版本有着最基础的代码，可以作为基础增加自己的功能。

**Full Changelog**: https://github.com/MeowKJ/BinaryKeyboard/commits/v1

3DBasic.stl -> 基础款的3D打印文件

3DKnob.zip -> 旋钮款的3D打印文件

main.hex -> 基础的固件，按键映射为 0 1 回车 空格

