---
outline: [2, 3]
---

# 📋 更新日志

::: tip 自动同步
本页内容由 GitHub Actions 自动从 [Releases](https://github.com/MeowKJ/BinaryKeyboard/releases) 同步生成。
:::

## ✨ binarykeyboard-20260318

📅 **发布时间**：2026 年 3 月 18 日

🔗 [在 GitHub 上查看](https://github.com/MeowKJ/BinaryKeyboard/releases/tag/binarykeyboard-20260318)

> **Release 2026-03-18**

---

### 版本信息

| 组件 | 版本 | 说明 |
|:-----|:-----|:-----|
| 🎹 Studio | `v` | 改键工具（Web） |
| 📡 CH592F | `v` | 无线固件（BLE） |
| 🔌 CH552G | `v` | 有线固件（USB） |

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

### 固件文件

| 文件 | Flash 占用 | RAM 占用 | 说明 |
|:-----|:----------|:---------|:-----|
| `CH552G-5KEY-3.0.20.hex` | ███████████████████░ 96% (12.8/13 KB) | 0.3 / 0.9 KB (31%) | 🔌 有线 · 五键 |
| `CH552G-BASIC-3.0.20.hex` | ███████████████████░ 96% (12.8/13 KB) | 0.2 / 0.9 KB (26%) | 🔌 有线 · 基础 |
| `CH552G-KNOB-3.0.20.hex` | ███████████████████░ 98% (13.0/13 KB) | 0.3 / 0.9 KB (31%) | 🔌 有线 · 旋钮 |
| `CH592F-5KEY-3.0.27.hex` | ███████░░░░░░░░░░░░░ 37% (169.4/448 KB) | 22.8 / 26.0 KB (88%) | 📡 无线 · 五键 |
| `CH592F-KNOB-3.0.27.hex` | ███████░░░░░░░░░░░░░ 37% (169.4/448 KB) | 22.7 / 26.0 KB (87%) | 📡 无线 · 旋钮 |

---

### 变更范围

- 首次自动发布，当前共 **166** 个提交

<!-- Release notes generated using configuration in .github/release.yml at 56ce97193508aba6b28b53e0899c5f49344e874f -->

## What's Changed
### Pull Requests
* feat: MeowMacro 宏语言 + CH552G 宏支持 by @MeowKJ in https://github.com/MeowKJ/BinaryKeyboard/pull/54
* feat: MeowFS 宏存储系统与出厂重置 by @MeowKJ in https://github.com/MeowKJ/BinaryKeyboard/pull/55
* fix: CH552G 构建修复 (OSEG/Flash/SDCC) by @MeowKJ in https://github.com/MeowKJ/BinaryKeyboard/pull/57
* 优化 Release Notes 与作者识别 by @MeowKJ in https://github.com/MeowKJ/BinaryKeyboard/pull/58
* 优化 Release Notes 结构 by @MeowKJ in https://github.com/MeowKJ/BinaryKeyboard/pull/59
* feat: Electron 桌面版 + CH592F RGB DataFlash 修复 by @MeowKJ in https://github.com/MeowKJ/BinaryKeyboard/pull/60

### 提交记录

- Initial commit by @MeowKJ ([`b9311ba`](https://github.com/MeowKJ/BinaryKeyboard/commit/b9311baba645a50d509f6b5a2e690a85384b377e))
- First Commit by @MeowKJ ([`65291a1`](https://github.com/MeowKJ/BinaryKeyboard/commit/65291a1cccf12cd85bcdad012e7545dd0e1cc635))
- add README.md by @MeowKJ ([`d9514c5`](https://github.com/MeowKJ/BinaryKeyboard/commit/d9514c548e15dc573e58c8756732909b8613db57))
- Update README.md by @MeowKJ ([`7a2350f`](https://github.com/MeowKJ/BinaryKeyboard/commit/7a2350fc5b15553f1bcf4288c4b15d759135a7d0))
- Update README.md by @MeowKJ ([`1685040`](https://github.com/MeowKJ/BinaryKeyboard/commit/16850406c87fc7297ad1404c94b787d2ff850427))
- feat✨: 复合HID与EEPROM配置管理  • ✨ 新增复合HID设备支持   - 添加键盘(0x01)/鼠标(0x02)/媒体控制(0x03)三路Report ID   - 符合USB HID 1.11规范   - 支持同时发送多设备事件  • 🗃️ 实现EEPROM配置管理   | 地址 | 字段         | 说明                 |   |------|--------------|----------------------|   | 0x00 | 固件版本      | 当前版本 v1.0 (0x01) |   | 0x01 | 设备类型      | 旋钮款标识 (0x02)    |   | 0x02 | 键位配置数据  | 8键×3字节连续存储     | by @MeowKJ ([`870c0a9`](https://github.com/MeowKJ/BinaryKeyboard/commit/870c0a991fedcc7390fc96cb49946067e93c9792))
- feat✨: 添加Vue前端改键项目框架 by @MeowKJ ([`523d725`](https://github.com/MeowKJ/BinaryKeyboard/commit/523d725c6f6f536b16953b14f05a431e8b4d3e65))
- feat✨: 添加键盘、媒体和鼠标类型常量，重构按键映射逻辑，优化组件交互（未完成的） by @MeowKJ ([`37693de`](https://github.com/MeowKJ/BinaryKeyboard/commit/37693deb57f34a68f718ee69de107b4c07d10eef))
- Merge branch 'main' of https://github.com/MeowKJ/BinaryKeyboard by @MeowKJ ([`006d58e`](https://github.com/MeowKJ/BinaryKeyboard/commit/006d58efc0bd8b6f8c460741db6f4f737a31b723))
- feat✨: 重构NormalButton和KnobKeyboard组件，优化按键映射逻辑，删除无用的验证工具函数 by @MeowKJ ([`5a229d0`](https://github.com/MeowKJ/BinaryKeyboard/commit/5a229d0b0c8f6cba2d831b69b2a04527e0f497d7))
- feat✨: 添加MouseSelector组件，更新NormalButton组件以支持当前索引，优化样式和类型定义 by @MeowKJ ([`00927d6`](https://github.com/MeowKJ/BinaryKeyboard/commit/00927d64e9e095d963ae4230101d5855c86d87ee))
- feat✨: 添加MouseSelector组件，更新鼠标配置逻辑，优化样式和类型定义，增加primeicons依赖 by @MeowKJ ([`4b9afb6`](https://github.com/MeowKJ/BinaryKeyboard/commit/4b9afb6fd704462699e5b02c7b8951e7366c32bb))
- feat✨: (软硬联调)更新键盘处理逻辑，添加原始HID数据下按键按下和释放功能，实现键盘类型的按键修改 by @MeowKJ ([`098f6dc`](https://github.com/MeowKJ/BinaryKeyboard/commit/098f6dc2ce172fd9a66459fe4d8c50b2cd03bee9))
- feat✨: 添加KnobButton组件，重构KnobKeyboard，优化按钮选择逻辑，更新媒体和鼠标配置 by @MeowKJ ([`d3c879f`](https://github.com/MeowKJ/BinaryKeyboard/commit/d3c879f77409e359c845b5f69ad96d7331c372db))
- feat✨:完善3种类型映射录入， 添加DataInfoCard组件，更新设备信息展示逻辑，优化数据发送和接收处理。基本功能完成。 by @MeowKJ ([`626bfdb`](https://github.com/MeowKJ/BinaryKeyboard/commit/626bfdbf15e5f32034e25a5bd2bbca2d95e0f878))
- feat✨: 添加primeflex依赖，优化DeviceInfoCard样式，重构媒体选择器和键盘选择器逻辑，移除冗余样式，增加HID转换器功能 by @MeowKJ ([`5bc9ec0`](https://github.com/MeowKJ/BinaryKeyboard/commit/5bc9ec0b709f27897b93a6e09d317b2781bf10e2))
- fix🐛: 更新AppFooter组件中的社交链接URL by @MeowKJ ([`de2f29b`](https://github.com/MeowKJ/BinaryKeyboard/commit/de2f29b5d941d10581c6ca5c76a75a7460aab12f))
- feat✨: 优化DataInfoCard组件，高亮修改后的数据 by @MeowKJ ([`9bf99f4`](https://github.com/MeowKJ/BinaryKeyboard/commit/9bf99f41c7b5636b9bc8be372dd2009a7fcc6127))
- feat✨: 更新按键处理逻辑，优化编码器事件处理，添加配置文件支持 by @MeowKJ ([`4abc208`](https://github.com/MeowKJ/BinaryKeyboard/commit/4abc20864b9bf4cab566d006cc7731458839aae4))
- feat✨: by @MeowKJ ([`c45026b`](https://github.com/MeowKJ/BinaryKeyboard/commit/c45026b609944b471595ab6ac8ba0603682e8ff4))
- feat✨: 更新USB报告描述符，增加侧键支持并调整按钮数量固件更新到(0x09) by @MeowKJ ([`d928e60`](https://github.com/MeowKJ/BinaryKeyboard/commit/d928e60595cda63b20aea4a2ae4175ee1c21edcc))
- Merge branch 'main' of https://github.com/MeowKJ/BinaryKeyboard by @MeowKJ ([`eb76bda`](https://github.com/MeowKJ/BinaryKeyboard/commit/eb76bda53cf7c21000e30931bcffc6e22b9e2de1))
- feat✨: 优化旋钮按钮组件，增加表示方向的图标 by @MeowKJ ([`ee8cd9d`](https://github.com/MeowKJ/BinaryKeyboard/commit/ee8cd9d9e7e97cea076e4b3776d00ea7b6b3c19e))
- feat✨: 更新首页标题为“BinaryKeyboard按键映射修改”，增加favicon.ico by @MeowKJ ([`7c23665`](https://github.com/MeowKJ/BinaryKeyboard/commit/7c23665a1e6469dd55c989f7fc2315a82607873e))
- Merge pull request #2 from Aaaou/main  更新6键无冲突，更新RGB6灯光，fun按键触发 by @MeowKJ ([`2ec7b0b`](https://github.com/MeowKJ/BinaryKeyboard/commit/2ec7b0bb99ec0b2138ef5536cb7b41d560be0eb6))
- feat(未测试)✨: 初步整合了rgb代码，编译无错误，未测试。 by @MeowKJ ([`a80b4e2`](https://github.com/MeowKJ/BinaryKeyboard/commit/a80b4e2beeaefd61cf69e86e744a86bd9830056b))
- Merge pull request #4 from Aaaou/main  bug修复 by @MeowKJ ([`fa21ae1`](https://github.com/MeowKJ/BinaryKeyboard/commit/fa21ae1e0e48782cb344ddd9d2eac4f739bb13d9))
- Merge pull request #7 from MeowKJ/wireless-dev  合并无线款和有线款的工程 by @MeowKJ ([`0c6f80b`](https://github.com/MeowKJ/BinaryKeyboard/commit/0c6f80bc1788f73a562bfbeb411d516315e04402))
- docs📝: 更新 README 文件，添加无线款的开发环境配置说明 by @MeowKJ ([`45bfd6b`](https://github.com/MeowKJ/BinaryKeyboard/commit/45bfd6ba38791acbbdecb2bb77d342b079ccdad6))
- refactor🎨: CH552.ino -> CH552G.ino by @MeowKJ ([`ba7c8b7`](https://github.com/MeowKJ/BinaryKeyboard/commit/ba7c8b7f40d1d0b88b84cca677ad9e5da428ad77))
- chore: 添加 build/ 目录到 .gitignore by @MeowKJ ([`838437c`](https://github.com/MeowKJ/BinaryKeyboard/commit/838437c4b2e1950d6f4eee5b37177ad9acbe5307))
- chore: 删除 CH592F.launch 配置文件 by @MeowKJ ([`7d257d8`](https://github.com/MeowKJ/BinaryKeyboard/commit/7d257d8634e03c73d558cc10304576ee9189e38b))
- fix🐛: 五键款RGB显示问题 by @MeowKJ ([`4db0564`](https://github.com/MeowKJ/BinaryKeyboard/commit/4db0564e1dca1fb465b83999c3443d308433f78b))
- fix🐛: cannot find -lISP592. Fixes #14 by @MeowKJ ([`68f63b4`](https://github.com/MeowKJ/BinaryKeyboard/commit/68f63b4d428dfc3f66457ccf9210884917010618))
- fix: 修复多媒体按键配置时发生连锁修改的 Bug  解决了在配置多媒体按键时，修改一个按键的键值会导致其他所有按键的配置同步被修改的问题。  主要修复措施： 1.  **设备 Store 初始化**: 修正 `deviceStore.ts` 中 `keyMappingsList` 的初始化逻辑，由错误的 `Array(8).fill(obj)` 改为使用 `Array.from`，确保数组中的每个配置对象都是独立的内存引用，避免默认状态下所有按键共享同一个配置对象。 2.  **配置加载隔离**: 在 `Selector.vue` 的 `onDialogOpen` 中，对 Store 中的配置对象（Keyboard, Media, Mouse）进行浅复制（Spread `...`），确保组件内部修改的是本地副本，只有在点击保存后才更新 Store，从而防止实时修改导致的状态污染。  fixes #15 by @MeowKJ ([`25ad1e9`](https://github.com/MeowKJ/BinaryKeyboard/commit/25ad1e9569320efbff0c1df1399343409c1d4935))
- Refactor keyboard driver and remove unused timer header  - Updated key.h to enhance keyboard event handling with new configuration parameters and event structures. - Removed tim.h as it was no longer needed. - Refactored key.c to implement a more efficient event queue for key presses and releases, including debounce handling. - Added support for function keys with dedicated event handling. - Improved GPIO interrupt handling for key events. - Updated project configuration to use a newer toolchain version. - Cleaned up USB device handling code for better readability and maintainability. by @MeowKJ ([`dfe31c1`](https://github.com/MeowKJ/BinaryKeyboard/commit/dfe31c1dfdfa83feda900f19af3b14dd921d7dba))
- refactor: Fn键支持长按/单点，重构并抽离 Boot 逻辑 (fixes #16)  - 增强 FnKey 识别：增加时长判定，支持单点 (CLICK) 和长按 (LONG_PRESS) - 体验优化：[无线款] 长按 Fn 自动软复位进入下载模式，无需手动拔插 USB by @MeowKJ ([`3a7893b`](https://github.com/MeowKJ/BinaryKeyboard/commit/3a7893bd8210e5e70a8608edf744e34d3c51312e))
- feat: 添加文档和指南，包含项目概览、硬件复刻、固件刷写等内容 by @MeowKJ ([`287bc42`](https://github.com/MeowKJ/BinaryKeyboard/commit/287bc42be5a21df35722661e20a359253994bd1c))
- feat: 添加 GitHub Releases 同步更新日志的工作流和脚本 by @MeowKJ ([`50e36f0`](https://github.com/MeowKJ/BinaryKeyboard/commit/50e36f072c938ffb6f966f0d38fa61fb8151091b))
- feat: 添加 pip 缓存和依赖安装步骤以优化 changelog 同步工作流 by @MeowKJ ([`5603edd`](https://github.com/MeowKJ/BinaryKeyboard/commit/5603edd9c738863a1edb30719ef5892dee9457fc))
- feat: 添加依赖文件并优化 pip 缓存配置以提升 changelog 同步效率 by @MeowKJ ([`0b07eed`](https://github.com/MeowKJ/BinaryKeyboard/commit/0b07eed05baefb2f1bad4133ddf165c534f75d16))
- feat: 添加 requests 依赖以支持脚本功能 by @MeowKJ ([`66e0fb3`](https://github.com/MeowKJ/BinaryKeyboard/commit/66e0fb3234adf96573737a4704c2d33728775b5c))
- doc: 更新文档导航和内容，优化链接路径，删除无用文件 by @MeowKJ ([`ea7caec`](https://github.com/MeowKJ/BinaryKeyboard/commit/ea7caec7560a56ac19a8e6a85c6f4894225ea2c9))
- Merge branch 'classic-keyboard' of https://github.com/MeowKJ/BinaryKeyboard into classic-keyboard by @MeowKJ ([`3a17b9d`](https://github.com/MeowKJ/BinaryKeyboard/commit/3a17b9db9937c0fdac686070aa174ab842b5d94d))
- feat: 蓝牙移植 暂存 by @MeowKJ ([`5fd3bbc`](https://github.com/MeowKJ/BinaryKeyboard/commit/5fd3bbcf49f3ac0647615003a3b25050304e2ab9))
- fix🐛: 补充CH59xBLE.a等蓝牙文件 by @MeowKJ ([`721dc0c`](https://github.com/MeowKJ/BinaryKeyboard/commit/721dc0c42c9eeaf86142ecf95a61ca473fe710dd))
- feat: 增加 WS2812B RGB 驱动代码  - 基于 TMR1 PWM + DMA 实现  Closes #11 by @MeowKJ ([`aa5ea36`](https://github.com/MeowKJ/BinaryKeyboard/commit/aa5ea36967db50930d5e16eb63e0312a4b555e0c))
- feat(ws2812): Enhance WS2812 driver with new features and demo  - Updated ws2812.h and ws2812.c to include new functions for brightness control and color conversion. - Added support for multiple LED modes in RGB_TEST.c, including breathing, rainbow, and chaser effects. - Modified Main.c to initialize WS2812 and demonstrate LED effects. - Updated project configuration to exclude User/Main.c from build. - Improved comments and code formatting for better readability. by @MeowKJ ([`e031ba3`](https://github.com/MeowKJ/BinaryKeyboard/commit/e031ba31af3da4efece4299ea821d7ec4bb052ab))
- chore(ch592f): reorganize firmware and cleanup logging by @MeowKJ ([`56297ec`](https://github.com/MeowKJ/BinaryKeyboard/commit/56297ec91a2014efa375b8ae8c34285de0dd244b))
- chore(ch592f): 固件整理与日志清理  - 将 CH592F 相关忽略规则移入子模块 .gitignore - BLE HID 日志从 DM_PRINT 迁移到 LOG_* 并补充 debug 模块 - dual_mode 重命名为 kbd_mode，刷新 BLE/USB/驱动层 - 补充按键映射  无关联 issue by @MeowKJ ([`305848d`](https://github.com/MeowKJ/BinaryKeyboard/commit/305848dc38c8cfa707525f5fada37d79884e4cd4))
- Merge branch 'classic-keyboard' of https://github.com/MeowKJ/BinaryKeyboard into classic-keyboard by @MeowKJ ([`878098b`](https://github.com/MeowKJ/BinaryKeyboard/commit/878098b0e125d858afe92a8194b31574cb7d0074))
- docs: migrate to VitePress and restructure documentation  - Migrate documentation system from old version to VitePress - Restructure documentation into classic/ and wireless/ versions - Add custom theme styles (light mode blue, dark mode pink) - Configure GitHub Actions for automatic deployment to GitHub Pages - Optimize table styles and document formatting - Clean up old documentation files and empty directories - Add .gitignore to ignore build artifacts and dependencies - Update license information and browser support details by @MeowKJ ([`a950a1f`](https://github.com/MeowKJ/BinaryKeyboard/commit/a950a1fb797bbbbe355d42ab289c8afc672b2cb7))
- docs: 更新 GitHub Pages 部署配置  - 添加 classic-keyboard 分支到部署触发分支列表 - 支持从多个分支自动部署文档 by @MeowKJ ([`08cedf7`](https://github.com/MeowKJ/BinaryKeyboard/commit/08cedf711a6f12f98355a647962f6b3a8a3d28d9))
- feat: 添加 GitHub Issue 模板  - 添加 Bug 报告模板（bug_report.yml） - 添加功能请求模板（feature_request.yml） - 支持中英文双语 - 包含版本类型、键盘类型、固件版本等必要字段 by @MeowKJ ([`a18b726`](https://github.com/MeowKJ/BinaryKeyboard/commit/a18b72654ffeaec1a62550ef28a69d171c67adb0))
- docs: add GitHub edit link to all pages by @MeowKJ ([`9665aef`](https://github.com/MeowKJ/BinaryKeyboard/commit/9665aeff888e37d5f75be91c35ff4a8a35338058))
- chore: update branch references from classic-keyboard to main by @MeowKJ ([`68af866`](https://github.com/MeowKJ/BinaryKeyboard/commit/68af8662857cccf185b1f9f4bca98a60c32639da))
- docs: sync changelog from releases by @github-actions[bot] ([`0b1dcc9`](https://github.com/MeowKJ/BinaryKeyboard/commit/0b1dcc90021696d5b58f086658e18c02201cb24b))
- Merge pull request #20 from MeowKJ/wireless-dev  StudioUI重构 by @MeowKJ ([`27b6d5f`](https://github.com/MeowKJ/BinaryKeyboard/commit/27b6d5f936315e84cd467e408542f37510286902))
- Merge pull request #22 from MeowKJ/wireless-dev  Wireless dev by @MeowKJ ([`2edd4b8`](https://github.com/MeowKJ/BinaryKeyboard/commit/2edd4b87857b3c9cab58653c89cbc2ebe7282df6))
- chore(firmware): 将 CH592F 固件版本设为 3.0.0 by @MeowKJ ([`27adc8a`](https://github.com/MeowKJ/BinaryKeyboard/commit/27adc8ac8716d5a52f490719e49105b7eb841d28))
- Merge pull request #23 from MeowKJ/dev by @MeowKJ ([`1753db3`](https://github.com/MeowKJ/BinaryKeyboard/commit/1753db39e061a443dc6b0dad326a1bd29121e9d7))
- feat(firmware): 版本统一宏 KBD_VERSION_BCD16，USB/BLE 同步；kbd_log 包含 kbd_mode.h 消除告警 by @MeowKJ ([`c7887bc`](https://github.com/MeowKJ/BinaryKeyboard/commit/c7887bcdc50b4aabe1ebfefadbe05c846c73815c))
- docs: 改改README by @MeowKJ ([`b4c7666`](https://github.com/MeowKJ/BinaryKeyboard/commit/b4c76662a1d2ff764a27f15d50cf9e0e0a3e44fe))
- chore: 代码检查功能，适配新的目录结构 by @MeowKJ ([`af83cb3`](https://github.com/MeowKJ/BinaryKeyboard/commit/af83cb33919bb0259dea7f62309b36da9c9c289b))
- fix(rgb): align studio controls and indicator-only behavior by @MeowKJ ([`c9e71ab`](https://github.com/MeowKJ/BinaryKeyboard/commit/c9e71ab0bf8ee8def0b37adb1c613e848736e663))
- Update project name and description in README by @MeowKJ ([`33e2c65`](https://github.com/MeowKJ/BinaryKeyboard/commit/33e2c65cade707a49b2c300a0ba0f2706ee97a6c))
- docs: sync changelog from releases by @github-actions[bot] ([`438fbca`](https://github.com/MeowKJ/BinaryKeyboard/commit/438fbcafd92f36587ed018d14a426706b16db656))
- feat(build): 引入 CMake 构建系统与开发工具链 by @MeowKJ ([`451a542`](https://github.com/MeowKJ/BinaryKeyboard/commit/451a542dd4dda4a155e76c4503b54eac72939758))
- fix(studio): 移除 vite-plugin-vue-devtools 修复 Node.js 启动崩溃 by @MeowKJ ([`19f5595`](https://github.com/MeowKJ/BinaryKeyboard/commit/19f5595f40054344bb635d5e225ef1c2e26745bf))
- feat(studio): 欢迎页加入旧版改键工具链接 by @MeowKJ ([`ec6093a`](https://github.com/MeowKJ/BinaryKeyboard/commit/ec6093a167a8127d9d1608ce872260601409ee8e))
- chore(firmware): 移除旧版 MounRiver Studio 工程文件 by @MeowKJ ([`1663dbb`](https://github.com/MeowKJ/BinaryKeyboard/commit/1663dbbfeb9806c3f78a11af0c8df52ea02fedc9))
- docs: 整合固件 README，更新无线版构建文档至 CMake by @MeowKJ ([`98ad610`](https://github.com/MeowKJ/BinaryKeyboard/commit/98ad610ce30de3a4ccca2918211711764df47df3))
- ci: 引入固件编译检查与自动 Release 工作流 by @MeowKJ ([`1846fa4`](https://github.com/MeowKJ/BinaryKeyboard/commit/1846fa489b53bc9a6b0f3ba324ce4c6f84dab21d))
- fix(ci): 修复 Docker 镜像名大小写问题，镜像名须全小写 by @MeowKJ ([`9e2431c`](https://github.com/MeowKJ/BinaryKeyboard/commit/9e2431c00e513b76c0817aa00b5bfb21ddeea94c))
- fix(ci): 修复容器内 git safe.directory 报错 by @MeowKJ ([`04e4f8a`](https://github.com/MeowKJ/BinaryKeyboard/commit/04e4f8a6bab6398f3b7f995b44fb6a70a772389e))
- fix(ci): 自动 Release 改为 pre-release by @MeowKJ ([`99ea4e1`](https://github.com/MeowKJ/BinaryKeyboard/commit/99ea4e1809edb04fb029b0f96f0f7a690785effa))
- docs: sync changelog from releases by @github-actions[bot] ([`1d94b30`](https://github.com/MeowKJ/BinaryKeyboard/commit/1d94b30209dee91facce13520fa4bb19d13ae7a1))
- fix: CH592F CMake编译错误 by @MeowKJ ([`e153974`](https://github.com/MeowKJ/BinaryKeyboard/commit/e15397458d9cc7abaec7e915cf79ddda67f31028))
- feat: 重构存储架构、完善 BLE/USB HID 及 Studio PWA 支持 by @MeowKJ ([`fd5091c`](https://github.com/MeowKJ/BinaryKeyboard/commit/fd5091cfcda5e50f47ef12b97df01f9b0cc6ebb5))
- chore: 整理项目结构，添加 __pycache__ 到 .gitignore by @MeowKJ ([`ea71d5c`](https://github.com/MeowKJ/BinaryKeyboard/commit/ea71d5c2dd427ff72b3fe27a28933931513c4620))
- Improve CH592F toolchain guidance by @MeowKJ ([`693f20e`](https://github.com/MeowKJ/BinaryKeyboard/commit/693f20eb0a1165f0132c5e767db7a115cb57e3f1))
- Add project console and refresh CH592F workflow docs by @MeowKJ ([`d21a79a`](https://github.com/MeowKJ/BinaryKeyboard/commit/d21a79a0fba951c61896f927151f217e7dca69de))
- Trim CH592F README down to doc links by @MeowKJ ([`ebbfac6`](https://github.com/MeowKJ/BinaryKeyboard/commit/ebbfac63d4d602722ece3bafb3422389c6c45747))
- Converge subdirectory READMEs into doc links by @MeowKJ ([`e00a875`](https://github.com/MeowKJ/BinaryKeyboard/commit/e00a8750813820de8272d650a1dfdcab8c48815f))
- 更新 README.md，简化文档结构，移除多余内容，统一设计描述 by @MeowKJ ([`812d9dc`](https://github.com/MeowKJ/BinaryKeyboard/commit/812d9dcf44e612042c00ea5a8c21662862be2cd7))
- Unify CH552G tooling and project TUI workflows by @MeowKJ ([`1a58450`](https://github.com/MeowKJ/BinaryKeyboard/commit/1a58450431d59363a5b4aab0063cedbcd8954dc9))
- Refactor device layers and unify firmware build workflow by @MeowKJ ([`927a1b9`](https://github.com/MeowKJ/BinaryKeyboard/commit/927a1b975c7fee138ee890f481afe91595dcc390))
- docs: sync changelog from releases by @github-actions[bot] ([`3eb7b3a`](https://github.com/MeowKJ/BinaryKeyboard/commit/3eb7b3a060aff424fafd30df4cd02b3fd8cd69a3))
- feat: restructure firmware, enhance Studio & release pipeline by @MeowKJ ([`bf2d7eb`](https://github.com/MeowKJ/BinaryKeyboard/commit/bf2d7eba17b4022c3b6e534835f6adfc06a78fa5))
- docs: update README hero image layout by @MeowKJ ([`2fdf102`](https://github.com/MeowKJ/BinaryKeyboard/commit/2fdf102599dda488e259871037e1569e5a6c01ec))
- fix: CI build failures - add python3 to container, fix flashLayerColor params by @MeowKJ ([`81c0cfa`](https://github.com/MeowKJ/BinaryKeyboard/commit/81c0cfa89ad4a532309d38263b44a4d1ff6207a2))
- fix: resolve CH592F version mismatch and beautify release notes by @MeowKJ ([`ff9c568`](https://github.com/MeowKJ/BinaryKeyboard/commit/ff9c568c922ce5349f9825d6fe4e001300d222f8))
- style: simplify release title and add tool versions to build env by @MeowKJ ([`a6b84d5`](https://github.com/MeowKJ/BinaryKeyboard/commit/a6b84d5aa907f764ca2b2fa8d243dfc3ac64b397))
- style: format deploy-docs.yml and CustomUSBHID.c by @MeowKJ ([`179259c`](https://github.com/MeowKJ/BinaryKeyboard/commit/179259cf1d5f204b505c810877f8fd47b8d4aeb5))
- docs: sync changelog from releases by @github-actions[bot] ([`2d4b9f1`](https://github.com/MeowKJ/BinaryKeyboard/commit/2d4b9f1a96a888fe9a4ad7b557dd52d160dd5b07))
- 为发布流程补充固件尺寸报告 by @MeowKJ ([`4390c72`](https://github.com/MeowKJ/BinaryKeyboard/commit/4390c72a52205da0df3d669c770cbedabfdf1efa))
- Initial plan by @Copilot ([`b9b629f`](https://github.com/MeowKJ/BinaryKeyboard/commit/b9b629fe12485b76dc43526c55f85cbb53011f30))
- Fix issue #41: Add clear key button to support modifier-key-only remapping by @Copilot ([`4c068ae`](https://github.com/MeowKJ/BinaryKeyboard/commit/4c068aeb3e95119ab39e231edf18ffd49ce4e33e))
- Remove accidentally committed package-lock.json by @Copilot ([`d77b6db`](https://github.com/MeowKJ/BinaryKeyboard/commit/d77b6db513ee2cc86cbfc34c048adc43204d0c07))
- docs: sync changelog from releases by @github-actions[bot] ([`58e643b`](https://github.com/MeowKJ/BinaryKeyboard/commit/58e643b6c5b8c6f224b0a264a11979127b901bf3))
- feat: add textual console and unify script workflows by @MeowKJ ([`4a35a61`](https://github.com/MeowKJ/BinaryKeyboard/commit/4a35a61b7f92d920cf8564ebb1cdc6dd973efbb6))
- 整理双模固件、工具链与文档 by @MeowKJ ([`26e6f8f`](https://github.com/MeowKJ/BinaryKeyboard/commit/26e6f8f9be782ba70fad3aa2f250e507945e5194))
- Simplify console startup and bootstrap venv by @MeowKJ ([`c0912c5`](https://github.com/MeowKJ/BinaryKeyboard/commit/c0912c5c5abc0ce0327f259525ae3be5a563e869))
- Fix #44 sync CH592 press effect layering with CH552 by @MeowKJ ([`68a2814`](https://github.com/MeowKJ/BinaryKeyboard/commit/68a2814b6c502b5744976098b2a396b7eb38842c))
- Keep CH592 light fade idle state dark by @MeowKJ ([`bcea76d`](https://github.com/MeowKJ/BinaryKeyboard/commit/bcea76d8cb2010bbe253558620a2caac55c3f083))
- Fix #46 launch dev terminal on macOS by @MeowKJ ([`f5580b9`](https://github.com/MeowKJ/BinaryKeyboard/commit/f5580b91d72590ab7ce5adfb00f79174e060365b))
- fix: 使用 Fluent Emoji 3D 图片替换原生猫咪 emoji，统一跨平台显示效果 by @MeowKJ ([`5f85a47`](https://github.com/MeowKJ/BinaryKeyboard/commit/5f85a4767503f11eba6338fb4ca75fffc7960a40))
- 调整 Release 命名并修正 CH592 尺寸输出 by @MeowKJ ([`9dc0b59`](https://github.com/MeowKJ/BinaryKeyboard/commit/9dc0b59dca188982c6ed45c53a96e84771964c66))
- 修正 CH592 脚本缺失 json 导入 by @MeowKJ ([`368bb42`](https://github.com/MeowKJ/BinaryKeyboard/commit/368bb42d62af224d21f6fb9d0ffcfba4f1443814))
- Update console image URL in README.md by @MeowKJ ([`789d8bd`](https://github.com/MeowKJ/BinaryKeyboard/commit/789d8bd40fa90ad7f01df48bd27b6f62c6b4dff6))
- 修复前端依赖安全告警 by @MeowKJ ([`4c7e427`](https://github.com/MeowKJ/BinaryKeyboard/commit/4c7e42719e1c28dfd30e6146ad598bb17c865ecf))
- 新增 CH592 宏功能与编辑器 by @MeowKJ ([`432c859`](https://github.com/MeowKJ/BinaryKeyboard/commit/432c859343941e63540c444f39bbcb7a3ffa26b3))
- 修正 CH552G 切层提示文案 by @MeowKJ ([`e1cce62`](https://github.com/MeowKJ/BinaryKeyboard/commit/e1cce62e19118ffcb4b0014130229d8d7a3795cc))
- docs: sync changelog from releases by @github-actions[bot] ([`d654cec`](https://github.com/MeowKJ/BinaryKeyboard/commit/d654cec4eddb3acc78b573bc911f29a3ae2c7b92))
- build(ch552g): 统一 code-size=14208 预留宏区 by @MeowKJ ([`15254d1`](https://github.com/MeowKJ/BinaryKeyboard/commit/15254d12201a95af7e6bb15ce0c5890c24da00a3))
- ch552g: 预留192B宏区并加入MacroStorage(2x96B) by @MeowKJ ([`9a02219`](https://github.com/MeowKJ/BinaryKeyboard/commit/9a02219e83d754383fc02fce9c97542f5974b258))
- feat(ch552g): 增加宏按键类型与宏HID命令 by @MeowKJ ([`c3329c5`](https://github.com/MeowKJ/BinaryKeyboard/commit/c3329c53b33062968fcade67c516545ed0871779))
- fix(rgb): light-fade叠加基底并优化CH552G体积 by @MeowKJ ([`30b8de2`](https://github.com/MeowKJ/BinaryKeyboard/commit/30b8de29c9c5cc45aea59fc52dcf16e08056cf48))
- chore(console): Windows重启venv时透传参数 by @MeowKJ ([`fb1e4be`](https://github.com/MeowKJ/BinaryKeyboard/commit/fb1e4be922da7cfcac9ebcbb4d6810620d5c1fcb))
- feat(studio): MeowMacro代码模式与CH552宏槽位适配 by @MeowKJ ([`c933174`](https://github.com/MeowKJ/BinaryKeyboard/commit/c93317422dec66041e661c750631b4a904e67ec9))
- docs: 增加MeowMacro文档与宏编辑器指南 by @MeowKJ ([`c35db38`](https://github.com/MeowKJ/BinaryKeyboard/commit/c35db3889e0494ee6a274f0a809dd9e88e4b8bc3))
- ci(release): 发布说明加入RAM占用与变更记录 by @MeowKJ ([`47aeca3`](https://github.com/MeowKJ/BinaryKeyboard/commit/47aeca312a4af2cf1c528e511500ae404284bf25))
- docs: sync changelog from releases by @github-actions[bot] ([`4089504`](https://github.com/MeowKJ/BinaryKeyboard/commit/4089504d6214fa7ace172c4f4a2e145c5e8e54c6))
- build(ch552g): 提取 BSP 层并优化构建配置 by @MeowKJ ([`1d5c149`](https://github.com/MeowKJ/BinaryKeyboard/commit/1d5c149b492ebfbef0d6434182eb3148dc1caa69))
- refactor(ch552g): 精简 RGB 模块减少代码体积 by @MeowKJ ([`43517c9`](https://github.com/MeowKJ/BinaryKeyboard/commit/43517c99a5a11ed269895d3feee6d8dd2cce1ba3))
- feat(ch552g): MeowFS 重构、Flash 擦写修复与出厂重置 by @MeowKJ ([`4d1fd4e`](https://github.com/MeowKJ/BinaryKeyboard/commit/4d1fd4ea956a1cc25c63c9fc937bde982f5ca3e6))
- feat(ch592f): MeowFS 宏存储适配与命令处理重构 by @MeowKJ ([`a0983b5`](https://github.com/MeowKJ/BinaryKeyboard/commit/a0983b56abac28491542ef32aa1dd9da02eabea7))
- feat(studio): MeowFS 协议适配与宏编辑器增强 by @MeowKJ ([`a359049`](https://github.com/MeowKJ/BinaryKeyboard/commit/a3590493820a415fd2899243e37a6c4caabf2149))
- docs: 增加 MeowFS 技术文档 by @MeowKJ ([`970b15a`](https://github.com/MeowKJ/BinaryKeyboard/commit/970b15a3174e34880b70f1efdcb89f4ce93505f1))
- ci(release): 切换到 HEX 格式并美化发布说明 by @MeowKJ ([`759d83d`](https://github.com/MeowKJ/BinaryKeyboard/commit/759d83da75c247d869aa2566ad8de9c95a70ce2c))
- fix(ci): 修复 release.yml YAML heredoc 缩进语法错误 by @MeowKJ ([`609911e`](https://github.com/MeowKJ/BinaryKeyboard/commit/609911e3bad0ecf4050117d513b42e9fdb8156d3))
- fix(ch552g): 修复 IRAM/OSEG 溢出和 Flash 超限 by @MeowKJ ([`89b8a67`](https://github.com/MeowKJ/BinaryKeyboard/commit/89b8a673b755f8be5a614f0c28548135fe9acb4a))
- fix(ci): CI 安装 SDCC 4.5.0 替代 apt 旧版 4.2.0 by @MeowKJ ([`23ec0f6`](https://github.com/MeowKJ/BinaryKeyboard/commit/23ec0f6869e5f08723bb84bcb033282a567cc7c1))
- ci(release): simplify notes and improve author attribution by @MeowKJ ([`2e0aa68`](https://github.com/MeowKJ/BinaryKeyboard/commit/2e0aa68a8fd8b88d1ec6617f3f635fe3a61206a8))
- ci(release): reorder notes and clean generated text by @MeowKJ ([`f58ceee`](https://github.com/MeowKJ/BinaryKeyboard/commit/f58ceee5af44ac8c2033c27043780c740adc5ad1))
- feat: 添加 MeowMacro 语言支持，更新文档和清理标签 by @MeowKJ ([`ef9661d`](https://github.com/MeowKJ/BinaryKeyboard/commit/ef9661d8d48cfe6a218edec5bb569e49ceae9957))
- docs(wireless): integrate BOM into make.md; mark RGB boost optional by @MeowKJ ([`e1fa403`](https://github.com/MeowKJ/BinaryKeyboard/commit/e1fa4030ebb67899cb2cf4b462504e5399930db1))
- feat(studio): 添加 Electron 桌面版打包支持 (Mac/Win) by @MeowKJ ([`b517cbd`](https://github.com/MeowKJ/BinaryKeyboard/commit/b517cbda875b0cf66db382483bc0f3efc7404fdc))
- fix(ch592f): FN 键 RGB 操作后保存配置到 DataFlash by @MeowKJ ([`86f3bc8`](https://github.com/MeowKJ/BinaryKeyboard/commit/86f3bc8dc1464d5bd1422b1baeb17495552d2d64))
- feat(studio): sync-versions 同时更新 package.json version by @MeowKJ ([`5e2d11a`](https://github.com/MeowKJ/BinaryKeyboard/commit/5e2d11a11a407e3b187c0440de4b7d829c705bc0))

**Full Changelog**: https://github.com/MeowKJ/BinaryKeyboard/compare/binarykeyboard-20260316-f661ba3...binarykeyboard-20260318

---

<details><summary>构建环境</summary>

| 属性 | 值 |
|:-----|:---|
| 构建日期 |  |
| Commit | [``](https://github.com/MeowKJ/BinaryKeyboard/commit/) |
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
c63ce3152b51ab747340e9bfd202a8abb32a06d4c81fdf61705d5a6f7f2ec873  CH592F-5KEY-3.0.27.hex
a6912c1394656bf4590ad4b8b688ac0b1f6a6f28626a845e91cfbd69ce814184  CH592F-KNOB-3.0.27.hex
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

