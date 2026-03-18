# MeowFS 宏存储系统

MeowFS 是 BinaryKeyboard 当前统一使用的宏存储格式，用来替代固定槽位的大块宏区。它同时覆盖 `CH552G`、`CH592F` 和 `Studio`，让有线与无线设备都使用同一套动态宏模型。

## 适用范围

- `CH552G`：`1 KB` 宏区，页擦除粒度 `64 B`
- `CH592F`：`8 KB` 宏区，页擦除粒度 `256 B`
- `Studio`：按动态宏列表管理，不再假设固定槽位

## 设计目标

- 用顺序存放的可变长条目替代固定宏槽
- 删除时只做标记，不立即整区擦除
- 将压缩整理放在 `Studio` 侧，降低固件复杂度
- 用同一套存储格式兼容 `CH552G` 和 `CH592F`

## 条目格式

每个宏条目结构如下：

```text
+--------+--------+--------------------+
| marker | count  | actions[count * 2] |
+--------+--------+--------------------+
```

- `marker = 0xAA`：有效宏
- `marker = 0x00`：已删除宏
- `marker = 0xFF`：空白区域，扫描到这里结束
- `count`：动作数，单动作固定 `2` 字节

动作本体继续使用原有的 `MacroActionType + param` 编码，没有为 DSL 单独引入第二套存储格式。

## 存储方式

MeowFS 使用顺序追加的方式写入宏数据。新的宏会写到当前有效数据末尾；删除已有宏时，只把条目标记改成已删除，不立即移动后续内容。

这种设计有两个直接效果：

- 宏条目可以紧密排列，不需要预留固定槽位
- 删除操作足够轻量，整理工作可以延后到需要时再做

## 为什么不做页对齐

`CH592F` 的擦除粒度是 `256 B`，但 MeowFS 的删除并不是“整页擦除”，而是把条目头的 `0xAA` 改成 `0x00`。因此条目本身不需要对齐到擦除页。

这意味着：

- 宏条目可以连续排布
- 小宏不会因为页对齐而浪费空间
- 只有 `Studio` 触发整区压缩重写时，才需要关心页擦除

## Studio 的行为

对于使用 MeowFS 的设备，`Studio` 统一采用动态宏列表模型：

- 宏面板展示当前有效宏，以及末尾的新建入口
- 新建宏时直接追加到末尾
- 覆盖已有宏时，读取全部有效宏后整区重写
- 删除宏时，设备侧只做删除标记，随后刷新列表
- 删除中间宏后，同步重排 `keymap` 和 `FN` 里的宏索引引用，避免绑定漂移

## 当前限制

- MeowFS 没有保留旧版 `24 B` 名称头，`Studio` 不再为 CH552/CH592 的动态宏提供名称编辑
- 宏索引仍然是运行时索引，不是稳定 UUID

## 协议

### CH552G

`CH552G` 使用单命令 `0x40`，通过子命令区分操作：

- `INFO`
- `READ`
- `ERASE`
- `WRITE`
- `DELETE`

### CH592F

`CH592F` 继续沿用原有 `0x40 ~ 0x43` 命令族，但语义已经切换到 MeowFS：

- `0x40 MACRO_INFO`：返回总容量、页大小、宏数量、剩余空间
- `0x41 MACRO_GET`：按偏移读取原始 MeowFS 数据
- `0x42 MACRO_SET`
  - `sub=0`：擦页或全擦
  - `sub=1`：按偏移写入原始数据
- `0x43 MACRO_DEL`：按宏索引标记删除

这样可以尽量减少固件接口变化，同时让 `Studio` 对 CH552 和 CH592 的操作方式保持一致。

## 相关实现

- `tools/studio/src/stores/macroStore.ts`
- `tools/studio/src/services/hid/devices/ch552/codec.ts`
- `tools/studio/src/services/hid/devices/ch592/codec.ts`
- `firmware/CH552G/keyboard/MacroStorage.c`
- `firmware/CH592F/keyboard/src/kbd_storage.c`
- `firmware/CH592F/keyboard/src/kbd_command.c`
