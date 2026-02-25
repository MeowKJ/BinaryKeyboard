---
outline: [2, 3]
---

# 📋 更新日志

::: tip 自动同步
本页内容由 GitHub Actions 自动从 [Releases](https://github.com/MeowKJ/BinaryKeyboard/releases) 同步生成。
:::

## 🧪 toolchain-linux <Badge type="warning" text="Pre-release" />

📅 **发布时间**：2026 年 2 月 25 日

🔗 [在 GitHub 上查看](https://github.com/MeowKJ/BinaryKeyboard/releases/tag/toolchain-linux)

> **WCH RISC-V Toolchain — MRS Linux x64 V230**

MRS_Toolchain_Linux_x64_V230，供 CI Docker 镜像构建使用。请勿删除此 Release。

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

