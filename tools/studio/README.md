# BinaryKeyboard Studio

BinaryKeyboard 的 Web 改键工具前端（Vue 3 + Vite + PrimeVue），通过 WebHID 与键盘通信。

## 本次大更新（前端）

- 新增 PWA 支持（Manifest + Service Worker 缓存）。
- 新增新版本提示条，检测到更新后可一键刷新。
- 开发服务器固定为 `0.0.0.0:5173`，并启用 `strictPort`（端口被占用会直接报错）。
- 新增 PWA 客户端类型声明（`env.d.ts`）。

## 开发环境

- Node.js（建议 LTS）
- `pnpm`

推荐 IDE：VS Code + Volar（禁用 Vetur）。

## 快速开始

```sh
pnpm install
pnpm dev
```

默认地址：`http://localhost:5173`

## 常用命令

```sh
pnpm dev        # 本地开发（可局域网访问）
pnpm type-check # 类型检查
pnpm build      # 生产构建
pnpm preview    # 本地预览构建结果
```

## PWA 说明

- 构建后支持安装为应用（桌面/移动端，取决于浏览器）。
- 检测到新版本时，页面顶部会显示更新提示。
- WebHID 仍需 Chromium 内核浏览器（如 Chrome / Edge）。
- 线上环境建议使用 HTTPS（`localhost` 本地开发可直接使用）。

## 发布前检查

- 确认 `tools/studio/public/icon-192.png` 与 `tools/studio/public/icon-512.png` 已存在（PWA Manifest 依赖）。
- 确认静态资源可被缓存（VitePWA 已配置 Workbox 缓存规则）。
