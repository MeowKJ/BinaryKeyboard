# 便捷开发工具

::: warning
如果你使用 `MounRiver Studio` 开发，这一页可以直接跳过。
:::

这里就放两个可选小工具：

- `tools/scripts/console.py`（统一 TUI 入口）
- `.vscode/tasks.json` / `.vscode/settings.json`

## 怎么选

- 喜欢在终端里点一点、选一点，或没有桌面环境，用 `tools/scripts/console.py`
- 使用 VS Code，就直接用 `.vscode/tasks.json` 和 `.vscode/settings.json`

## PY 终端控制台

```bash
python tools/scripts/console.py
```

```bash
python tools/scripts/console.py --text
```

- `Home`：当前布局、preset、常用入口
- `ISP`：`probe / info / verify / erase / reset / eeprom / config`
- `Studio`：前端依赖安装和构建
- `Docs`：文档依赖安装和构建
- `Doctor`：环境检查
- `Links`：一些常用页面

- 鼠标现在是“单击选中，双击执行”
- `Enter` 也能执行当前选中的动作
- `Configure toolchain` 在有桌面环境的 macOS / Windows / Linux 下都会尽量弹文件夹选择器

## VS Code 任务

- `.vscode/tasks.json`
- `.vscode/settings.json`

- 可以配合 Task Buttons 一类插件一起用

- 完整配置放在文档最下面的附件里

## 附件 A：完整 `.vscode/tasks.json`

```jsonc
{
  "version": "2.0.0",
  "tasks": [
    // ── Flash ───────────────────────────────────────────────────────────────
    {
      "label": "WCH: Build & Flash (release)",
      "type": "shell",
      "command": "python3",
      "windows": { "command": "python" },
      "args": [
        "${workspaceFolder}/tools/scripts/flash.py",
        "flash",
        "--preset",
        "release"
      ],
      "group": { "kind": "build", "isDefault": true },
      "presentation": { "reveal": "always", "panel": "shared", "clear": true },
      "problemMatcher": []
    },
    {
      "label": "WCH: Build & Flash (debug)",
      "type": "shell",
      "command": "python3",
      "windows": { "command": "python" },
      "args": [
        "${workspaceFolder}/tools/scripts/flash.py",
        "flash",
        "--preset",
        "debug"
      ],
      "presentation": { "reveal": "always", "panel": "shared", "clear": true },
      "problemMatcher": []
    },
    {
      "label": "WCH: Flash Only (existing .bin)",
      "type": "shell",
      "command": "python3",
      "windows": { "command": "python" },
      "args": [
        "${workspaceFolder}/tools/scripts/flash.py",
        "flash",
        "--file",
        "${workspaceFolder}/firmware/CH592F/build/release/CH592F.bin"
      ],
      "presentation": { "reveal": "always", "panel": "shared", "clear": true },
      "problemMatcher": []
    },
    {
      "label": "WCH: Flash (skip verify)",
      "type": "shell",
      "command": "python3",
      "windows": { "command": "python" },
      "args": [
        "${workspaceFolder}/tools/scripts/flash.py",
        "flash",
        "--preset",
        "release",
        "--skip-verify"
      ],
      "presentation": { "reveal": "always", "panel": "shared", "clear": true },
      "problemMatcher": []
    },
    // ── Build only ──────────────────────────────────────────────────────────
    {
      "label": "WCH: Build (release)",
      "type": "shell",
      "command": "python3",
      "windows": { "command": "python" },
      "args": [
        "${workspaceFolder}/tools/scripts/flash.py",
        "build",
        "--preset",
        "release"
      ],
      "group": "build",
      "presentation": { "reveal": "always", "panel": "shared", "clear": true },
      "problemMatcher": []
    },
    {
      "label": "WCH: Build (debug)",
      "type": "shell",
      "command": "python3",
      "windows": { "command": "python" },
      "args": [
        "${workspaceFolder}/tools/scripts/flash.py",
        "build",
        "--preset",
        "debug"
      ],
      "group": "build",
      "presentation": { "reveal": "always", "panel": "shared", "clear": true },
      "problemMatcher": []
    },
    // ── Verify ──────────────────────────────────────────────────────────────
    {
      "label": "WCH: Verify Flash",
      "type": "shell",
      "command": "python3",
      "windows": { "command": "python" },
      "args": [
        "${workspaceFolder}/tools/scripts/flash.py",
        "verify",
        "--preset",
        "release"
      ],
      "presentation": { "reveal": "always", "panel": "shared", "clear": true },
      "problemMatcher": []
    },
    // ── Chip utilities ──────────────────────────────────────────────────────
    {
      "label": "WCH: Chip Info",
      "type": "shell",
      "command": "python3",
      "windows": { "command": "python" },
      "args": ["${workspaceFolder}/tools/scripts/flash.py", "info"],
      "presentation": { "reveal": "always", "panel": "shared", "clear": true },
      "problemMatcher": []
    },
    {
      "label": "WCH: Probe Devices",
      "type": "shell",
      "command": "python3",
      "windows": { "command": "python" },
      "args": ["${workspaceFolder}/tools/scripts/flash.py", "probe"],
      "presentation": { "reveal": "always", "panel": "shared", "clear": true },
      "problemMatcher": []
    },
    {
      "label": "WCH: Erase Flash",
      "type": "shell",
      "command": "python3",
      "windows": { "command": "python" },
      "args": ["${workspaceFolder}/tools/scripts/flash.py", "erase"],
      "presentation": { "reveal": "always", "panel": "shared", "clear": true },
      "problemMatcher": []
    },
    {
      "label": "WCH: Reset Chip",
      "type": "shell",
      "command": "python3",
      "windows": { "command": "python" },
      "args": ["${workspaceFolder}/tools/scripts/flash.py", "reset"],
      "presentation": { "reveal": "always", "panel": "shared", "clear": true },
      "problemMatcher": []
    },
    // ── EEPROM ──────────────────────────────────────────────────────────────
    {
      "label": "WCH: EEPROM Dump",
      "type": "shell",
      "command": "python3",
      "windows": { "command": "python" },
      "args": [
        "${workspaceFolder}/tools/scripts/flash.py",
        "eeprom",
        "dump",
        "--out",
        "${workspaceFolder}/eeprom_dump.bin"
      ],
      "presentation": { "reveal": "always", "panel": "shared", "clear": true },
      "problemMatcher": []
    },
    {
      "label": "WCH: EEPROM Erase",
      "type": "shell",
      "command": "python3",
      "windows": { "command": "python" },
      "args": ["${workspaceFolder}/tools/scripts/flash.py", "eeprom", "erase"],
      "presentation": { "reveal": "always", "panel": "shared", "clear": true },
      "problemMatcher": []
    },
    // ── Config ──────────────────────────────────────────────────────────────
    {
      "label": "WCH: Config Info",
      "type": "shell",
      "command": "python3",
      "windows": { "command": "python" },
      "args": ["${workspaceFolder}/tools/scripts/flash.py", "config", "info"],
      "presentation": { "reveal": "always", "panel": "shared", "clear": true },
      "problemMatcher": []
    },
    {
      "label": "WCH: Config Reset (factory defaults)",
      "type": "shell",
      "command": "python3",
      "windows": { "command": "python" },
      "args": ["${workspaceFolder}/tools/scripts/flash.py", "config", "reset"],
      "presentation": { "reveal": "always", "panel": "shared", "clear": true },
      "problemMatcher": []
    },
    // ── Docs ─────────────────────────────────────────────────────────────────
    {
      "label": "Docs: Dev Server",
      "type": "shell",
      "command": "pnpm",
      "args": ["run", "dev"],
      "options": { "cwd": "${workspaceFolder}/docs" },
      "isBackground": true,
      "presentation": {
        "reveal": "always",
        "panel": "dedicated",
        "clear": true
      },
      "problemMatcher": {
        "owner": "vitepress",
        "pattern": { "regexp": "^$" },
        "background": {
          "activeOnStart": true,
          "beginsPattern": "vitepress",
          "endsPattern": "Local:"
        }
      }
    },
    // ── Studio ───────────────────────────────────────────────────────────────
    {
      "label": "Studio: pnpm install",
      "type": "shell",
      "command": "pnpm",
      "args": ["install"],
      "options": { "cwd": "${workspaceFolder}/tools/studio" },
      "presentation": { "reveal": "always", "panel": "shared", "clear": true },
      "problemMatcher": []
    },
    {
      "label": "Docs: pnpm install",
      "type": "shell",
      "command": "pnpm",
      "args": ["install"],
      "options": { "cwd": "${workspaceFolder}/docs" },
      "presentation": { "reveal": "always", "panel": "shared", "clear": true },
      "problemMatcher": []
    },
    {
      "label": "Studio: Dev Server",
      "type": "shell",
      "command": "pnpm",
      "args": ["run", "dev", "--open"],
      "options": { "cwd": "${workspaceFolder}/tools/studio" },
      "isBackground": true,
      "presentation": {
        "reveal": "always",
        "panel": "dedicated",
        "clear": true
      },
      "problemMatcher": {
        "owner": "vite",
        "pattern": { "regexp": "^$" },
        "background": {
          "activeOnStart": true,
          "beginsPattern": "VITE",
          "endsPattern": "Local:"
        }
      }
    },
    // ── Console ──────────────────────────────────────────────────────────────
    {
      "label": "BinaryKeyboard: Console",
      "type": "shell",
      "command": "python3",
      "windows": { "command": "python" },
      "args": ["${workspaceFolder}/tools/scripts/console.py", "--text"],
      "presentation": { "reveal": "always", "panel": "dedicated", "clear": true },
      "problemMatcher": []
    },
    // ── Clean ────────────────────────────────────────────────────────────────
    {
      "label": "WCH: Clean Build Cache",
      "type": "shell",
      "command": "rm -rf '${workspaceFolder}/firmware/CH592F/build'",
      "windows": {
        "command": "if exist '${workspaceFolder}\\firmware\\CH592F\\build' rmdir /s /q '${workspaceFolder}\\firmware\\CH592F\\build'"
      },
      "presentation": { "reveal": "silent", "panel": "shared", "clear": true },
      "problemMatcher": []
    }
  ]
}
```

## 附件 B：完整 `.vscode/settings.json`

```json
{
  "cmake.sourceDirectory": "${workspaceFolder}/firmware/CH592F",
  "cmake.useCMakePresets": "always",
  "VsCodeTaskButtons.showCounter": false,
  "VsCodeTaskButtons.tasks": [
    { "label": "$(package)", "tooltip": "Studio: pnpm install", "task": "Studio: pnpm install" },
    { "label": "$(cloud-download)", "tooltip": "Docs: pnpm install", "task": "Docs: pnpm install" },
    { "label": "$(book) Docs", "tooltip": "Start Docs dev server (VitePress)", "task": "Docs: Dev Server" },
    { "label": "$(browser) Studio", "tooltip": "Start Studio dev server (pnpm run dev)", "task": "Studio: Dev Server" },
    { "label": "$(wrench) Build", "tooltip": "Build release firmware", "task": "WCH: Build (release)" },
    { "label": "$(zap) Flash", "tooltip": "Build & Flash (release)", "task": "WCH: Build & Flash (release)" },
    { "label": "$(trash) Clean", "tooltip": "Clean build cache", "task": "WCH: Clean Build Cache" },
    { "label": "$(database) Erase", "tooltip": "Clear DataFlash (EEPROM)", "task": "WCH: EEPROM Erase" }
  ]
}
```
