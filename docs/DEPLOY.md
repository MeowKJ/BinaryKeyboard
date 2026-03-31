# 部署到 GitHub Pages

GitHub Pages 同时发布以下静态文件：

- `api/release-manifest.json`
- `firmware/` 下的最新固件镜像

Vercel 上的 BinaryKeyboard Studio 通过 Pages 上的 manifest 和固件文件完成下载。

## 自动部署

GitHub Actions 工作流使用以下触发方式：

- `main` 上 `docs/` 或 `deploy-docs.yml` 变化时自动部署
- `Release Firmware` 在 `main` 完成后自动重新部署 Pages

固件发布后，Pages 会同步最新固件和 manifest，Studio 按此内容下载。

### 首次设置

1. 在 GitHub 仓库设置中启用 GitHub Pages：
   - 进入仓库 Settings → Pages
   - Source 选择：**GitHub Actions**

2. 推送代码到 `main` 分支：
   ```bash
   git add .
   git commit -m "Update docs"
   git push origin main
   ```

3. 查看部署状态：
   - 进入仓库 Actions 标签页
   - 查看 "Deploy Docs to GitHub Pages" 工作流运行状态

## 当前会发布什么

`Deploy Docs to GitHub Pages` 包含以下步骤：

1. 构建文档站点
2. 从 latest release 同步最新固件到 `docs/public/firmware/`
3. 生成 `docs/public/api/release-manifest.json`
4. 一起发布到 Pages

当前策略是：

- **Pages 只镜像 latest release 的固件**
- 历史完整文件仍然放在 GitHub Releases

### 访问地址

部署完成后，文档将发布到：
```
https://meowkj.github.io/BinaryKeyboard/
```

## 手动部署

如果需要手动部署：

```bash
# Firmware + manifest are now provided via the pages-firmware workflow artifact
# from the latest successful Release Firmware run. Place them manually:
#   docs/public/firmware/ch592f/  — full.hex + app.bin
#   docs/public/firmware/ch552g/  — hex
#   docs/public/api/release-manifest.json

cd docs
pnpm install
pnpm run build
```

构建产物在 `docs/.vitepress/dist` 目录，可以手动上传到 GitHub Pages。

## 注意事项

- 确保 `base` 路径与仓库名称一致（当前为 `/BinaryKeyboard/`）
- 如果仓库在组织下，路径可能需要调整
- 首次部署可能需要几分钟时间
- 如果 Release 已更新，但 `firmware/` 中的文件仍未同步，等待下一次 Pages 部署完成即可
