# TMOS 使用说明（CH592F）

基于 WCH BLE 库的 `TMOS`（任务/事件调度）在本项目中的用法说明，重点覆盖：

- `task + event` 事件模型
- 定时事件与消息事件
- 本项目现有调用路径
- 用 TMOS 做高频状态（如层号）延迟保存

> 本页面向项目开发，术语与接口以 `CH59xBLE_LIB.h` 和当前固件代码为准。

## 什么是 TMOS

TMOS 是 WCH BLE 库内置的轻量任务调度系统，核心是：

- `Task`：任务（注册后获得 `taskID`）
- `Event`：事件位图（每个 bit 表示一种事件）
- `Timer`：延时触发某个事件
- `Message`：任务消息队列（触发 `SYS_EVENT_MSG`）

任务处理函数原型（项目实际使用）：

```c
uint16_t Task_ProcessEvent(uint8_t task_id, uint16_t events);
```

处理完某个事件后，返回 `events ^ EVT_xxx`（清除已处理事件位）。

## 核心规则（很重要）

### 1. 事件是位图

- 一个任务一次可能收到多个事件（`events` 是位图）
- 用 `if (events & EVT_xxx)` 逐个处理
- 每个事件用独立 bit，避免冲突

### 2. `SYS_EVENT_MSG` 是系统保留事件

- `0x8000`（最高位）由 TMOS 用于“消息到达”
- 用户自定义事件不要占用 `0x8000`
- 你自己的事件建议从 `0x0001`、`0x0002`、`0x0004`... 开始

### 3. 任务优先级与注册顺序有关

- 先注册的任务优先级更高
- 优先级相同时按注册顺序处理

### 4. 事件处理函数要短小

- 不要在事件回调里做长时间阻塞操作
- 慢操作（如 Flash 擦写）应通过“延时事件 + 合并写入”处理

## 常用 API（项目里会用到）

接口声明见 `firmware/CH592F/MeowBLE/lib/CH59xBLE_LIB.h`。

| API | 作用 | 备注 |
| :-- | :-- | :-- |
| `TMOS_ProcessEventRegister(cb)` | 注册任务处理函数 | 返回 `taskID` |
| `TMOS_SystemProcess()` | 处理系统任务/事件 | 主循环里持续调用 |
| `tmos_set_event(task, evt)` | 立即触发事件 | 异步执行 |
| `tmos_start_task(task, evt, time)` | 延时触发一次事件 | 常用防抖/定时 |
| `tmos_start_reload_task(task, evt, time)` | 周期事件 | 自动重装载 |
| `tmos_stop_task(task, evt)` | 停止某事件定时器 | 取消延时任务 |
| `tmos_get_task_timer(task, evt)` | 查询事件剩余定时 | 调试用 |
| `tmos_msg_allocate(len)` | 分配消息缓冲 | 从 TMOS 内存池分配 |
| `tmos_msg_send(task, msg)` | 投递消息到任务 | 会触发 `SYS_EVENT_MSG` |
| `tmos_msg_receive(task)` | 读取一条消息 | 在 `SYS_EVENT_MSG` 中调用 |
| `tmos_msg_deallocate(msg)` | 释放消息 | 收到消息后要释放 |

## 项目中的 TMOS 调用路径

### 主循环驱动 TMOS

`firmware/CH592F/User/Main.c`

```c
while (1) {
    TMOS_SystemProcess();
    KBD_Mode_Process();
    KBD_Core_Process();
    KBD_Log_Flush();
}
```

说明：

- `TMOS_SystemProcess()` 负责分发 TMOS 任务事件
- 你的业务循环（模式/按键/日志）继续在主循环执行
- 这意味着 TMOS 事件处理和业务主循环是协作式的，事件回调越短越好

### 示例 1：RGB 周期更新（定时事件）

`firmware/CH592F/MeowKeyboard/src/kbd_rgb.c`

- 注册任务：`TMOS_ProcessEventRegister(KBD_RGB_ProcessEvent)`
- 启动定时：`tmos_start_task(..., RGB_UPDATE_EVT, ...)`
- 在事件中处理后再次 `tmos_start_task(...)`，形成周期调度

这是项目里最清晰的 TMOS 用法模板，适合复用到 runtime 保存任务。

### 示例 2：BLE/HAL 消息事件（`SYS_EVENT_MSG`）

`firmware/CH592F/MeowBLE/core/src/ble_mcu.c` 与 `firmware/CH592F/MeowBLE/hid/src/ble_hid.c`

典型模式：

```c
if (events & SYS_EVENT_MSG) {
    uint8_t *msg = tmos_msg_receive(task_id);
    if (msg) {
        // 处理消息
        tmos_msg_deallocate(msg);
    }
    return (events ^ SYS_EVENT_MSG);
}
```

重点：

- 处理完消息必须 `tmos_msg_deallocate()`
- `SYS_EVENT_MSG` 是系统事件，优先处理通常更稳

## 推荐：用 TMOS 做 runtime 延迟保存（高频状态）

适用场景：

- `current_layer`（高频变化）
- 未来可扩展 `active_mode`

目标：

- 不在切层路径直接擦写 DataFlash
- 合并短时间内连续变化
- 减少对 BLE/USB 响应的影响

### 设计原则（简洁版）

- `runtime` 才轮转（256B 页环）
- 低频配置不做复杂轮转（按 `CFG_SAVE` 保存）
- 写前比较，没变化不写
- 用 TMOS 延时事件防抖（如 `150~300ms`）

### 推荐事件定义

```c
#define KBD_STORAGE_RUNTIME_SAVE_EVT   0x0001
```

> 不要使用 `0x8000`，该位保留给 `SYS_EVENT_MSG`。

### 推荐状态变量

```c
static tmosTaskID s_storage_task_id = TASK_NO_TASK;
static uint8_t s_runtime_dirty = 0;
static uint8_t s_runtime_pending_layer = 0;
static uint8_t s_runtime_last_saved_layer = 0xFF;
```

### 推荐接口（可读性优先）

```c
void KBD_Storage_TMOS_Init(void);
void KBD_Storage_RequestRuntimeSave(uint8_t layer);
static uint16_t KBD_Storage_ProcessEvent(uint8_t task_id, uint16_t events);
static void KBD_Storage_FlushRuntimeIfDirty(void);
```

### 调用流程

1. `KBD_SetCurrentLayer(new_layer)` 只更新 RAM
2. 若值变化，调用 `KBD_Storage_RequestRuntimeSave(new_layer)`
3. `RequestRuntimeSave()` 仅做：
   - 标记 `dirty`
   - 更新 `pending_layer`
   - 重启一次延时事件（防抖）
4. TMOS 事件到期后执行实际 `SaveRuntime()`（256B 页轮转）

### 代码模板（可直接改造）

```c
#define KBD_STORAGE_RUNTIME_SAVE_EVT  0x0001
#define KBD_STORAGE_RUNTIME_SAVE_DELAY_MS  200

static tmosTaskID s_storage_task_id = TASK_NO_TASK;
static uint8_t s_runtime_dirty = 0;
static uint8_t s_runtime_pending_layer = 0;
static uint8_t s_runtime_last_saved_layer = 0xFF;

static uint16_t KBD_Storage_ProcessEvent(uint8_t task_id, uint16_t events)
{
    (void)task_id;

    if (events & KBD_STORAGE_RUNTIME_SAVE_EVT) {
        if (s_runtime_dirty && s_runtime_pending_layer != s_runtime_last_saved_layer) {
            if (KBD_Storage_SaveRuntimeLayer(s_runtime_pending_layer) == 0) {
                s_runtime_last_saved_layer = s_runtime_pending_layer;
                s_runtime_dirty = 0;
            } else {
                /* 写失败：短延时重试，避免阻塞当前路径 */
                tmos_start_task(s_storage_task_id,
                                KBD_STORAGE_RUNTIME_SAVE_EVT,
                                MS1_TO_SYSTEM_TIME(100));
            }
        } else {
            s_runtime_dirty = 0;
        }
        return (events ^ KBD_STORAGE_RUNTIME_SAVE_EVT);
    }

    return 0;
}

void KBD_Storage_TMOS_Init(void)
{
    s_storage_task_id = TMOS_ProcessEventRegister(KBD_Storage_ProcessEvent);
}

void KBD_Storage_RequestRuntimeSave(uint8_t layer)
{
    if (layer == s_runtime_last_saved_layer) {
        return;  /* 无变化不写 */
    }

    s_runtime_pending_layer = layer;
    s_runtime_dirty = 1;

    /* 防抖：重启延时保存 */
    tmos_stop_task(s_storage_task_id, KBD_STORAGE_RUNTIME_SAVE_EVT);
    tmos_start_task(s_storage_task_id,
                    KBD_STORAGE_RUNTIME_SAVE_EVT,
                    MS1_TO_SYSTEM_TIME(KBD_STORAGE_RUNTIME_SAVE_DELAY_MS));
}
```

## TMOS 内存池（项目注意点）

`firmware/CH592F/User/Main.c` 中定义了 TMOS/BLE 使用的内存池：

```c
__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];
```

建议：

- 保持 4 字节对齐
- `BLE_MEMHEAP_SIZE` 不足时，消息分配/协议栈行为会异常
- 若新增消息型任务，先观察内存余量

## 常见坑（建议直接规避）

### 1. 在高频路径直接写 Flash

问题：

- 层切换/按键路径里直接 `EEPROM_ERASE/WRITE` 会放大延迟

建议：

- 改成 TMOS 延时事件写入（上面的模板）

### 2. 忘记释放消息

问题：

- `tmos_msg_receive()` 后不 `tmos_msg_deallocate()` 会泄漏内存池

建议：

- 在 `SYS_EVENT_MSG` 分支固定写成“receive -> process -> deallocate”模板

### 3. 事件位冲突

问题：

- 同一任务里多个事件用到相同 bit，会出现逻辑串扰

建议：

- 每个任务集中定义事件位（按 `0x0001/0x0002/0x0004...`）

### 4. 使用 `0x8000` 作为自定义事件

问题：

- 与 `SYS_EVENT_MSG` 冲突

建议：

- `0x8000` 保留给系统消息事件

### 5. 事件处理函数过长

问题：

- 会影响 BLE/USB 响应及时性

建议：

- 事件回调只做状态推进与短操作
- 长操作拆分或延后

## 调试建议

- 给每个 TMOS 任务记录 `taskID`（日志里打印一次）
- 对关键事件打印节流日志（不要每次都打印）
- 调试“事件没触发”时优先检查：
  - 任务是否已注册
  - 事件 bit 是否冲突
  - 是否误用了 `0x8000`
  - `tmos_start_task()` 的 `taskID` 是否正确
  - 主循环是否持续调用 `TMOS_SystemProcess()`

## 参考资料

- WCH 产品页（CH592，官方资料入口）：<https://www.wch.cn/products/CH592.html>
- 《CH58x BLE 软件开发参考手册》TMOS 章节（镜像，便于检索）：<https://manuals.plus/vi/bez-imeni/ch58x-ble-software-development-manual>

> 本项目文档基于官方 TMOS 说明整理，并结合 `CH592F` 固件现有代码用法进行约束与建议。
