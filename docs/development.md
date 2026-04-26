# 开发总览

开发文档放在使用文档之后。建议先确认目标芯片和键盘型号，再进入对应固件目录。

## 开发入口

| 目标 | 文档 | 主要内容 |
| :--- | :--- | :--- |
| 经典版 CH552G | [经典版固件开发](./wired/dev.md) | SDCC / CMake、USB HID、EEPROM、FUNC、RGB、旋钮 |
| 无线版 CH592F | [无线版固件开发](./wireless/dev.md) | RISC-V 工具链、USB/BLE 双模、DataFlash、FN、宏、IAP |
| 统一控制台 | [无线版便捷开发工具](./wireless/dev-tools.md) | `console.py`、工具链缓存、构建与烧录菜单 |
| Studio / 协议 | [无线版 HID 通讯协议](./wireless/hid.md) | WebHID 帧格式、命令、状态与配置同步 |
| 宏存储 | [MeowFS](./meowfs.md) | 有线 / 无线共用宏存储格式 |
| 宏语言 | [MeowMacro](./meowmacro.md) | Studio 宏编辑语言与示例 |

## 推荐工作流

1. 从仓库根目录运行统一控制台：

```bash
./run.sh
```

2. 在控制台中选择 target：

- `CH552G`：经典版 BASIC / 5KEY / KNOB。
- `CH592F`：无线版 5KEY / KNOB。

3. 选择 keyboard 和 profile。
4. 执行 build。
5. 使用对应刷写流程验证。
6. 用 Studio 连接设备，确认设备信息、键位读写、层切换、RGB 和宏功能。

## 目标与产物

### CH552G

| keyboard | 产物示例 | 用途 |
| :--- | :--- | :--- |
| BASIC | `CH552G-BASIC-<version>.hex` | 经典基础款 |
| 5KEY | `CH552G-5KEY-<version>.hex` | 经典五键款 |
| KNOB | `CH552G-KNOB-<version>.hex` | 经典旋钮款 |

### CH592F

| 文件 | 用途 |
| :--- | :--- |
| `CH592F-<MODEL>-<version>-full.hex` | 首刷 / 救砖恢复 |
| `CH592F-<MODEL>-<version>-app.bin` | Studio 在线更新 |
| `CH592F-<MODEL>-<version>-iap.hex` | 高地址 IAP 单独产物 |

## 关键目录

```text
firmware/
├── CH552G/      # 经典版固件
├── CH592F/      # 无线版固件
└── cmake/       # 两个固件共用的 CMake helper

tools/
├── scripts/     # console.py、构建脚本、刷写脚本
├── meowisp/     # wchisp 封装与分发
└── studio/      # BinaryKeyboard Studio 前端
```

## 改动建议

- 默认键位：先看 `kbd_storage.c` 或 `KeysDataHandler.c`。
- 键位布局：先看固件型号宏，再看 Studio 的 `layouts.ts`。
- 通讯协议：先改固件命令处理，再同步 Studio codec。
- 宏：优先保持 MeowFS / MeowMacro 兼容，不要为单个芯片单独造格式。
- 发布产物：保持 `config/versions.json`、Release asset 命名和 Studio 更新逻辑一致。

## 验证清单

每次改固件至少验证：

- 目标型号能成功编译。
- 首刷或普通刷写能完成。
- Studio 能连接并读到正确型号。
- 默认层与键位数量正确。
- 键位写入后断电重启仍保留。
- 如果改到无线版，USB / BLE 两种模式都能输入。
- 如果改到宏或存储，旧配置异常时能恢复默认配置。
