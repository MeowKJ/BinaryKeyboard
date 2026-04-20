/**
 * @file    kbd_core.c
 * @brief   MeowKeyboard 核心处理模块实现
 * @author  MeowKJ
 * @version V2.0.0
 * @date    2024-11-07
 */

#include "kbd_core.h"
#include "kbd_storage.h"
#include "kbd_macro.h"
#include "kbd_rgb.h"
#include "kbd_mode.h"
#include "kbd_log.h"
#include "hal_utils.h"
#include "key.h"
#include "encoder.h"
#include "debug.h"

#define TAG "CORE"

/*============================================================================*/
/* 私有变量 */
/*============================================================================*/

/** 当前按下的按键 (用于多键同时按下，HID 最多 6 键) */
static uint8_t s_pressed_keys[6] = {0};
static uint8_t s_pressed_count = 0;
static uint8_t s_current_modifier = 0;
static bool s_boot_key_prev_pressed = false;

/*============================================================================*/
/* 私有函数声明 */
/*============================================================================*/

static void ExecuteKeyAction(const kbd_action_t *action, bool pressed);
static void ExecuteFnAction(kbd_fn_action_t action, uint8_t param);
static void OnModeChange(kbd_work_mode_t new_mode);
static void OnConnStateChange(kbd_conn_state_t state);
static void OnLedReport(uint8_t leds);
static void KBD_Core_ProcessBootKey(void);

/*============================================================================*/
/* 模式管理回调结构 */
/*============================================================================*/

static kbd_mode_callbacks_t s_callbacks = {
    .onModeChange = OnModeChange,
    .onConnStateChange = OnConnStateChange,
    .onLedReport = OnLedReport,
};

/*============================================================================*/
/* 公共函数实现 */
/*============================================================================*/

/**
 * @brief 初始化键盘核心模块
 */
void KBD_Core_Init(void)
{
    s_pressed_count = 0;
    s_current_modifier = 0;
    s_boot_key_prev_pressed = (BootKey_IsPressed() == 1);

    for (uint8_t i = 0; i < 6; i++)
    {
        s_pressed_keys[i] = 0;
    }

    LOG_I(TAG, "Core initialized");
}

/**
 * @brief 核心处理函数（主循环调用）
 */
void KBD_Core_Process(void)
{
    key_event_t key_evt;
    fnkey_event_t fn_evt;

    KBD_Core_ProcessBootKey();

    /* 处理普通按键事件 */
    while (Key_GetEvent(&key_evt))
    {
        KBD_Core_HandleKeyEvent(&key_evt);
    }

    /* 处理旋钮事件 */
    while (Encoder_GetEvent(&key_evt))
    {
        KBD_Core_HandleKeyEvent(&key_evt);
    }

    /* 处理 FN 按键事件 */
    while (FnKey_GetEvent(&fn_evt))
    {
        KBD_Core_HandleFnEvent(&fn_evt);
    }
}

/**
 * @brief 检查是否有 FN 键被按下
 */
static bool IsFnKeyHeld(void)
{
    for (uint8_t i = 0; i < KBD_MAX_FN_KEYS; i++)
    {
        if (FnKey_IsDown(i) == 1)
        {
            return true;
        }
    }
    return false;
}

static void KBD_Core_ProcessBootKey(void)
{
#if KBD_BOOT_KEY_ENTER_BOOTLOADER
    bool pressed = (BootKey_IsPressed() == 1);

    if (pressed && !s_boot_key_prev_pressed)
    {
        LOG_W(TAG, "BOOT: enter IAP");
        KBD_Core_ReleaseAll();
        KBD_RGB_Flash(255, 140, 0, 120);
        mDelaymS(120);
        Hal_JumpToBootloader();
    }

    s_boot_key_prev_pressed = pressed;
#endif
}

/**
 * @brief 处理普通按键事件
 */
void KBD_Core_HandleKeyEvent(const key_event_t *evt)
{
    if (evt == NULL || evt->key >= KBD_MAX_KEYS)
        return;

    KBD_Mode_RecordActivity();

    bool pressed = (evt->type == KEY_EVT_PRESS);

    /* 按下效果：记录按键事件到 RGB 引擎 */
    if (pressed)
    {
        KBD_RGB_RegisterKeyPress(evt->key);
    }

    /* FN 组合键：按住 FN + 按键 = 切换到对应层 */
    if (IsFnKeyHeld() && pressed)
    {
        uint8_t target_layer = evt->key; /* 按键0->层0, 按键1->层1... */
        kbd_keymap_t *keymap = KBD_GetKeymap();

        if (target_layer < keymap->num_layers)
        {
            uint8_t old_layer = keymap->current_layer;
            KBD_SetCurrentLayer(target_layer);
            LOG_I(TAG, "FN+Key%d -> Layer %d", evt->key, target_layer);
            KBD_Log_LayerEvent(old_layer, target_layer);
            KBD_RGB_FlashLayer(target_layer);
            /* 标记所有按住的 FN 键: 松开时不触发 click/long */
            for (uint8_t i = 0; i < KBD_MAX_FN_KEYS; i++)
            {
                if (FnKey_IsDown(i) == 1)
                    FnKey_MarkComboUsed(i);
            }
            return; /* 不执行按键原本的动作 */
        }
    }

    const kbd_action_t *action = KBD_GetKeyAction(evt->key);
    if (action == NULL)
        return;

    LOG_D(TAG, "key %d %s", evt->key, pressed ? "press" : "release");
    KBD_Log_KeyEvent(evt->key, pressed ? 1 : 0, action->type, action->param1);
    ExecuteKeyAction(action, pressed);
}

/**
 * @brief 处理 FN 按键事件
 */
void KBD_Core_HandleFnEvent(const fnkey_event_t *evt)
{
    if (evt == NULL || evt->id >= KBD_MAX_FN_KEYS)
        return;

    KBD_Mode_RecordActivity();

    kbd_fnkey_config_t *fnkey_cfg = KBD_GetFnKeyConfig();
    kbd_fnkey_entry_t *entry = &fnkey_cfg->fn[evt->id];

    if (evt->type == FNKEY_EVT_LONG)
    {
        LOG_D(TAG, "FN%d long", evt->id + 1);
        KBD_Log_FnEvent(evt->id, 1, entry->long_action, entry->long_param);
        ExecuteFnAction((kbd_fn_action_t)entry->long_action, entry->long_param);
    }
    else
    {
        LOG_D(TAG, "FN%d click", evt->id + 1);
        KBD_Log_FnEvent(evt->id, 0, entry->click_action, entry->click_param);
        ExecuteFnAction((kbd_fn_action_t)entry->click_action, entry->click_param);
    }
}

/**
 * @brief 释放所有按键
 */
void KBD_Core_ReleaseAll(void)
{
    s_pressed_count = 0;
    s_current_modifier = 0;
    for (uint8_t i = 0; i < 6; i++)
    {
        s_pressed_keys[i] = 0;
    }
    KBD_Mode_ReleaseAllKeys();
}

/**
 * @brief 获取双模回调结构
 */
void *KBD_Core_GetCallbacks(void)
{
    return &s_callbacks;
}

/*============================================================================*/
/* 模式管理回调实现 */
/*============================================================================*/

/**
 * @brief 模式切换回调
 */
static void OnModeChange(kbd_work_mode_t new_mode)
{
    uint8_t old_mode = (new_mode == KBD_WORK_MODE_USB) ? 1 : 0; /* 反推旧模式 */
    LOG_I(TAG, "mode changed: %s", (new_mode == KBD_WORK_MODE_USB) ? "USB" : "BLE");
    KBD_Log_ModeEvent(old_mode, (uint8_t)new_mode);

    /* 切换模式时释放所有按键 */
    KBD_Core_ReleaseAll();

    /* RGB 状态指示 + 切换确认闪烁（200ms） */
    if (new_mode == KBD_WORK_MODE_USB)
    {
        KBD_RGB_Flash(255, 255, 255, 200); /* 白色短闪 = 进入 USB 模式 */
        KBD_RGB_SetState(KBD_STATE_USB_CONNECTED);
    }
    else
    {
        KBD_RGB_Flash(0, 80, 255, 200); /* 蓝色短闪 = 进入 BLE 模式 */
        /* 根据实际连接状态设置指示灯，避免覆盖已进入广播的蓝色状态 */
        kbd_conn_state_t conn = KBD_Mode_GetConnState();
        if (conn == KBD_CONN_ADVERTISING)
        {
            KBD_RGB_SetState(KBD_STATE_BLE_ADVERTISING);
        }
        else
        {
            KBD_RGB_SetState(KBD_STATE_BLE_DISCONNECTED);
        }
    }
}

/**
 * @brief 连接状态变化回调
 */
static void OnConnStateChange(kbd_conn_state_t state)
{
    LOG_I(TAG, "conn state: %d", state);
    KBD_Log_BleEvent((uint8_t)state);

    /* RGB 状态指示 */
    if (KBD_Mode_Get() == KBD_WORK_MODE_USB)
    {
        KBD_RGB_SetState(KBD_STATE_USB_CONNECTED);
    }
    else
    {
        switch (state)
        {
        case KBD_CONN_DISCONNECTED:
            KBD_RGB_SetState(KBD_STATE_BLE_DISCONNECTED);
            break;
        case KBD_CONN_ADVERTISING:
            KBD_RGB_SetState(KBD_STATE_BLE_ADVERTISING);
            break;
        case KBD_CONN_CONNECTED:
            KBD_RGB_SetState(KBD_STATE_BLE_CONNECTED);
            break;
        case KBD_CONN_SUSPENDED:
            KBD_RGB_SetState(KBD_STATE_BLE_DISCONNECTED);
            break;
        }
    }
}

/**
 * @brief LED 报告回调（来自主机的 Caps Lock 等状态）
 */
static void OnLedReport(uint8_t leds)
{
    LOG_D(TAG, "led report: 0x%02X", leds);
    /* 可在此控制物理 LED 指示灯 */
}

/*============================================================================*/
/* 私有函数实现 */
/*============================================================================*/

/**
 * @brief 执行按键动作
 */
static void ExecuteKeyAction(const kbd_action_t *action, bool pressed)
{
    if (action == NULL || action->type == KBD_ACTION_NONE)
        return;

    switch (action->type)
    {
    case KBD_ACTION_KEYBOARD:
        if (pressed)
        {
            /* 按下：添加按键 */
            if (s_pressed_count < 6)
            {
                s_pressed_keys[s_pressed_count++] = action->param1;
            }
            s_current_modifier |= action->modifier;
            KBD_Mode_SendKeyboardReport(s_current_modifier, s_pressed_keys, s_pressed_count);
        }
        else
        {
            /* 释放：移除按键 */
            for (uint8_t i = 0; i < s_pressed_count; i++)
            {
                if (s_pressed_keys[i] == action->param1)
                {
                    for (uint8_t j = i; j < s_pressed_count - 1; j++)
                    {
                        s_pressed_keys[j] = s_pressed_keys[j + 1];
                    }
                    s_pressed_count--;
                    break;
                }
            }
            s_current_modifier &= ~action->modifier;
            if (s_pressed_count > 0)
            {
                KBD_Mode_SendKeyboardReport(s_current_modifier, s_pressed_keys, s_pressed_count);
            }
            else
            {
                KBD_Mode_ReleaseAllKeys();
            }
        }
        break;

    case KBD_ACTION_MOUSE_BTN:
        if (pressed)
        {
            KBD_Mode_SendMouseReport(action->param1, 0, 0, 0);
        }
        else
        {
            KBD_Mode_SendMouseReport(0, 0, 0, 0);
        }
        break;

    case KBD_ACTION_MOUSE_WHEEL:
        if (pressed)
        {
            int8_t wheel = 0;
            switch (action->param1)
            {
            case KBD_WHEEL_UP:
                wheel = 1;
                break;
            case KBD_WHEEL_DOWN:
                wheel = -1;
                break;
            case KBD_WHEEL_CLICK:
                KBD_Mode_SendMouseReport(KBD_MOUSE_MIDDLE, 0, 0, 0);
                mDelaymS(50);
                KBD_Mode_SendMouseReport(0, 0, 0, 0);
                return;
            }
            if (wheel != 0)
            {
                KBD_Mode_SendMouseReport(0, 0, 0, wheel);
            }
        }
        break;

    case KBD_ACTION_CONSUMER:
    {
        uint16_t consumer_code = action->param1 | (action->param2 << 8);
        if (pressed)
        {
            KBD_Mode_SendConsumerReport(consumer_code);
        }
        else
        {
            KBD_Mode_SendConsumerReport(0);
        }
    }
    break;

    case KBD_ACTION_LAYER:
        if (pressed)
        {
            uint8_t old_l = KBD_GetCurrentLayer();
            KBD_SetCurrentLayer(action->param1);
            LOG_I(TAG, "Layer -> %d", action->param1);
            KBD_Log_LayerEvent(old_l, action->param1);
            /* 切层时停止正在运行的宏 */
            if (KBD_Macro_IsRunning())
            {
                KBD_Macro_Cancel();
            }
        }
        break;

    case KBD_ACTION_MACRO:
        if (pressed)
        {
            kbd_macro_trigger_t trig = (kbd_macro_trigger_t)action->modifier;
            if (trig == KBD_MACRO_TRIG_TOGGLE && KBD_Macro_IsRunning())
            {
                KBD_Macro_Cancel(); /* Toggle 模式: 再按 → 停 */
            }
            else
            {
                int ret = KBD_Macro_Execute(action->param1, trig);
                if (ret != 0)
                {
                    LOG_W(TAG, "Macro %d exec fail: %d", action->param1, ret);
                }
            }
        }
        else
        {
            /* 松开事件通知宏引擎 */
            KBD_Macro_OnKeyRelease();
        }
        break;

    default:
        break;
    }
}

/**
 * @brief 执行 FN 动作
 */
static void ExecuteFnAction(kbd_fn_action_t action, uint8_t param)
{
    int ret;

    switch (action)
    {
    case KBD_FN_NONE:
        break;

    /* 模式控制 */
    case KBD_FN_MODE_TOGGLE:
        LOG_I(TAG, "FN: mode toggle");
        ret = KBD_Mode_Toggle();
        if (ret != 0)
        {
            LOG_W(TAG, "mode toggle failed: %d", ret);
            KBD_RGB_Flash(255, 0, 0, 200);
        }
        break;

    case KBD_FN_BLE_ADV:
        if (KBD_Mode_Get() == KBD_WORK_MODE_BLE && !KBD_Mode_IsConnected())
        {
            LOG_I(TAG, "FN: start adv");
            ret = KBD_Mode_BLE_StartAdvertising();
            if (ret != 0)
            {
                LOG_W(TAG, "start adv failed: %d", ret);
                KBD_RGB_Flash(255, 0, 0, 200);
            }
        }
        break;

    case KBD_FN_BLE_DISCONNECT:
        LOG_I(TAG, "FN: disconnect");
        ret = KBD_Mode_BLE_Disconnect();
        if (ret != 0)
        {
            LOG_W(TAG, "disconnect failed: %d", ret);
        }
        break;

    case KBD_FN_BLE_CLEAR_BONDS:
        if (KBD_Mode_Get() != KBD_WORK_MODE_BLE)
        {
            /* 非 BLE 模式：按用户要求静默忽略 */
            break;
        }
        LOG_I(TAG, "FN: clear bonds");
        ret = KBD_Mode_BLE_ClearBonds();
        if (ret == 0)
        {
            KBD_RGB_Flash(0, 120, 255, 300);
        }
        else
        {
            LOG_W(TAG, "clear bonds failed: %d", ret);
            KBD_RGB_Flash(255, 0, 0, 300);
        }
        break;

    /* RGB 控制 */
    case KBD_FN_RGB_TOGGLE:
        KBD_RGB_Toggle();
        KBD_Config_Save();
        break;

    case KBD_FN_RGB_MODE_NEXT:
        KBD_RGB_NextMode();
        KBD_Config_Save();
        break;

    case KBD_FN_RGB_MODE_PREV:
        KBD_RGB_PrevMode();
        KBD_Config_Save();
        break;

    case KBD_FN_RGB_BRIGHT_UP:
        KBD_RGB_BrightnessUp(16);
        KBD_Config_Save();
        break;

    case KBD_FN_RGB_BRIGHT_DOWN:
        KBD_RGB_BrightnessDown(16);
        KBD_Config_Save();
        break;

    /* 层控制 */
    case KBD_FN_LAYER_NEXT:
    {
        if (KBD_Macro_IsRunning())
            KBD_Macro_Cancel();
        uint8_t layer = KBD_NextLayer();
        LOG_I(TAG, "FN: layer next -> %d", layer);
        KBD_RGB_FlashLayer(layer);
    }
    break;

    case KBD_FN_LAYER_PREV:
    {
        if (KBD_Macro_IsRunning())
            KBD_Macro_Cancel();
        uint8_t layer = KBD_PrevLayer();
        LOG_I(TAG, "FN: layer prev -> %d", layer);
        KBD_RGB_FlashLayer(layer);
    }
    break;

    case KBD_FN_LAYER_SET:
        if (KBD_Macro_IsRunning())
            KBD_Macro_Cancel();
        LOG_I(TAG, "FN: layer set %d", param);
        KBD_SetCurrentLayer(param);
        KBD_RGB_FlashLayer(param);
        break;

    /* 系统 */
    case KBD_FN_SLEEP:
        LOG_I(TAG, "FN: sleep");
        KBD_Mode_EnterSleep();
        break;

    case KBD_FN_BOOTLOADER:
        LOG_W(TAG, "FN: enter IAP");
        Hal_JumpToBootloader();
        break;

    /* 宏 */
    case KBD_FN_MACRO:
        LOG_I(TAG, "FN: macro %d", param);
        KBD_Macro_Execute(param, KBD_MACRO_TRIG_ONCE);
        break;

    default:
        break;
    }
}
