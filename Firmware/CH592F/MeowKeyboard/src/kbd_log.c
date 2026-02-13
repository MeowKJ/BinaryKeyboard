/**
 * @file    kbd_log.c
 * @brief   MeowKeyboard HID 日志系统实现
 * @author  MeowKJ
 * @version V1.2.0
 * @date    2024-11-07
 *
 * @details
 * 使用环形队列缓冲日志帧，避免在按键 ISR / 回调中直接操作 USB。
 * 主循环调用 KBD_Log_Flush() 逐条通过 EP4 发送。
 * 固件端只控制总开关，类别过滤由上位机 UI 完成。
 *
 * @copyright Copyright (c) 2024 MeowKJ. All rights reserved.
 */

#include "kbd_log.h"
#include "kbd_mode.h"
#include "kbd_storage.h"
#include <string.h>

/*============================================================================*/
/* 外部函数 (usb_hid.c)                                                       */
/*============================================================================*/

extern void USB_Config_SendResponse(uint8_t cmd, uint8_t *data, uint8_t len);

/*============================================================================*/
/* 运行时配置                                                                  */
/*============================================================================*/

static uint8_t s_enabled = 1;  /**< 日志总开关 */

/*============================================================================*/
/* 编译期常量                                                                  */
/*============================================================================*/

/** 单条日志最大数据长度 (不含 CMD/SUB/LEN 头) */
#define LOG_MAX_DATA  8

/** 每次 flush 最大条数 */
#define LOG_FLUSH_COUNT 2

/** 队列深度 (2 的幂便于取模) */
#define LOG_QUEUE_SIZE 16
#define LOG_QUEUE_MASK (LOG_QUEUE_SIZE - 1)

/*============================================================================*/
/* 环形队列                                                                    */
/*============================================================================*/

typedef struct {
    uint8_t category; /**< KBD_LOG_xxx */
    uint8_t len;      /**< 数据长度 */
    uint8_t data[LOG_MAX_DATA];
} log_entry_t;

static log_entry_t s_queue[LOG_QUEUE_SIZE];
static volatile uint8_t s_head = 0;  /**< 写入位置 */
static volatile uint8_t s_tail = 0;  /**< 读取位置 */

/** 队列是否为空 */
static inline uint8_t queue_empty(void) { return s_head == s_tail; }

/** 队列是否已满 */
static inline uint8_t queue_full(void)  { return ((s_head + 1) & LOG_QUEUE_MASK) == s_tail; }

/**
 * @brief 入队一条日志
 */
static void queue_push(uint8_t category, const uint8_t *data, uint8_t len)
{
    if (queue_full()) return; /* 满则丢弃 */

    log_entry_t *e = &s_queue[s_head];
    e->category = category;
    e->len = (len > LOG_MAX_DATA) ? LOG_MAX_DATA : len;
    memcpy(e->data, data, e->len);

    s_head = (s_head + 1) & LOG_QUEUE_MASK;
}

/** 出队一条日志，返回 0 成功，-1 空 */
static int queue_pop(log_entry_t *out)
{
    if (queue_empty()) return -1;

    *out = s_queue[s_tail];
    s_tail = (s_tail + 1) & LOG_QUEUE_MASK;
    return 0;
}

/*============================================================================*/
/* 公共函数                                                                    */
/*============================================================================*/

void KBD_Log_Init(void)
{
    s_head = 0;
    s_tail = 0;

    /* 从系统配置加载日志开关 */
    kbd_system_config_t *sys = KBD_GetSystemConfig();
    s_enabled = sys->log_enabled ? 1 : 0;
}

void KBD_Log_Flush(void)
{
    /* USB 未插入时清空队列，防止 EP4 阻塞主循环 */
    if (!KBD_Mode_USB_IsPlugged()) {
        s_head = s_tail;
        return;
    }

    log_entry_t entry;

    for (uint8_t i = 0; i < LOG_FLUSH_COUNT; i++) {
        if (queue_pop(&entry) != 0) break;

        /* 构造 [SUB=category][LEN=n][DATA...] 放入 buf */
        uint8_t buf[LOG_MAX_DATA + 2];
        buf[0] = entry.category;
        buf[1] = entry.len;
        memcpy(&buf[2], entry.data, entry.len);

        USB_Config_SendResponse(KBD_CMD_LOG, buf, entry.len + 2);
    }
}

/*============================================================================*/
/* 运行时配置                                                                  */
/*============================================================================*/

void KBD_Log_SetEnabled(uint8_t enabled)
{
    s_enabled = enabled ? 1 : 0;

    /* 同步到系统配置 RAM 副本 (需调用 CFG_SAVE 持久化) */
    kbd_system_config_t *sys = KBD_GetSystemConfig();
    sys->log_enabled = s_enabled;
}

uint8_t KBD_Log_IsEnabled(void)
{
    return s_enabled;
}

/*============================================================================*/
/* 日志记录函数                                                                */
/*============================================================================*/

void KBD_Log_KeyEvent(uint8_t key_index, uint8_t pressed, uint8_t action_type, uint8_t param)
{
    if (!s_enabled) return;
    uint8_t data[4] = { key_index, pressed, action_type, param };
    queue_push(KBD_LOG_KEY_EVENT, data, 4);
}

void KBD_Log_FnEvent(uint8_t fn_id, uint8_t is_long, uint8_t action, uint8_t param)
{
    if (!s_enabled) return;
    uint8_t data[4] = { fn_id, is_long, action, param };
    queue_push(KBD_LOG_FN_EVENT, data, 4);
}

void KBD_Log_LayerEvent(uint8_t old_layer, uint8_t new_layer)
{
    if (!s_enabled) return;
    uint8_t data[2] = { old_layer, new_layer };
    queue_push(KBD_LOG_LAYER_EVENT, data, 2);
}

void KBD_Log_ModeEvent(uint8_t old_mode, uint8_t new_mode)
{
    if (!s_enabled) return;
    uint8_t data[2] = { old_mode, new_mode };
    queue_push(KBD_LOG_MODE_EVENT, data, 2);
}

void KBD_Log_BleEvent(uint8_t state)
{
    if (!s_enabled) return;
    uint8_t data[1] = { state };
    queue_push(KBD_LOG_BLE_EVENT, data, 1);
}

void KBD_Log_RgbEvent(uint8_t mode, uint8_t brightness)
{
    if (!s_enabled) return;
    uint8_t data[2] = { mode, brightness };
    queue_push(KBD_LOG_RGB_EVENT, data, 2);
}

void KBD_Log_SystemEvent(uint8_t event)
{
    if (!s_enabled) return;
    uint8_t data[1] = { event };
    queue_push(KBD_LOG_SYSTEM_EVENT, data, 1);
}
