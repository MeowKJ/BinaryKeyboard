---
outline: [2, 3]
---

# 📋 更新日志

::: tip 自动同步
本页内容由 GitHub Actions 自动从 [Releases](https://github.com/MeowKJ/BinaryKeyboard/releases) 同步生成。
:::

## ✨ binarykeyboard-20260319

📅 **发布时间**：2026 年 3 月 19 日

🔗 [在 GitHub 上查看](https://github.com/MeowKJ/BinaryKeyboard/releases/tag/binarykeyboard-20260319)

> **Release 2026-03-19**

---

### 版本信息

| 组件 | 版本 | 说明 |
|:-----|:-----|:-----|
| 🎹 Studio | `v3.0.31` | 改键工具（Web） |
| 📡 CH592F | `v3.0.29` | 无线固件（BLE） |
| 🔌 CH552G | `v3.0.20` | 有线固件（USB） |

---

### 🖥️ Studio 桌面版

| 平台 | 架构 | 格式 |
|:-----|:-----|:-----|
| macOS | Apple Silicon (arm64) | DMG |
| macOS | Intel (x64) | DMG |
| Windows | x64 | 便携式 EXE |
| Windows | x86 / 32位 (ia32) | 便携式 EXE |
| Windows | ARM64 | 便携式 EXE |

---

### 📦 固件文件

| 文件 | Flash 占用 | RAM 占用 | 说明 |
|:-----|:----------|:---------|:-----|
| `CH552G-5KEY-3.0.20.hex` | ███████████████████░ 96% (12.8/13 KB) | 0.3 / 0.9 KB (31%) | 🔌 有线 · 五键 |
| `CH552G-BASIC-3.0.20.hex` | ███████████████████░ 96% (12.8/13 KB) | 0.2 / 0.9 KB (26%) | 🔌 有线 · 基础 |
| `CH552G-KNOB-3.0.20.hex` | ███████████████████░ 98% (13.0/13 KB) | 0.3 / 0.9 KB (31%) | 🔌 有线 · 旋钮 |
| `CH592F-5KEY-3.0.29.hex` | ███████░░░░░░░░░░░░░ 37% (169.8/448 KB) | 22.9 / 26.0 KB (88%) | 📡 无线 · 五键 |
| `CH592F-KNOB-3.0.29.hex` | ███████░░░░░░░░░░░░░ 37% (169.8/448 KB) | 22.8 / 26.0 KB (88%) | 📡 无线 · 旋钮 |

---

### 变更范围

- 自 [`binarykeyboard-20260318`](https://github.com/MeowKJ/BinaryKeyboard/releases/tag/binarykeyboard-20260318) 以来共 **11** 个提交
- 完整对比: https://github.com/MeowKJ/BinaryKeyboard/compare/binarykeyboard-20260318...binarykeyboard-20260319

<!-- Release notes generated using configuration in .github/release.yml at 11147d0619113e83bcadc13f311887143f31e989 -->

## What's Changed
### Pull Requests
* 修复宏写入 + Studio UI 优化 + CI 修复 by @MeowKJ in https://github.com/MeowKJ/BinaryKeyboard/pull/61

### 提交记录

- feat(release): 更新发布说明格式，添加图标以增强可读性 by @MeowKJ ([`2b4ea61`](https://github.com/MeowKJ/BinaryKeyboard/commit/2b4ea610bbfeed890cf088c144af5313c20b3d4a))
- docs: sync changelog from releases by @github-actions[bot] ([`d24fe92`](https://github.com/MeowKJ/BinaryKeyboard/commit/d24fe92d3643da67f23d3659e03c0a14269912b1))
- fix(docs): relax themeConfig typing in vitepress config by @MeowKJ ([`7764316`](https://github.com/MeowKJ/BinaryKeyboard/commit/77643168d86c266eadc2f112e7bb5753257cd067))
- chore: save workspace state — commit all current changes by @MeowKJ ([`7d42543`](https://github.com/MeowKJ/BinaryKeyboard/commit/7d4254372deb6060337fca8f0be3b450223c1d88))
- fix(firmware): CH592F 宏写入数据丢失 by @MeowKJ ([`cc3176b`](https://github.com/MeowKJ/BinaryKeyboard/commit/cc3176b1e6bcb6971721c2f620c034adfaebf61e))
- 修复(studio): 宏保存流程优化 by @MeowKJ ([`95cc1aa`](https://github.com/MeowKJ/BinaryKeyboard/commit/95cc1aaafe254f49e0c1d6eed63ac50626a830b9))
- 功能(studio): MeowMacro 重复折叠优化 by @MeowKJ ([`c4c167e`](https://github.com/MeowKJ/BinaryKeyboard/commit/c4c167e75be90872aef45d0ccd9ad03905043292))
- 优化(studio): UI 刷新 by @MeowKJ ([`871f584`](https://github.com/MeowKJ/BinaryKeyboard/commit/871f584feb8856f142f827a647539d0b60c6eb95))
- 功能(studio): 猫咪助手交互菜单 + 新表情资源 by @MeowKJ ([`8b33ea0`](https://github.com/MeowKJ/BinaryKeyboard/commit/8b33ea0803987c7469a29778ba054852cc2cfaa2))
- 修复(ci): release changelog 范围修正 by @MeowKJ ([`cb0051b`](https://github.com/MeowKJ/BinaryKeyboard/commit/cb0051b946e449737db1c6330b0a304da91c4c22))

**Full Changelog**: https://github.com/MeowKJ/BinaryKeyboard/compare/binarykeyboard-20260318...binarykeyboard-20260319

---

<details><summary>构建环境</summary>

| 属性 | 值 |
|:-----|:---|
| 构建日期 | 2026-03-19 |
| Commit | [`11147d0`](https://github.com/MeowKJ/BinaryKeyboard/commit/11147d0) |
| CH592F 工具链 | `MRS_Toolchain_Linux_x64_V230` (RISC-V GCC 12) |
| SDCC | `4.2.0+dfsg-1` |
| CMake | `3.31.6` |
| Python | `3.12.3` |

</details>

<details><summary>SHA-256 校验值</summary>

```
a63aeeae100029f2a841ef3461aba40cc815943cdcdd0c895c93a04bfdbf2dec  CH552G-5KEY-3.0.20.hex
aeb32b2a60e700a648b15a3f6099ed0d2fe5f7a017890f2552a17fee1c946e0d  CH552G-BASIC-3.0.20.hex
8ecfc558f6a7af23bbdd6a4b5a3d711212d27529161218c5baa3356daed2597e  CH552G-KNOB-3.0.20.hex
4efb765a7416989d374bee6944bd230bfcf842491954bfc2e7c79f4ceb155f13  CH592F-5KEY-3.0.29.hex
76720a5740748797e1d197406858270c4ddf8ff8aa7c281e609bb66e502b31ef  CH592F-KNOB-3.0.29.hex
```

</details>

---

## ✨ binarykeyboard-20260316-f661ba3

📅 **发布时间**：2026 年 3 月 16 日

🔗 [在 GitHub 上查看](https://github.com/MeowKJ/BinaryKeyboard/releases/tag/binarykeyboard-20260316-f661ba3)

> **Release 2026-03-16**

> 此 Release 由 GitHub Actions 自动构建并发布，基于 `main` 分支最新提交。

### 📋 版本信息

| 组件 | 版本 | 说明 |
|------|------|------|
| 🎹 Studio | v3.0.18 | 改键工具（Web） |
| 📡 CH592F | v3.0.24 | 无线固件（BLE） |
| 🔌 CH552G | v3.0.12 | 有线固件（USB） |

### 📦 固件文件

| 文件 | 大小 | Flash 占用 | 说明 |
|------|------|-----------|------|
| `CH552G-5KEY-3.0.12.bin` | 14KiB | ███████████████████░ 97% (14KiB) | 🔌 有线 · 五键 |
| `CH552G-BASIC-3.0.12.bin` | 14KiB | ███████████████████░ 97% (14KiB) | 🔌 有线 · 基础 |
| `CH552G-KNOB-3.0.12.bin` | 14KiB | ███████████████████░ 99% (14KiB) | 🔌 有线 · 旋钮 |
| `CH592F-5KEY-3.0.24.bin` | 170KiB | ███████░░░░░░░░░░░░░ 37% (448KiB) | 📡 无线 · 五键 |
| `CH592F-KNOB-3.0.24.bin` | 170KiB | ███████░░░░░░░░░░░░░ 37% (448KiB) | 📡 无线 · 旋钮 |

### 📊 Flash / RAM 占用

| 固件 | Flash | RAM |
|------|-------|-----|
| `CH552G-5KEY` | 13.6 / 14.0 KB (97.4%) | 0.2 / 0.9 KB (28.5%) |
| `CH552G-BASIC` | 13.6 / 14.0 KB (97.3%) | 0.2 / 0.9 KB (23.7%) |
| `CH552G-KNOB` | 13.9 / 14.0 KB (99.2%) | 0.2 / 0.9 KB (28.5%) |
| `CH592F-5KEY` | 169.2 / 448.0 KB (37.8%) | 22.8 / 26.0 KB (87.8%) |
| `CH592F-KNOB` | 169.1 / 448.0 KB (37.7%) | 22.7 / 26.0 KB (87.3%) |

### 🔧 构建环境

| 属性 | 值 |
|------|------|
| 构建日期 | 2026-03-16 |
| Commit | [`f661ba3`](https://github.com/MeowKJ/BinaryKeyboard/commit/f661ba3) |
| Runner | `ubuntu-latest` |
| CH592F 工具链 | `MRS_Toolchain_Linux_x64_V230`（RISC-V GCC 12） |
| SDCC | `4.2.0+dfsg-1` |
| CMake | `3.31.6` |
| Python | `3.12.3` |

<details><summary>🔒 SHA-256 校验值</summary>

```
7b90b29de9ce0e7cac396a7a22d51e538f30f7fe1a67dac6bde601ab4838fc55  CH552G-5KEY-3.0.12.bin
c837155538555eaeeaac17fd4f104811cafcf04e04c2749c79d4f1e584e4976a  CH552G-BASIC-3.0.12.bin
d8d750802cb822c50693044feb12be489d881747319a2b96bd889458be3bff6a  CH552G-KNOB-3.0.12.bin
ce2c11497b22ce2a4bdc1addc56ab66a97b0d7d2b77c5f13b71325777bebde13  CH592F-5KEY-3.0.24.bin
93056086729cb6b0c0b944ed925ede7138f3d953976a93cc29811e2d9da8e2ca  CH592F-KNOB-3.0.24.bin
```

</details>

<!-- Release notes generated using configuration in .github/release.yml at main -->

## What's Changed
### Other Changes
* 修复前端依赖安全告警 by @MeowKJ in https://github.com/MeowKJ/BinaryKeyboard/pull/50

**Full Changelog**: https://github.com/MeowKJ/BinaryKeyboard/compare/binarykeyboard-20260316-f3dfc03...binarykeyboard-20260316-f661ba3

---

## ✨ toolchain-linux

📅 **发布时间**：2026 年 2 月 25 日

🔗 [在 GitHub 上查看](https://github.com/MeowKJ/BinaryKeyboard/releases/tag/toolchain-linux)

> **WCH RISC-V Toolchain — MRS Linux x64 V230**

MRS_Toolchain_Linux_x64_V230，供 CI Docker 镜像构建使用。请勿删除此 Release。

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

