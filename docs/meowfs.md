# 💾 MeowFS 宏存储系统

MeowFS 是 BinaryKeyboard 当前统一使用的宏存储格式，用来替代固定槽位的大块宏区。

它现在同时覆盖：

- `CH552G` 经典版：`1 KB` 宏区，页擦除粒度 `64 B`
- `CH592F` 无线版：`8 KB` 宏区，页擦除粒度 `256 B`
- `Studio`：统一按动态宏列表管理，不再假设固定槽位

## 设计目标

- 让宏区从“固定 2/8 槽”改成“顺序存放的可变长条目”
- 删除时不立即整区擦除，只做标记删除
- 把压缩整理放到 `Studio` 侧做，减少固件复杂度
- 用同一套格式兼容 `CH552G` 和 `CH592F`

## 条目格式

每个宏条目格式如下：

```text
+--------+--------+--------------------+
| marker | count  | actions[count * 2] |
+--------+--------+--------------------+
```

- `marker = 0xAA`：有效宏
- `marker = 0x00`：已删除宏
- `marker = 0xFF`：空白区域，扫描到这里结束
- `count`：动作数，单动作固定 `2` 字节

动作本体沿用原有 `MacroActionType + param` 结构，没有再引入第二套 DSL 存储格式。

## 为什么不做 256B 对齐

不需要。

`CH592F` 的页擦除粒度虽然是 `256 B`，但 MeowFS 的删除不是“擦整页”，而是把条目头的 `0xAA` 写成 `0x00`。因此：

- 宏条目本身可以紧密顺序排布
- 不需要给每个宏补齐到 `256 B`
- 真正需要整页擦除时，只发生在 Studio 做压缩重写的时候

这比“每个宏固定对齐到擦除页”更省空间，特别适合大量小宏。

## 当前 Studio 行为

Studio 现在对 MeowFS 设备统一使用动态列表模型：

- 宏面板展示“已有宏 + 末尾一个新建入口”
- 保存到末尾时会追加新条目
- 覆盖已有宏时，Studio 会读取全部有效宏并整区重写
- 删除宏时，固件仅做删除标记；Studio 会刷新列表
- 删除中间宏后，Studio 会同步重排 `keymap` / `FN` 里的宏索引引用，避免绑定漂移

当前限制：

- MeowFS 设备侧未保留旧的 `24 B` 名称头，`Studio` 对 CH552/CH592 动态宏列表不再提供名称编辑
- 宏索引仍然是运行时索引，不是稳定 UUID

## 协议现状

### CH552G

`CH552G` 使用单命令 `0x40`，子命令如下：

- `INFO`
- `READ`
- `ERASE`
- `WRITE`
- `DELETE`

### CH592F

`CH592F` 仍沿用原有 `0x40 ~ 0x43` 命令族，但语义已经切到 MeowFS：

- `0x40 MACRO_INFO`：返回文件系统总量、页大小、宏数量、剩余空间
- `0x41 MACRO_GET`：按偏移读取原始 MeowFS 数据
- `0x42 MACRO_SET`
  - `sub=0`：擦页 / 全擦
  - `sub=1`：按偏移写原始数据
- `0x43 MACRO_DEL`：按宏索引标记删除

这样可以最小化固件改动，同时让 Studio 侧和 CH552 的行为保持一致。

## 本次编译与测试结果

验证日期：`2026-03-18`

### Studio

| 项目 | 命令 | 结果 |
| :-- | :-- | :-- |
| Type check | `pnpm --dir tools/studio run type-check` | 通过 |
| Unit tests | `pnpm --dir tools/studio test` | `1` 个测试文件、`47` 个测试全部通过 |
| Production build | `pnpm --dir tools/studio run build-only` | 通过 |

`Studio build` 有两个非阻塞提示：

- 主 bundle 约 `642 KB`，超过 Vite 默认 `500 KB` 警告阈值
- 构建日志里仍有一条 `manualChunks` 配置警告，但不影响产物生成

### CH552G

| 变体 | 命令 | 结果 | Flash | 备注 |
| :-- | :-- | :-- | :-- | :-- |
| BASIC | `python tools/scripts/ch552g.py build -k BASIC` | 通过 | `12904 / 13312 B` | 剩余 `408 B` |
| 5KEY | `python tools/scripts/ch552g.py build -k 5KEY` | 通过 | `12919 / 13312 B` | 剩余 `393 B` |
| KNOB | `python tools/scripts/ch552g.py build -k KNOB` | 通过 | `13168 / 13312 B` | 剩余 `144 B` |

`KNOB` 仍然非常紧，构建报告里还有：

- `internal RAM full`
- 栈余量约 `139 B`

这不是本次改动新引入的问题，但说明 `KNOB` 版本后续继续加功能时需要非常克制。

### CH592F

| 变体 | 命令 | 结果 | Flash | RAM |
| :-- | :-- | :-- | :-- | :-- |
| 5KEY release | `python tools/scripts/ch592f.py build -k 5KEY --profile release` | 通过 | `173772 / 458752 B` | `23248 / 26624 B` |
| KNOB release | `python tools/scripts/ch592f.py build -k KNOB --profile release` | 通过 | `173704 / 458752 B` | `23136 / 26624 B` |

## 自动验证范围

本次自动验证已经覆盖：

- TypeScript 类型检查
- `Studio` 单元测试
- `Studio` 生产构建
- `CH552G` 全变体固件构建
- `CH592F` 当前主用 release 变体构建

## 仍需上板手测

下面这些只能在真机上确认：

1. `CH552G` 连接 Studio 后创建多个宏，确认列表与容量显示正确
2. `CH552G` 删除中间宏，确认按键绑定被正确重排
3. `CH592F` 创建、删除、重写大宏，确认 `8 KB` 宏区行为正常
4. `CH592F` 删除中间宏后，宏执行索引与 Studio 列表一致
5. `ONCE / HOLD_ABORT / HOLD_FINISH / TOGGLE` 四种触发模式在两端都能正常执行
6. 断电重连后重新读取宏列表，确认没有扫描异常或脏数据

## 相关文件

- `tools/studio/src/stores/macroStore.ts`
- `tools/studio/src/services/hid/devices/ch552/codec.ts`
- `tools/studio/src/services/hid/devices/ch592/codec.ts`
- `firmware/CH552G/keyboard/MacroStorage.c`
- `firmware/CH592F/keyboard/src/kbd_storage.c`
- `firmware/CH592F/keyboard/src/kbd_command.c`

## 结论

当前代码已经完成：

- CH552G MeowFS 固件
- CH592F MeowFS 固件
- Studio 动态多宏管理
- 删除后的索引重排保护

从自动化验证结果看，现阶段已经可以进入真机回归测试。
