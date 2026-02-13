# AI 代码自动检查方案 - 快速开始指南

## 📦 方案概述

本方案为 `main` 分支提供了一套完整的基于 AI 的代码自动检查系统，包括：

- 🤖 **GitHub Copilot 自动代码审查**
- 🔍 **CodeQL 安全扫描**
- 📝 **ESLint 代码规范检查**
- 🔧 **C/C++ 静态分析**
- 📊 **代码质量报告**

## 🚀 快速配置（5 分钟）

### 步骤 1：安装依赖（本地开发）

```bash
cd Software/binary-keyboard-studio-ui
pnpm install
```

这将安装 ESLint 和相关工具。

### 步骤 2：配置 GitHub 分支保护规则

1. 访问仓库：`Settings` → `Branches`
2. 编辑或创建 `main` 分支保护规则
3. 按照 [BRANCH_PROTECTION.md](./BRANCH_PROTECTION.md) 中的详细说明配置

**关键配置项**：
- ✅ 启用 "Automatically request Copilot code review"
- ✅ 启用 "Manage static analysis tools in Copilot code review"
- ✅ 添加 CodeQL 和 ESLint 工具
- ✅ 配置必需的状态检查

### 步骤 3：验证 GitHub Actions

1. 创建一个测试 Pull Request
2. 检查 Actions 标签页，确认工作流正常运行
3. 查看 PR 中的 Copilot 审查评论

## 📁 文件结构

```
.github/
├── workflows/
│   └── code-quality.yml          # 代码质量检查工作流
├── codeql-config.yml             # CodeQL 配置文件
├── BRANCH_PROTECTION.md          # 分支保护规则详细配置
├── COPILOT_REVIEW.md             # Copilot 审查配置指南
└── AI_CODE_REVIEW_SETUP.md       # 本文件

Software/binary-keyboard-studio-ui/
├── .eslintrc.cjs                 # ESLint 配置文件
└── package.json                  # 已更新，包含 lint 脚本
```

## 🔧 本地使用

### 运行代码检查

```bash
# JavaScript/TypeScript 代码检查
cd Software/binary-keyboard-studio-ui
pnpm run lint          # 自动修复问题
pnpm run lint:check    # 仅检查，不修复

# TypeScript 类型检查
pnpm run type-check
```

### 提交前检查

建议在提交代码前运行：

```bash
cd Software/binary-keyboard-studio-ui
pnpm run lint
pnpm run type-check
pnpm run build-only
```

## 📋 工作流程

### 1. 创建 Pull Request

当您创建 PR 时：

1. **自动触发**：
   - GitHub Actions 运行代码质量检查
   - CodeQL 执行安全扫描
   - Copilot 开始代码审查

2. **检查结果**：
   - 在 PR 的 **Checks** 标签页查看状态
   - 在 **Files changed** 标签页查看 Copilot 评论
   - 在 **Security** 标签页查看安全警报

### 2. 审查和修复

1. **查看 AI 建议**：
   - 阅读 Copilot 的审查评论
   - 查看 CodeQL 的安全警报
   - 检查 ESLint 的规范问题

2. **修复问题**：
   - 优先修复高严重性的安全问题
   - 修复错误级别的代码质量问题
   - 根据实际情况决定是否采纳风格建议

3. **重新提交**：
   - 修复后推送到 PR 分支
   - 检查会自动重新运行

### 3. 合并 PR

当所有检查通过后：

- ✅ CodeQL 扫描无高严重性问题
- ✅ ESLint 检查通过
- ✅ TypeScript 类型检查通过
- ✅ 构建成功
- ✅ Copilot 审查完成（可选）

## 🎯 检查项说明

### CodeQL 安全扫描

- **语言**：C/C++、JavaScript
- **检查内容**：
  - 安全漏洞（SQL 注入、XSS、缓冲区溢出等）
  - 代码质量问题（空指针、内存泄漏等）
- **阈值**：高严重性问题会阻止合并

### ESLint 代码规范

- **适用范围**：`Software/binary-keyboard-studio-ui/**`
- **检查内容**：
  - Vue 组件规范
  - TypeScript 类型安全
  - JavaScript 最佳实践
- **配置文件**：`.eslintrc.cjs`

### C/C++ 静态分析

- **工具**：cppcheck
- **适用范围**：`Firmware/**`
- **检查内容**：
  - 内存错误
  - 未定义行为
  - 代码质量

### Copilot 代码审查

- **触发**：自动（每个新 PR）
- **内容**：
  - 代码改进建议
  - 安全最佳实践
  - 性能优化建议
  - 集成静态分析结果

## 📊 监控和维护

### 查看报告

1. **Security 标签页**：
   - 查看 CodeQL 安全警报
   - 查看依赖漏洞

2. **Insights → Code review**：
   - 查看审查统计
   - 分析问题趋势

3. **Actions 标签页**：
   - 查看工作流运行历史
   - 下载检查报告

### 定期维护

- **每周**：查看安全警报和代码质量趋势
- **每月**：审查和更新 ESLint 规则
- **每季度**：评估 Copilot 审查质量，调整配置

## 🆘 故障排除

### 问题：GitHub Actions 失败

**解决方案**：
1. 检查 Actions 日志，查看具体错误
2. 确认依赖安装成功
3. 检查配置文件语法

### 问题：Copilot 审查未触发

**解决方案**：
1. 确认已启用 "Automatically request Copilot code review"
2. 检查账户是否有 Copilot 订阅
3. 确认有足够的配额

### 问题：ESLint 检查失败

**解决方案**：
1. 本地运行 `pnpm run lint` 查看问题
2. 运行 `pnpm run lint` 自动修复可修复的问题
3. 手动修复剩余问题

## 📚 详细文档

- [分支保护规则配置](./BRANCH_PROTECTION.md) - 详细的 GitHub 设置说明
- [Copilot 审查配置](./COPILOT_REVIEW.md) - Copilot 功能详解和使用技巧

---

**需要帮助？** 查看详细文档或提交 Issue。
