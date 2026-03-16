#!/usr/bin/env python3
"""Bilingual (en/zh) string table for BinaryKeyboard Console."""

from __future__ import annotations

import locale
import os

_EN: dict[str, str] = {
        # ── General UI ────────────────────────────────────────────────────
        "title": "BinaryKeyboard Console",
        "title_short": "BK Console",
        "press_enter": "\nPress Enter to return...",
        "back": "Back",
        "quit": "Quit",
        "ready": "Ready",
        "not_built": "Not built",
        "missing": "missing",
        "welcome": "Welcome to BinaryKeyboard Console",

        # ── Tab names ─────────────────────────────────────────────────────
        "tab.home": "Home",
        "tab.target": "Target",
        "tab.isp": "ISP",
        "tab.dev": "Dev",
        "tab.info": "Info",
        "tab.studio": "Studio",
        "tab.docs": "Docs",
        "tab.doctor": "Doctor",
        "tab.links": "Links",

        # ── Parameter bar ─────────────────────────────────────────────────
        "bar.firmware": "Firmware",

        # ── Status bar ────────────────────────────────────────────────────
        "quit_hint": "Press Ctrl+C again to quit",
        "status.compact": "{tab} | \u2190\u2192 tab  \u2191\u2193 select  Enter run  Ctrl+C×2 quit",
        "status.full": "{tab}  \u2502  \u2190\u2192 switch tab  \u2191\u2193 select  Enter run  click  1-9 quick  r refresh  Ctrl+C×2 quit",

        # ── Home content ──────────────────────────────────────────────────
        "home.build_config": "Build config",
        "home.artifact": "Firmware",
        "home.build_dir": "Build dir",

        # ── Studio tab ────────────────────────────────────────────────────
        "path": "Path",
        "dev.tools": "Frontend tools",
        "dev.dependencies": "Dependencies",
        "dev.build_output": "Build output",
        "dev.not_installed": "Not installed",
        "studio.stack": "Stack: Vue 3 + Vite + PrimeVue",
        "studio.targets": "Targets: CH592F / CH552G",

        # ── Docs tab ─────────────────────────────────────────────────────
        "docs.stack": "Stack: VitePress",

        # ── Target details ────────────────────────────────────────────────
        "detail.target": "Target",
        "detail.build_dir": "Build dir",
        "detail.compiler": "Compiler",
        "detail.keyboard": "Keyboard",
        "detail.profile": "Profile",
        "detail.toolchain": "Toolchain",
        "detail.toolchain_source": "Toolchain source",
        "detail.preset_family": "Preset family: shared release-* / debug-*",
        "detail.ch552_keymap": "Keymap: multi-layer remap + FUNC-hold layer switching",
        "detail.ch552_rgb": "RGB: key RGB with layer flash feedback (no indicator RGB)",

        # ── Action labels ─────────────────────────────────────────────────
        "action.toggle_target": "Toggle target  [{target}]",
        "action.cycle_keyboard": "Cycle keyboard  [{keyboard}]",
        "action.toggle_profile": "Toggle profile  [{profile}]",
        "action.configure_toolchain": "Configure toolchain",
        "action.build": "Build selected target",
        "action.clean": "Clean build directory",
        "action.flash": "Flash selected target",
        "action.show_commands": "Show build commands",
        "action.generate_ide": "Generate IDE config",
        "action.install_wchisp": "Install or update wchisp",
        "action.probe": "Probe ISP devices",
        "action.check_tools": "Check tools",
        "action.show_isp_sheet": "Show ISP command sheet",
        "action.probe_devices": "Probe devices",
        "action.chip_commands": "Chip commands...",
        "action.eeprom_commands": "EEPROM commands...",
        "action.clear_dataflash": "Clear dataflash",
        "action.config_commands": "Config commands...",
        "action.install_deps": "Install dependencies",
        "action.dev_server": "Dev server",
        "action.studio_install": "Install (Studio)",
        "action.studio_dev": "Dev server (Studio)",
        "action.build_studio": "Build studio",
        "action.docs_install": "Install (Docs)",
        "action.docs_dev": "Dev server (Docs)",
        "action.build_docs": "Build docs",
        "action.show_doctor": "Show doctor report",

        # ── Action hints ──────────────────────────────────────────────────
        "hint.toggle_target": "Switch between CH592F and CH552G workflows.",
        "hint.cycle_keyboard_552": "Cycle BASIC / KNOB / 5KEY.",
        "hint.cycle_keyboard_592": "Switch between 5KEY and KNOB.",
        "hint.toggle_profile": "Switch between release and debug.",
        "hint.configure_toolchain": "Cache MRS_TOOLCHAIN_ROOT for the shared CH592 presets.",
        "hint.build_552": "Run tools/scripts/ch552g.py build.",
        "hint.build_592": "Run tools/scripts/ch592f.py build.",
        "hint.clean": "Remove the build output directory.",
        "hint.flash_552": "Build, then flash the CH552G firmware.",
        "hint.flash_592": "Build, then flash the CH592F firmware.",
        "hint.show_commands": "Print the resolved build / flash / verify commands.",
        "hint.generate_ide": "Write VSCode C/C++ settings and a root compile_commands.json.",
        "hint.install_wchisp": "Run tools/scripts/setup.py.",
        "hint.probe": "List connected WCH ISP devices.",
        "hint.check_tools": "Show cmake/ninja/wchisp/toolchain detection.",
        "hint.show_isp_sheet": "Print all flash.py ISP commands and examples.",
        "hint.probe_devices": "Run flash.py probe.",
        "hint.chip_commands": "Choose info / verify / erase / reset.",
        "hint.eeprom_commands": "Choose dump / write / erase.",
        "hint.clear_dataflash": "Run flash.py eeprom erase.",
        "hint.config_commands": "Choose config info / reset.",
        "hint.install_studio": "Run pnpm install in tools/studio.",
        "hint.dev_studio": "Run pnpm run dev in tools/studio.",
        "hint.build_studio": "Run pnpm run build in tools/studio.",
        "hint.install_docs": "Run pnpm install in docs.",
        "hint.dev_docs": "Run pnpm run dev in docs.",
        "hint.build_docs": "Run pnpm run build in docs.",
        "hint.show_doctor": "Rerun all health checks.",

        # ── Toolchain config ──────────────────────────────────────────────
        "toolchain.saved": "Saved toolchain root.",
        "toolchain.shared_presets": "Shared CH592 presets will now use release-* / debug-* directly.",
        "toolchain.found": "Found compiler: {name}",
        "toolchain.no_gcc": "No riscv-*-elf-gcc found under this path. Not saved.",
        "toolchain.path_missing": "Path does not exist on disk. Not saved.",
        "toolchain.empty": "Toolchain root is empty, aborted.",
        "toolchain.reset": "Cached toolchain root cleared. Will re-detect from env/PATH.",
        "toolchain.reset_no_cache": "No cached toolchain root to clear.",
        "action.reset_toolchain": "Reset toolchain path",
        "hint.reset_toolchain": "Clear cached toolchain root and re-detect from environment.",

        # ── Build result ──────────────────────────────────────────────────
        "build.complete": "Build complete!",
        "build.failed": "Build failed (exit {rc})",
        "clean.done": "Cleaned: {path}",
        "clean.nothing": "Build directory does not exist.",

        # ── ISP reference panel ───────────────────────────────────────────
        "isp.target": "target",
        "isp.wchisp": "wchisp",
        "isp.flash_wrapper": "flash wrapper",
        "isp.default_image": "default image",
        "isp.transport_options": "Transport options:",
        "isp.device_help": "  -d/--device N   choose USB device index",
        "isp.serial_help": "  -s/--serial     use serial transport",
        "isp.port_help": "  --port PORT     serial port path",
        "isp.commands_header": "ISP commands:",
        "isp.cmd.info": "  info                chip info / UID / bootloader",
        "isp.cmd.probe": "  probe               list connected ISP devices",
        "isp.cmd.flash": "  flash               program a prepared firmware image",
        "isp.cmd.verify": "  verify              verify chip flash against image",
        "isp.cmd.erase": "  erase               erase code flash",
        "isp.cmd.reset": "  reset               reset target chip",
        "isp.cmd.eeprom_dump": "  eeprom dump         dump EEPROM to file",
        "isp.cmd.eeprom_erase": "  eeprom erase        erase data EEPROM",
        "isp.cmd.dataflash_erase": "  dataflash clear     erase dataflash (EEPROM)",
        "isp.cmd.eeprom_write": "  eeprom write        write file into EEPROM",
        "isp.cmd.config_info": "  config info         read config registers",
        "isp.cmd.config_reset": "  config reset        reset config registers",

        # ── Prompts ───────────────────────────────────────────────────────
        "prompt.chip_command": "Chip command [probe/info/verify/erase/reset]",
        "prompt.eeprom_command": "EEPROM command [dump/write/erase]",
        "prompt.config_command": "Config command [info/reset]",
        "prompt.eeprom_dump_file": "EEPROM dump output file",
        "prompt.eeprom_input_file": "EEPROM input file",
        "prompt.file_empty": "File path is empty, aborted.",
        "prompt.unknown_command": "Unknown command: {cmd}",

        # ── Show commands ─────────────────────────────────────────────────
        "cmd.target": "Target",
        "cmd.build_config": "Build config",
        "cmd.artifact": "Output",
        "cmd.build_label": "Build:",
        "cmd.flash_label": "Flash:",
        "cmd.verify_label": "Verify:",

        # ── ISP command sheet ─────────────────────────────────────────────
        "isp_sheet.title": "ISP command sheet",
        "isp_sheet.build_first": "Build first:",
        "isp_sheet.isp": "ISP:",
        "isp_sheet.transport_flags": "Optional transport flags:",

        # ── IDE config ────────────────────────────────────────────────────
        "ide.target": "Target",
        "ide.build_config": "Build config",
        "ide.build_dir": "Selected build dir",
        "ide.dbs_merged": "Compile databases merged",
        "ide.entries": "Entries written",
        "ide.wrote": "[OK] Wrote {path}",
        "ide.clion_support": "[OK] CLion/clangd support written via root compile_commands.json",
        "ide.warn_no_db": "[WARN] Selected build has no compile_commands.json yet; using fallback include paths where needed.",
        "ide.warn_no_includes": "[WARN] Selected build compile_commands.json was found, but include flags could not be parsed.",
        "ide.clion_note": "CLion note: open the firmware CMake project directly, or point it at compile_commands.json in the repo root.",

        # ── Doctor ────────────────────────────────────────────────────────
        "doctor.dirty": "dirty",
        "doctor.clean": "clean",
        "doctor.git_worktree": "git worktree",
        "doctor.sec_build": "── Build Tools ──",
        "doctor.sec_flash": "── Flash Tools ──",
        "doctor.sec_ch552": "── CH552 Compiler ──",
        "doctor.sec_ch592": "── CH592 Toolchain ──",
        "doctor.sec_frontend": "── Frontend / VCS ──",

        # ── Open URL ──────────────────────────────────────────────────────
        "url.opened": "Opened {name}: {url}",
        "url.failed": "Failed to open browser automatically.",

        # ── Link labels ───────────────────────────────────────────────────
        "link.mrs_download": "Open MRS download",
        "link.wchisp_releases": "Open wchisp releases",
        "link.wch_homepage": "Open WCH homepage",
        "link.project_repo": "Open project repo",
        "link.mrs_hint": "MounRiver Studio download page.",
        "link.wchisp_hint": "wchisp release page.",
        "link.wch_hint": "Official WCH site.",
        "link.repo_hint": "BinaryKeyboard repository.",

        # ── Terminal launch ───────────────────────────────────────────────
        "launched_terminal": "Launched in new terminal: {cmd}",
        "launch_terminal_failed": "Failed to launch terminal for {cmd}: {err}",

        # ── Language ──────────────────────────────────────────────────────
        "action.toggle_lang": "Switch language  [{lang}]",
        "hint.toggle_lang": "Toggle between English and 简体中文.",
        "lang.en": "English",
        "lang.zh": "简体中文",
}

_ZH: dict[str, str] = {
        # ── General UI ────────────────────────────────────────────────────
        "title": "BinaryKeyboard 控制台",
        "title_short": "BK 控制台",
        "press_enter": "\n按 Enter 返回...",
        "back": "返回",
        "quit": "退出",
        "ready": "就绪",
        "not_built": "未编译",
        "missing": "缺失",
        "welcome": "欢迎使用 BinaryKeyboard 控制台",

        # ── Tab names ─────────────────────────────────────────────────────
        "tab.home": "主页",
        "tab.target": "目标",
        "tab.isp": "ISP",
        "tab.dev": "开发",
        "tab.info": "信息",
        "tab.studio": "Studio",
        "tab.docs": "文档",
        "tab.doctor": "诊断",
        "tab.links": "链接",

        # ── Parameter bar ─────────────────────────────────────────────────
        "bar.firmware": "固件",

        # ── Status bar ────────────────────────────────────────────────────
        "quit_hint": "再按一次 Ctrl+C 退出",
        "status.compact": "{tab} | ←→ 切换  ↑↓ 选择  Enter 执行  Ctrl+C×2 退出",
        "status.full": "{tab}  │  ←→ 切换标签  ↑↓ 选择  Enter 执行  点击  1-9 快捷  r 刷新  Ctrl+C×2 退出",

        # ── Home content ──────────────────────────────────────────────────
        "home.build_config": "编译配置",
        "home.artifact": "固件",
        "home.build_dir": "编译目录",

        # ── Studio tab ────────────────────────────────────────────────────
        "path": "路径",
        "dev.tools": "前端工具",
        "dev.dependencies": "依赖",
        "dev.build_output": "构建产物",
        "dev.not_installed": "未安装",
        "studio.stack": "技术栈: Vue 3 + Vite + PrimeVue",
        "studio.targets": "目标: CH592F / CH552G",

        # ── Docs tab ─────────────────────────────────────────────────────
        "docs.stack": "技术栈: VitePress",

        # ── Target details ────────────────────────────────────────────────
        "detail.target": "目标",
        "detail.build_dir": "编译目录",
        "detail.compiler": "编译器",
        "detail.keyboard": "键盘",
        "detail.profile": "方案",
        "detail.toolchain": "工具链",
        "detail.toolchain_source": "工具链来源",
        "detail.preset_family": "预设族: 共享 release-* / debug-*",
        "detail.ch552_keymap": "键映射: 多层 remap + FUNC 保持层切换",
        "detail.ch552_rgb": "RGB: 按键 RGB + 层切换反馈（无指示灯 RGB）",

        # ── Action labels ─────────────────────────────────────────────────
        "action.toggle_target": "切换目标  [{target}]",
        "action.cycle_keyboard": "切换键盘  [{keyboard}]",
        "action.toggle_profile": "切换方案  [{profile}]",
        "action.configure_toolchain": "配置工具链",
        "action.build": "编译当前目标",
        "action.clean": "清除编译目录",
        "action.flash": "烧录当前目标",
        "action.show_commands": "显示编译命令",
        "action.generate_ide": "生成 IDE 配置",
        "action.install_wchisp": "安装或更新 wchisp",
        "action.probe": "探测 ISP 设备",
        "action.check_tools": "检查工具",
        "action.show_isp_sheet": "显示 ISP 命令表",
        "action.probe_devices": "探测设备",
        "action.chip_commands": "芯片命令...",
        "action.eeprom_commands": "EEPROM 命令...",
        "action.clear_dataflash": "清除 dataflash",
        "action.config_commands": "配置命令...",
        "action.install_deps": "安装依赖",
        "action.dev_server": "开发服务器",
        "action.studio_install": "安装 (Studio)",
        "action.studio_dev": "开发服务器 (Studio)",
        "action.build_studio": "编译 Studio",
        "action.docs_install": "安装 (Docs)",
        "action.docs_dev": "开发服务器 (Docs)",
        "action.build_docs": "编译文档",
        "action.show_doctor": "显示诊断报告",

        # ── Action hints ──────────────────────────────────────────────────
        "hint.toggle_target": "在 CH592F 和 CH552G 之间切换。",
        "hint.cycle_keyboard_552": "循环切换 BASIC / KNOB / 5KEY。",
        "hint.cycle_keyboard_592": "在 5KEY 和 KNOB 之间切换。",
        "hint.toggle_profile": "在 release 和 debug 之间切换。",
        "hint.configure_toolchain": "缓存 MRS_TOOLCHAIN_ROOT 供共享预设使用。",
        "hint.build_552": "运行 tools/scripts/ch552g.py build。",
        "hint.build_592": "运行 tools/scripts/ch592f.py build。",
        "hint.clean": "删除编译输出目录。",
        "hint.flash_552": "先编译，再烧录 CH552G 固件。",
        "hint.flash_592": "先编译，再烧录 CH592F 固件。",
        "hint.show_commands": "打印解析后的 build / flash / verify 命令。",
        "hint.generate_ide": "写入 VSCode C/C++ 设置和根目录 compile_commands.json。",
        "hint.install_wchisp": "运行 tools/scripts/setup.py。",
        "hint.probe": "列出已连接的 WCH ISP 设备。",
        "hint.check_tools": "显示 cmake/ninja/wchisp/工具链检测结果。",
        "hint.show_isp_sheet": "打印所有 flash.py ISP 命令和示例。",
        "hint.probe_devices": "运行 flash.py probe。",
        "hint.chip_commands": "选择 info / verify / erase / reset。",
        "hint.eeprom_commands": "选择 dump / write / erase。",
        "hint.clear_dataflash": "运行 flash.py eeprom erase。",
        "hint.config_commands": "选择 config info / reset。",
        "hint.install_studio": "在 tools/studio 中运行 pnpm install。",
        "hint.dev_studio": "在 tools/studio 中运行 pnpm run dev。",
        "hint.build_studio": "在 tools/studio 中运行 pnpm run build。",
        "hint.install_docs": "在 docs 中运行 pnpm install。",
        "hint.dev_docs": "在 docs 中运行 pnpm run dev。",
        "hint.build_docs": "在 docs 中运行 pnpm run build。",
        "hint.show_doctor": "重新运行所有健康检查。",

        # ── Toolchain config ──────────────────────────────────────────────
        "toolchain.saved": "已保存工具链路径。",
        "toolchain.shared_presets": "共享 CH592 预设将直接使用 release-* / debug-*。",
        "toolchain.found": "找到编译器: {name}",
        "toolchain.no_gcc": "此路径下未找到 riscv-*-elf-gcc，未保存。",
        "toolchain.path_missing": "路径不存在，未保存。",
        "toolchain.empty": "工具链路径为空，已取消。",
        "toolchain.reset": "已清除缓存的工具链路径，将从环境变量/PATH 重新检测。",
        "toolchain.reset_no_cache": "没有缓存的工具链路径可清除。",
        "action.reset_toolchain": "重置工具链路径",
        "hint.reset_toolchain": "清除缓存的工具链路径，从环境变量重新检测。",

        # ── Build result ──────────────────────────────────────────────────
        "build.complete": "编译完成！",
        "build.failed": "编译失败 (退出码 {rc})",
        "clean.done": "已清除: {path}",
        "clean.nothing": "编译目录不存在。",

        # ── ISP reference panel ───────────────────────────────────────────
        "isp.target": "目标",
        "isp.wchisp": "wchisp",
        "isp.flash_wrapper": "flash 封装",
        "isp.default_image": "默认镜像",
        "isp.transport_options": "传输参数:",
        "isp.device_help": "  -d/--device N   选择 USB 设备序号",
        "isp.serial_help": "  -s/--serial     使用串口传输",
        "isp.port_help": "  --port PORT     串口路径",
        "isp.commands_header": "ISP 命令:",
        "isp.cmd.info": "  info                查看芯片信息 / UID / bootloader",
        "isp.cmd.probe": "  probe               列出已连接 ISP 设备",
        "isp.cmd.flash": "  flash               烧录准备好的固件镜像",
        "isp.cmd.verify": "  verify              校验芯片 flash 与镜像一致",
        "isp.cmd.erase": "  erase               擦除代码区 flash",
        "isp.cmd.reset": "  reset               复位目标芯片",
        "isp.cmd.eeprom_dump": "  eeprom dump         导出 EEPROM 到文件",
        "isp.cmd.eeprom_erase": "  eeprom erase        擦除数据 EEPROM",
        "isp.cmd.dataflash_erase": "  dataflash clear     清除 dataflash（EEPROM）",
        "isp.cmd.eeprom_write": "  eeprom write        将文件写入 EEPROM",
        "isp.cmd.config_info": "  config info         读取配置寄存器",
        "isp.cmd.config_reset": "  config reset        重置配置寄存器",

        # ── Prompts ───────────────────────────────────────────────────────
        "prompt.chip_command": "芯片命令 [probe/info/verify/erase/reset]",
        "prompt.eeprom_command": "EEPROM 命令 [dump/write/erase]",
        "prompt.config_command": "配置命令 [info/reset]",
        "prompt.eeprom_dump_file": "EEPROM 转储输出文件",
        "prompt.eeprom_input_file": "EEPROM 输入文件",
        "prompt.file_empty": "文件路径为空，已取消。",
        "prompt.unknown_command": "未知命令: {cmd}",

        # ── Show commands ─────────────────────────────────────────────────
        "cmd.target": "目标",
        "cmd.build_config": "编译配置",
        "cmd.artifact": "产物",
        "cmd.build_label": "编译:",
        "cmd.flash_label": "烧录:",
        "cmd.verify_label": "校验:",

        # ── ISP command sheet ─────────────────────────────────────────────
        "isp_sheet.title": "ISP 命令表",
        "isp_sheet.build_first": "先编译:",
        "isp_sheet.isp": "ISP:",
        "isp_sheet.transport_flags": "可选传输参数:",

        # ── IDE config ────────────────────────────────────────────────────
        "ide.target": "目标",
        "ide.build_config": "编译配置",
        "ide.build_dir": "选中的编译目录",
        "ide.dbs_merged": "编译数据库已合并",
        "ide.entries": "已写入条目",
        "ide.wrote": "[OK] 已写入 {path}",
        "ide.clion_support": "[OK] 已通过根目录 compile_commands.json 写入 CLion/clangd 支持",
        "ide.warn_no_db": "[WARN] 选中的编译目录没有 compile_commands.json；使用回退 include 路径。",
        "ide.warn_no_includes": "[WARN] 找到了 compile_commands.json，但无法解析 include 标志。",
        "ide.clion_note": "CLion 提示: 直接打开 firmware CMake 项目，或指向仓库根目录的 compile_commands.json。",

        # ── Doctor ────────────────────────────────────────────────────────
        "doctor.dirty": "有修改",
        "doctor.clean": "干净",
        "doctor.git_worktree": "git 工作区",
        "doctor.sec_build": "── 构建工具 ──",
        "doctor.sec_flash": "── 烧录工具 ──",
        "doctor.sec_ch552": "── CH552 编译器 ──",
        "doctor.sec_ch592": "── CH592 工具链 ──",
        "doctor.sec_frontend": "── 前端 / 版本控制 ──",

        # ── Open URL ──────────────────────────────────────────────────────
        "url.opened": "已打开 {name}: {url}",
        "url.failed": "无法自动打开浏览器。",

        # ── Link labels ───────────────────────────────────────────────────
        "link.mrs_download": "打开 MRS 下载页",
        "link.wchisp_releases": "打开 wchisp 发布页",
        "link.wch_homepage": "打开 WCH 官网",
        "link.project_repo": "打开项目仓库",
        "link.mrs_hint": "MounRiver Studio 下载页面。",
        "link.wchisp_hint": "wchisp 发布页面。",
        "link.wch_hint": "WCH 官方网站。",
        "link.repo_hint": "BinaryKeyboard 项目仓库。",

        # ── Terminal launch ───────────────────────────────────────────────
        "launched_terminal": "已在新终端启动: {cmd}",
        "launch_terminal_failed": "启动终端失败: {cmd} ({err})",

        # ── Language ──────────────────────────────────────────────────────
        "action.toggle_lang": "切换语言  [{lang}]",
        "hint.toggle_lang": "在 English 和简体中文之间切换。",
        "lang.en": "English",
        "lang.zh": "简体中文",
}

# ISP reference lines are identical in both languages (technical)
for _k in list(_EN):
    if _k.startswith("isp.") and _k not in _ZH:
        _ZH[_k] = _EN[_k]

# ── Language state ────────────────────────────────────────────────────────────

_TABLES = {"en": _EN, "zh": _ZH}
_current_lang: str = "en"


def _detect_lang() -> str:
    env_lang = os.environ.get("BK_LANG", "").strip().lower()
    if env_lang in _TABLES:
        return env_lang
    try:
        loc = locale.getdefaultlocale()[0] or ""
    except Exception:
        loc = ""
    return "zh" if loc.startswith("zh") else "en"


_current_lang = _detect_lang()


def get_lang() -> str:
    return _current_lang


def set_lang(lang: str) -> None:
    global _current_lang
    if lang in _TABLES:
        _current_lang = lang


def toggle_lang() -> str:
    global _current_lang
    _current_lang = "zh" if _current_lang == "en" else "en"
    return _current_lang


def t(key: str, **kwargs: object) -> str:
    """Look up a string by key in the active language, with optional format parameters."""
    table = _TABLES.get(_current_lang, _EN)
    text = table.get(key) or _EN.get(key, key)
    return text.format(**kwargs) if kwargs else text
