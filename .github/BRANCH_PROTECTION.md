# Main 分支保护规则配置指南

本文档说明如何为 `main` 分支配置基于 AI 的代码自动检查规则。

## 📋 配置步骤

### 1. 访问分支保护设置

1. 进入仓库的 **Settings** → **Branches**
2. 在 **Branch protection rules** 部分，点击 **Add rule** 或编辑现有的 `main` 分支规则
3. 在 **Branch name pattern** 中输入 `main`

### 2. 基础保护设置

#### ✅ 必须启用的选项

- **✅ Require a pull request before merging**
  - 要求所有更改必须通过 Pull Request
  - 建议设置：至少需要 1 个审查者批准

- **✅ Require status checks to pass before merging**
  - 必须通过的状态检查：
    - `CodeQL Security Analysis (cpp)`
    - `CodeQL Security Analysis (javascript)`
    - `JavaScript/TypeScript Quality Check`
    - `C/C++ Quality Check`
    - `Code Quality & AI Review / quality-summary`

- **✅ Require branches to be up to date before merging**
  - 确保分支与 main 保持同步

- **✅ Block force pushes**
  - 阻止强制推送，保护代码历史

- **✅ Block deletions**
  - 阻止删除 main 分支

### 3. AI 代码审查配置

#### ✅ 自动请求 Copilot 代码审查

启用 **Automatically request Copilot code review**：

- ✅ 勾选此选项
- 描述：如果作者有权访问 Copilot 代码审查且其高级请求配额未达到上限，则自动为新的拉取请求请求 Copilot 代码审查
- **效果**：每个新的 PR 将自动触发 AI 代码审查

#### ✅ 在 Copilot 代码审查中管理静态分析工具

启用 **Manage static analysis tools in Copilot code review**：

- ✅ 勾选此选项（预览功能）
- 描述：Copilot 代码审查将包含来自所选静态分析工具的发现作为其审查评论

**配置静态分析工具**：

点击 **Select tools** 下拉菜单，添加以下工具：

1. **CodeQL**
   - 类型：Standard queries（标准查询）
   - 用途：安全漏洞检测和代码质量分析

2. **ESLint**
   - 类型：Standard rules（标准规则）
   - 用途：JavaScript/TypeScript 代码规范检查
   - 适用范围：`Software/binary-keyboard-studio-ui/**`

3. **cppcheck**（如果可用）
   - 类型：Standard rules（标准规则）
   - 用途：C/C++ 代码静态分析
   - 适用范围：`Firmware/**`

### 4. 代码扫描结果要求

#### ✅ 要求代码扫描结果

启用 **Require code scanning results**：

**所需工具和警报阈值**：

- **CodeQL**
  - 安全警报（Security alerts）：**High or higher**（高或更高）
  - 警报（Alerts）：**Errors**（错误）
  - 说明：只有高严重性的安全问题和错误级别的代码质量问题会阻止合并

**添加工具**：
- 点击 **+ Add tool** 可以添加更多代码扫描工具

### 5. 代码质量结果要求（可选）

#### ⚠️ 要求代码质量结果

**Require code quality results**（可选启用）：

- 如果启用，选择哪些严重级别的代码质量结果应阻止拉取请求合并
- 建议设置：
  - **Block on**: Errors（错误）
  - **Warn on**: Warnings（警告）

## 🔧 自动化工作流

项目已配置以下 GitHub Actions 工作流：

### `code-quality.yml`

自动执行以下检查：

1. **CodeQL 安全扫描**
   - 语言：C/C++、JavaScript
   - 触发：PR、推送到 main、每日定时扫描

2. **JavaScript/TypeScript 质量检查**
   - ESLint 代码规范检查
   - TypeScript 类型检查
   - 构建验证

3. **C/C++ 质量检查**
   - cppcheck 静态分析
   - 代码质量报告

4. **代码复杂度分析**
   - 代码重复检测
   - 复杂度报告

## 📊 检查结果查看

### 在 Pull Request 中查看

1. 打开任意 Pull Request
2. 在 **Checks** 标签页查看所有检查结果
3. 在 **Files changed** 标签页查看 Copilot 的代码审查评论

### 在 Security 标签页查看

1. 进入仓库的 **Security** 标签页
2. 查看 **Code scanning alerts** 了解安全漏洞
3. 查看 **Dependabot alerts** 了解依赖漏洞

## 🎯 最佳实践

1. **及时修复问题**
   - 优先修复高严重性的安全问题
   - 修复错误级别的代码质量问题

2. **审查 AI 建议**
   - 仔细阅读 Copilot 的代码审查评论
   - 根据项目实际情况决定是否采纳建议

3. **保持代码质量**
   - 在提交前本地运行 `pnpm lint` 和 `pnpm type-check`
   - 确保所有检查通过后再创建 PR

4. **定期审查**
   - 每周查看代码扫描报告
   - 关注安全警报和代码质量趋势

## 🔗 相关资源

- [GitHub 分支保护规则文档](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-protected-branches/about-protected-branches)
- [GitHub Copilot 代码审查文档](https://docs.github.com/en/copilot/using-github-copilot/github-copilot-code-review)
- [CodeQL 文档](https://codeql.github.com/docs/)
- [ESLint 文档](https://eslint.org/docs/latest/)

## 📝 配置检查清单

在配置完成后，请确认以下项目：

- [ ] 已启用 Pull Request 要求
- [ ] 已配置必需的状态检查
- [ ] 已启用 Copilot 自动代码审查
- [ ] 已配置静态分析工具（CodeQL、ESLint）
- [ ] 已设置代码扫描结果要求
- [ ] 已阻止强制推送和删除
- [ ] GitHub Actions 工作流正常运行
- [ ] 团队成员了解新的代码审查流程

---

**注意**：某些功能（如 Copilot 代码审查）可能需要 GitHub Enterprise 或 GitHub Team 订阅。请确认您的账户权限。
