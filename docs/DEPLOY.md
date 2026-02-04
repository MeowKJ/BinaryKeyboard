# 部署到 GitHub Pages

## 自动部署（推荐）

已配置 GitHub Actions 工作流，推送到 `main` 分支的 `docs/` 目录更改会自动触发部署。

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

### 访问地址

部署完成后，文档将发布到：
```
https://meowkj.github.io/BinaryKeyboard/
```

## 手动部署

如果需要手动部署：

```bash
cd docs
pnpm install
pnpm run build
```

构建产物在 `docs/.vitepress/dist` 目录，可以手动上传到 GitHub Pages。

## 注意事项

- 确保 `base` 路径与仓库名称一致（当前为 `/BinaryKeyboard/`）
- 如果仓库在组织下，路径可能需要调整
- 首次部署可能需要几分钟时间
