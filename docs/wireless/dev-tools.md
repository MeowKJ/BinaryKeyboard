# 便捷开发工具

::: warning
如果你使用 `MounRiver Studio` 开发，这一页可以直接跳过。
:::

这里放两个可选小工具：

- `tools/scripts/console.py`：统一 TUI 入口
- `.vscode/tasks.json` / `.vscode/settings.json`：VS Code 任务栏按钮示例

## 怎么选

- 喜欢在终端里用方向键、回车、鼠标点选，就用 `tools/scripts/console.py`
- 使用 VS Code，就直接配一套任务按钮

## 统一 TUI

![BinaryKeyboard TUI](https://github.com/MeowKJ/BinaryKeyboard/releases/download/readme-assets/console-tui.png)

```bash
python tools/scripts/console.py
```

如果当前终端不适合 `curses`，或者 Windows Python 缺少 `_curses` 扩展，也可以：

```bash
python tools/scripts/console.py --text
```

Windows 下如果想启用完整 TUI，而不是文本回退模式：

```powershell
python -m pip install windows-curses
```

### 当前功能

- `Home`：当前目标、构建配置、常用入口
- `Target`：切换 `CH592F` / `CH552G`
- `ISP`：`probe / info / verify / erase / reset / eeprom / config`
- `Studio`：前端依赖安装和构建
- `Docs`：文档依赖安装和构建
- `Doctor`：环境检查
- `Links`：常用页面

### 当前交互

- 方向键或 `hjkl` 切换标签与动作
- `Enter` 执行当前动作
- 鼠标单击选中，双击执行
- `q` 退出
- `r` 刷新 Doctor / 缓存信息

### 当前脚本职责

- `tools/scripts/ch592f.py`：`CH592F` 键盘型号 + profile 构建
- `tools/scripts/ch552g.py`：`CH552G` keyboard 构建
- `tools/scripts/flash.py`：通用烧录 / 校验 / ISP 操作
- `tools/scripts/setup.py`：下载 `wchisp`

### 工具缓存

`console.py`、`ch592f.py`、`ch552g.py` 会把常用工具路径写进：

- `tools/scripts/.binarykeyboard_console_state.json`

当前缓存项包括：

- `cmake`
- `ninja`
- `sdcc`
- `wchisp`
- `riscv_gcc`

第一次探测完成后，后续不会每次都重新扫描磁盘。

## VS Code 任务

仓库默认不提交 `.vscode/tasks.json` / `.vscode/settings.json`，下面这份是当前脚本体系对应的参考配置。

## 附件 A：示例 `.vscode/tasks.json`

```jsonc
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "BK: TUI Console",
      "type": "shell",
      "command": "python3",
      "windows": { "command": "python" },
      "args": ["${workspaceFolder}/tools/scripts/console.py"],
      "presentation": { "reveal": "always", "panel": "dedicated", "clear": true },
      "problemMatcher": []
    },
    {
      "label": "BK: TUI Console (text)",
      "type": "shell",
      "command": "python3",
      "windows": { "command": "python" },
      "args": ["${workspaceFolder}/tools/scripts/console.py", "--text"],
      "presentation": { "reveal": "always", "panel": "dedicated", "clear": true },
      "problemMatcher": []
    },
    {
      "label": "CH592F: Build (release)",
      "type": "shell",
      "command": "python3",
      "windows": { "command": "python" },
      "args": [
        "${workspaceFolder}/tools/scripts/ch592f.py",
        "build",
        "--keyboard",
        "5KEY",
        "--profile",
        "release"
      ],
      "group": "build",
      "presentation": { "reveal": "always", "panel": "shared", "clear": true },
      "problemMatcher": []
    },
    {
      "label": "CH592F: Flash (release bin)",
      "type": "shell",
      "command": "python3",
      "windows": { "command": "python" },
      "args": [
        "${workspaceFolder}/tools/scripts/flash.py",
        "flash",
        "--file",
        "${workspaceFolder}/firmware/CH592F/build/release-5key/CH592F-5KEY-<version>.bin"
      ],
      "presentation": { "reveal": "always", "panel": "shared", "clear": true },
      "problemMatcher": []
    },
    {
      "label": "CH552G: Build (BASIC)",
      "type": "shell",
      "command": "python3",
      "windows": { "command": "python" },
      "args": [
        "${workspaceFolder}/tools/scripts/ch552g.py",
        "build",
        "--keyboard",
        "BASIC"
      ],
      "group": "build",
      "presentation": { "reveal": "always", "panel": "shared", "clear": true },
      "problemMatcher": []
    },
    {
      "label": "CH552G: Flash (BASIC bin)",
      "type": "shell",
      "command": "python3",
      "windows": { "command": "python" },
      "args": [
        "${workspaceFolder}/tools/scripts/flash.py",
        "flash",
        "--file",
        "${workspaceFolder}/firmware/CH552G/build/basic/CH552G-BASIC-<version>.bin"
      ],
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
      "label": "WCH: Chip Info",
      "type": "shell",
      "command": "python3",
      "windows": { "command": "python" },
      "args": ["${workspaceFolder}/tools/scripts/flash.py", "info"],
      "presentation": { "reveal": "always", "panel": "shared", "clear": true },
      "problemMatcher": []
    },
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
      "label": "Studio: Build",
      "type": "shell",
      "command": "pnpm",
      "args": ["run", "build"],
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
      "label": "Docs: Build",
      "type": "shell",
      "command": "pnpm",
      "args": ["run", "build"],
      "options": { "cwd": "${workspaceFolder}/docs" },
      "presentation": { "reveal": "always", "panel": "shared", "clear": true },
      "problemMatcher": []
    }
  ]
}
```

## 附件 B：示例 `.vscode/settings.json`

```json
{
  "cmake.sourceDirectory": "${workspaceFolder}/firmware/CH592F",
  "cmake.useCMakePresets": "always",
  "VsCodeTaskButtons.showCounter": false,
  "VsCodeTaskButtons.tasks": [
    { "label": "$(terminal) TUI", "tooltip": "BinaryKeyboard TUI", "task": "BK: TUI Console" },
    { "label": "$(tools) 592", "tooltip": "Build CH592F release", "task": "CH592F: Build (release)" },
    { "label": "$(zap) 592", "tooltip": "Flash CH592F release bin", "task": "CH592F: Flash (release bin)" },
    { "label": "$(tools) 552", "tooltip": "Build CH552G BASIC", "task": "CH552G: Build (BASIC)" },
    { "label": "$(zap) 552", "tooltip": "Flash CH552G BASIC bin", "task": "CH552G: Flash (BASIC bin)" },
    { "label": "$(search) ISP", "tooltip": "Probe WCH ISP devices", "task": "WCH: Probe Devices" },
    { "label": "$(browser) Studio", "tooltip": "Build Studio", "task": "Studio: Build" },
    { "label": "$(book) Docs", "tooltip": "Build Docs", "task": "Docs: Build" }
  ]
}
```
