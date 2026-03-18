/**
 * @file    kbd_macro.c
 * @brief   MeowKeyboard 宏回放引擎实现
 * @author  MeowKJ
 *
 * @details
 * 基于 TMOS 定时器的非阻塞宏回放引擎。
 * 支持 4 种触发模式：单次、按住-立即停、按住-跑完停、切换循环。
 * 每步按需从 Flash 读取 2 字节动作，静态 RAM 开销 ~50 字节。
 */

#include "kbd_macro.h"
#include "kbd_storage.h"
#include "kbd_mode.h"
#include "CH59x_common.h"
#include "ble_config.h"
#include "debug.h"

#define TAG "MACRO"

/*============================================================================*/
/* TMOS 事件                                                                   */
/*============================================================================*/

#define MACRO_STEP_EVT 0x0001

/*============================================================================*/
/* 状态定义                                                                    */
/*============================================================================*/

enum {
    MACRO_IDLE = 0,
    MACRO_RUNNING = 1,
};

/*============================================================================*/
/* 私有变量                                                                    */
/*============================================================================*/

static tmosTaskID s_task_id;
static uint8_t    s_state = MACRO_IDLE;

/** 当前回放的宏 */
static uint8_t              s_slot;
static kbd_macro_trigger_t  s_trigger;
static kbd_macro_header_t   s_header;
static uint16_t             s_action_idx;

/** 触发键状态 */
static bool s_key_released;
static bool s_cancel_req;

/** 宏独立的 HID 按键状态 */
static uint8_t s_mod_mask;
static uint8_t s_keys[6];
static uint8_t s_key_count;

/*============================================================================*/
/* 私有函数声明                                                                */
/*============================================================================*/

static uint16_t KBD_Macro_ProcessEvent(uint8_t task_id, uint16_t events);
static void MacroStepActions(void);
static bool ShouldLoop(void);
static void MacroAddKey(uint8_t keycode);
static void MacroRemoveKey(uint8_t keycode);
static void MacroSendKeyboardReport(void);
static void MacroReleaseAll(void);

/*============================================================================*/
/* 公共函数                                                                    */
/*============================================================================*/

void KBD_Macro_Init(void)
{
    s_task_id = TMOS_ProcessEventRegister(KBD_Macro_ProcessEvent);
    s_state = MACRO_IDLE;
    LOG_I(TAG, "Macro engine initialized");
}

int KBD_Macro_Execute(uint8_t slot, kbd_macro_trigger_t trigger)
{
    /* 若已在运行，先取消 */
    if (s_state != MACRO_IDLE) {
        KBD_Macro_Cancel();
    }

    /* 读取宏头部 */
    int ret = Kbd_Macro_GetInfo(slot, &s_header);
    if (ret != 0 || s_header.valid != KBD_MACRO_VALID_MAGIC) {
        LOG_W(TAG, "Slot %d invalid or read fail", slot);
        return -2;
    }

    if (s_header.action_count == 0) {
        LOG_D(TAG, "Slot %d empty", slot);
        return 0;
    }

    /* 初始化回放状态 */
    s_slot = slot;
    s_trigger = trigger;
    s_action_idx = 0;
    s_key_released = false;
    s_cancel_req = false;
    s_mod_mask = 0;
    s_key_count = 0;
    for (uint8_t i = 0; i < 6; i++) {
        s_keys[i] = 0;
    }

    s_state = MACRO_RUNNING;
    LOG_I(TAG, "Execute slot %d, trigger %d, %d actions",
          slot, trigger, s_header.action_count);

    /* 立即开始第一步 */
    tmos_set_event(s_task_id, MACRO_STEP_EVT);
    return 0;
}

void KBD_Macro_OnKeyRelease(void)
{
    if (s_state != MACRO_RUNNING)
        return;

    s_key_released = true;

    if (s_trigger == KBD_MACRO_TRIG_HOLD_ABORT) {
        /* 按住-立即停: 松开立即中断 */
        KBD_Macro_Cancel();
    }
    /* HOLD_FINISH / TOGGLE: 由 ShouldLoop() 在当轮结束时判定 */
}

void KBD_Macro_Cancel(void)
{
    if (s_state == MACRO_IDLE)
        return;

    tmos_stop_task(s_task_id, MACRO_STEP_EVT);
    MacroReleaseAll();
    s_state = MACRO_IDLE;
    LOG_D(TAG, "Cancelled");
}

bool KBD_Macro_IsRunning(void)
{
    return s_state == MACRO_RUNNING;
}

/*============================================================================*/
/* TMOS 事件处理                                                               */
/*============================================================================*/

static uint16_t KBD_Macro_ProcessEvent(uint8_t task_id, uint16_t events)
{
    if (events & MACRO_STEP_EVT) {
        MacroStepActions();
        return (events ^ MACRO_STEP_EVT);
    }
    return 0;
}

/*============================================================================*/
/* 核心步进逻辑                                                                */
/*============================================================================*/

static void MacroStepActions(void)
{
    if (s_state != MACRO_RUNNING)
        return;

    while (s_action_idx < s_header.action_count) {
        kbd_macro_action_t action;
        uint16_t offset = s_action_idx * sizeof(kbd_macro_action_t);

        int ret = Kbd_Macro_Read(s_slot, offset, (uint8_t *)&action, sizeof(action));
        if (ret < (int)sizeof(action)) {
            LOG_W(TAG, "Flash read fail at idx %d", s_action_idx);
            KBD_Macro_Cancel();
            return;
        }

        s_action_idx++;

        switch (action.type) {
        case KBD_MACRO_KEY_DOWN:
            MacroAddKey(action.param);
            MacroSendKeyboardReport();
            break;

        case KBD_MACRO_KEY_UP:
            MacroRemoveKey(action.param);
            MacroSendKeyboardReport();
            break;

        case KBD_MACRO_MOD_DOWN:
            s_mod_mask |= action.param;
            MacroSendKeyboardReport();
            break;

        case KBD_MACRO_MOD_UP:
            s_mod_mask &= ~action.param;
            MacroSendKeyboardReport();
            break;

        case KBD_MACRO_DELAY: {
            uint32_t delay_ms = (uint32_t)action.param * 10;
            if (delay_ms == 0) delay_ms = 10;
            tmos_start_task(s_task_id, MACRO_STEP_EVT,
                            MS1_TO_SYSTEM_TIME(delay_ms));
            return; /* 等待定时器回调继续 */
        }

        case KBD_MACRO_CONSUMER:
            KBD_Mode_SendConsumerReport((uint16_t)action.param);
            /* 短暂延时后释放 */
            tmos_start_task(s_task_id, MACRO_STEP_EVT,
                            MS1_TO_SYSTEM_TIME(20));
            /* 发送释放 */
            KBD_Mode_SendConsumerReport(0);
            return;

        case KBD_MACRO_MOUSE_DOWN:
            KBD_Mode_SendMouseReport(action.param, 0, 0, 0);
            break;

        case KBD_MACRO_MOUSE_UP:
            KBD_Mode_SendMouseReport(0, 0, 0, 0);
            break;

        case KBD_MACRO_WHEEL: {
            int8_t wheel = 0;
            if (action.param == KBD_WHEEL_UP)
                wheel = 1;
            else if (action.param == KBD_WHEEL_DOWN)
                wheel = -1;
            KBD_Mode_SendMouseReport(0, 0, 0, wheel);
            break;
        }

        case KBD_MACRO_END:
            goto macro_round_done;

        default:
            /* 跳过未知动作 */
            break;
        }
    }

macro_round_done:
    if (ShouldLoop()) {
        /* 重新开始 */
        s_action_idx = 0;
        tmos_set_event(s_task_id, MACRO_STEP_EVT);
        LOG_D(TAG, "Loop restart");
    } else {
        MacroReleaseAll();
        s_state = MACRO_IDLE;
        LOG_D(TAG, "Slot %d done", s_slot);
    }
}

/*============================================================================*/
/* 循环判定                                                                    */
/*============================================================================*/

static bool ShouldLoop(void)
{
    switch (s_trigger) {
    case KBD_MACRO_TRIG_ONCE:
        return false;
    case KBD_MACRO_TRIG_HOLD_ABORT:
        /* 若还没松开则继续循环（松开时已经 Cancel 了） */
        return !s_key_released;
    case KBD_MACRO_TRIG_HOLD_FINISH:
        return !s_key_released;
    case KBD_MACRO_TRIG_TOGGLE:
        return !s_cancel_req;
    }
    return false;
}

/*============================================================================*/
/* HID 按键状态管理                                                            */
/*============================================================================*/

static void MacroAddKey(uint8_t keycode)
{
    /* 检查是否已在列表中 */
    for (uint8_t i = 0; i < s_key_count; i++) {
        if (s_keys[i] == keycode) return;
    }
    if (s_key_count < 6) {
        s_keys[s_key_count++] = keycode;
    }
}

static void MacroRemoveKey(uint8_t keycode)
{
    for (uint8_t i = 0; i < s_key_count; i++) {
        if (s_keys[i] == keycode) {
            /* 用最后一个填补 */
            s_keys[i] = s_keys[s_key_count - 1];
            s_keys[s_key_count - 1] = 0;
            s_key_count--;
            return;
        }
    }
}

static void MacroSendKeyboardReport(void)
{
    KBD_Mode_SendKeyboardReport(s_mod_mask, s_keys, s_key_count);
}

static void MacroReleaseAll(void)
{
    /* 释放所有键盘键 */
    s_mod_mask = 0;
    s_key_count = 0;
    for (uint8_t i = 0; i < 6; i++) {
        s_keys[i] = 0;
    }
    KBD_Mode_SendKeyboardReport(0, s_keys, 0);

    /* 释放鼠标 */
    KBD_Mode_SendMouseReport(0, 0, 0, 0);

    /* 释放多媒体键 */
    KBD_Mode_SendConsumerReport(0);
}
