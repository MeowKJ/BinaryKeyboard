# BinaryKeyboard TODO

本文件跟踪当前重构方向。核心原则：**协议驱动 + 薄固件 + 智能 Studio**。

## 架构决策

- 固件 CH552G / CH592F 各自独立，不做跨芯片 kernel 抽象
- Studio 负责多设备适配（HID 插件 + Codec + UI Definition）
- 协议差异由 Studio Codec 层消化，固件只需实现各自的线格式
- 不模仿 QMK/VIA 架构（键数少、芯片异构、资源受限）

## 已完成

1. Studio HID 插件架构
   - `services/hid/common/` — BaseHidAdapter, codecTypes, types
   - `services/hid/devices/ch552/` — adapter, codec, ui, index
   - `services/hid/devices/ch592/` — adapter, codec, ui, index
   - `services/hid/registry.ts` — 插件注册表

2. DeviceUiDefinition 驱动面板显示
   - `types/deviceUi.ts` — section 定义 + 工厂函数
   - App.vue 通过 `plugin.getUiDefinition()` 动态渲染面板

3. Scripts target profile 注册表
   - `scripts/targets/common.py` — TargetProfile 数据类
   - `scripts/targets/ch552/` / `ch592/` — 各自 profile
   - `scripts/targets/registry.py` — 注册 + 查询

4. 版本元数据集中管理
   - `config/versions.json` — 单一真理源
   - `tools/studio/scripts/sync-versions.mjs` — 生成 versionConfig.ts

## 进行中

1. 协议版本兼容性检查 — 连接时比对 protocolVersion，不兼容时 toast 警告
2. App.vue 组件拆分 — 侧边栏 5 个面板抽成独立 .vue 组件
3. 扩展 versions.json — 增加 Studio 支持的协议版本范围

## 下一步

1. 拆出侧边栏组件
   - `DeviceInfoPanel` — 设备信息展示
   - `LayerPanel` — 层选择 mini 键盘
   - `FnPanel` — FN 键配置
   - `RgbPanel` — RGB 灯效配置
   - `ActionsPanel` — 保存/撤销/重置按钮

2. 协议兼容范围
   - `config/versions.json` 增加 `studio.supported_protocols`
   - `sync-versions.mjs` 生成到 versionConfig.ts
   - `checkProtocolCompatibility()` 改用 min/max 范围判断

## 明确不做

- `firmware/kernel + platforms + devices` — 8051 与 RISC-V 无法共享 kernel
- Section Renderer 动态渲染引擎 — 2 个设备不需要
- JSON → 双端 codegen — 两个协议帧格式完全不同
- VIA JSON 运行时协议 — CH552G 资源不够

## 工作约定

1. 每完成一阶段，更新本文件
2. 每次重构后执行验证：
   - `pnpm --dir tools/studio exec tsc --noEmit`
   - `pnpm --dir tools/studio build`
   - `python3 tools/scripts/ch552g.py build --keyboard BASIC`
   - `python3 tools/scripts/ch592f.py build --keyboard 5KEY --profile release`
