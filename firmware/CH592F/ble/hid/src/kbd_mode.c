/********************************** (C) COPYRIGHT *******************************
 * File Name          : kbd_mode.c
 * Author             : MeowKJ
 * Version            : V3.0
 * Date               : 2024/11/07
 * Description        : 键盘模式管理器实现
 *                      采用 WCH Application 示例思路：
 *                      模式切换 = 保存模式 + SYS_ResetExecute() 全系统复位
 *                      每种模式只初始化对应的协议栈，避免动态切换冲突
 *******************************************************************************/

#include "kbd_mode.h"
#include "ble_hid.h"
#include "usb_device.h"
#include "usb_hid.h"
#include "kbd_storage.h"
#include "debug.h"
#include <string.h>

#define TAG "MODE"
#define BLE_BOND_CLEAR_SETTLE_MS 100

/*============================================================================*/
/* 私有变量 */
/*============================================================================*/

static kbd_work_mode_t g_current_mode = KBD_WORK_MODE_USB;
static kbd_conn_state_t g_conn_state = KBD_CONN_DISCONNECTED;
static kbd_mode_callbacks_t *g_pCallbacks = NULL;
static uint8_t g_keyboard_leds = 0;
static bool g_is_sleeping = false;
static bool g_mode_switching = false;

/** 报告缓冲区 */
static uint8_t g_kbd_report[KBD_HID_KEYBOARD_REPORT_LEN];
static uint8_t g_mouse_report[KBD_HID_MOUSE_REPORT_LEN];
static uint16_t g_consumer_report;

/*============================================================================*/
/* 私有函数声明 */
/*============================================================================*/

static void KBD_Mode_UpdateConnState(kbd_conn_state_t state);
static void KBD_Mode_BLE_StateCallback(gapRole_States_t newState);
static void KBD_Mode_BLE_LedCallback(uint8_t leds);

/*============================================================================*/
/* BLE 回调 */
/*============================================================================*/

static ble_hid_callbacks_t g_ble_callbacks = {
    .onStateChange = KBD_Mode_BLE_StateCallback,
    .onLedReport = KBD_Mode_BLE_LedCallback,
};

/*============================================================================*/
/* 初始化实现 */
/*============================================================================*/

int KBD_Mode_Init(kbd_work_mode_t initial_mode, kbd_mode_callbacks_t *pCBs)
{
    int ret = 0;

    g_pCallbacks = pCBs;
    g_current_mode = initial_mode;
    g_conn_state = KBD_CONN_DISCONNECTED;
    g_is_sleeping = false;
    g_mode_switching = false;

    /* 清空报告缓冲区 */
    memset(g_kbd_report, 0, sizeof(g_kbd_report));
    memset(g_mouse_report, 0, sizeof(g_mouse_report));
    g_consumer_report = 0;

    LOG_I(TAG, "init mode=%d", initial_mode);

    /*
     * WCH Application 示例思路：每种模式只初始化对应协议栈
     * - USB 模式：仅 USB，不初始化 BLE HID
     * - BLE 模式：仅 BLE HID，不初始化 USB
     * 模式切换通过 SYS_ResetExecute() 全系统复位实现
     */
    if (initial_mode == KBD_WORK_MODE_BLE) {
        /* BLE 模式：初始化 BLE HID，广播由 GAPROLE_STARTED 回调自动触发 */
        ret = BLE_HID_Init(&g_ble_callbacks);
        if (ret != 0) {
            LOG_E(TAG, "BLE init failed %d", ret);
            return ret;
        }
    } else {
        /* USB 模式：仅初始化 USB */
        USB_Device_Init();
    }

    return 0;
}

void KBD_Mode_Process(void)
{
    /* USB 模式：轮询枚举状态，枚举完成后才置 CONNECTED */
    if (g_current_mode == KBD_WORK_MODE_USB) {
        bool usb_configured = (g_USB_DeviceState == USB_STATE_CONFIGURED);
        bool is_connected   = (g_conn_state == KBD_CONN_CONNECTED);

        if (usb_configured && !is_connected) {
            KBD_Mode_UpdateConnState(KBD_CONN_CONNECTED);
        } else if (!usb_configured && is_connected) {
            KBD_Mode_UpdateConnState(KBD_CONN_DISCONNECTED);
        }
    }
}

/*============================================================================*/
/* 模式切换实现（SYS_ResetExecute 方式）                                      */
/*============================================================================*/

int KBD_Mode_Switch(kbd_work_mode_t mode)
{
    if (g_mode_switching) {
        return -1;
    }

    if (mode == g_current_mode) {
        return 0;
    }

    LOG_I(TAG, "switch %d -> %d (reset)", g_current_mode, mode);
    g_mode_switching = true;

    /* 释放当前所有按键 */
    KBD_Mode_ReleaseAllKeys();

    /* 保存目标模式到 DataFlash */
    KBD_SetLastMode(mode == KBD_WORK_MODE_BLE ? 1 : 0);
    KBD_Storage_FlushRuntime(); /* 立即落盘，确保复位前写入完成 */

    /* 等待 Flash 写入和外设稳定 */
    mDelaymS(10);

    /* 全系统复位，下次启动自动进入新模式 */
    SYS_ResetExecute();

    /* 不会到达这里 */
    return 0;
}

kbd_work_mode_t KBD_Mode_Get(void)
{
    return g_current_mode;
}

int KBD_Mode_Toggle(void)
{
    kbd_work_mode_t new_mode = (g_current_mode == KBD_WORK_MODE_USB) ?
                               KBD_WORK_MODE_BLE : KBD_WORK_MODE_USB;
    return KBD_Mode_Switch(new_mode);
}

/*============================================================================*/
/* 连接状态实现 */
/*============================================================================*/

kbd_conn_state_t KBD_Mode_GetConnState(void)
{
    return g_conn_state;
}

bool KBD_Mode_IsConnected(void)
{
    return (g_conn_state == KBD_CONN_CONNECTED);
}

static void KBD_Mode_UpdateConnState(kbd_conn_state_t state)
{
    if (state == g_conn_state) {
        return;
    }

    g_conn_state = state;
    LOG_D(TAG, "conn state=%d", state);

    if (g_pCallbacks && g_pCallbacks->onConnStateChange) {
        g_pCallbacks->onConnStateChange(state);
    }
}

/*============================================================================*/
/* 蓝牙控制实现 */
/*============================================================================*/

int KBD_Mode_BLE_StartAdvertising(void)
{
    if (g_current_mode != KBD_WORK_MODE_BLE) {
        return -1;
    }
    return BLE_HID_StartAdvertising();
}

int KBD_Mode_BLE_StopAdvertising(void)
{
    return BLE_HID_StopAdvertising();
}

int KBD_Mode_BLE_Disconnect(void)
{
    return BLE_HID_Disconnect();
}

int KBD_Mode_BLE_ClearBonds(void)
{
    int ret;

    if (g_mode_switching) {
        return -1;
    }

    /* 非 BLE 模式下不执行任何清配对操作 */
    if (g_current_mode != KBD_WORK_MODE_BLE) {
        return -1;
    }

    BLE_HID_StopAdvertising();

    ret = BLE_HID_ClearBonds();
    if (ret != 0) {
        KBD_Mode_UpdateConnState(KBD_CONN_DISCONNECTED);
        return ret;
    }

    /*
     * 清除 bond 后直接复位回 BLE 模式。
     * 这样可以避开当前连接/广播状态机残留，并让 BondMgr/白名单从干净状态重新初始化。
     */
    KBD_Mode_UpdateConnState(KBD_CONN_DISCONNECTED);
    KBD_SetLastMode(1);
    KBD_Storage_FlushRuntime();
    mDelaymS(BLE_BOND_CLEAR_SETTLE_MS);
    SYS_ResetExecute();

    return ret;
}

uint8_t KBD_Mode_BLE_GetBondCount(void)
{
    return BLE_HID_GetBondCount();
}

/*============================================================================*/
/* USB 控制实现 */
/*============================================================================*/

bool KBD_Mode_USB_IsPlugged(void)
{
    extern USB_DeviceState_t g_USB_DeviceState;
    return (g_USB_DeviceState >= USB_STATE_POWERED);
}

int KBD_Mode_USB_Wakeup(void)
{
    if (g_current_mode != KBD_WORK_MODE_USB) {
        return -1;
    }
    USB_Device_Wakeup();
    return 0;
}

/*============================================================================*/
/* HID 报告发送实现 */
/*============================================================================*/

int KBD_Mode_SendKeyboardReport(uint8_t modifier, uint8_t *keys, uint8_t key_count)
{
    if (!KBD_Mode_IsConnected()) {
        return -1;
    }

    /* 构建报告 */
    memset(g_kbd_report, 0, sizeof(g_kbd_report));
    g_kbd_report[0] = modifier;
    g_kbd_report[1] = 0;  /* Reserved */

    uint8_t count = (key_count > 6) ? 6 : key_count;
    if (keys && count > 0) {
        memcpy(&g_kbd_report[2], keys, count);
    }

    if (g_current_mode == KBD_WORK_MODE_USB) {
        USB_Keyboard_Press(modifier, keys, key_count);
        return 0;
    } else {
        return BLE_HID_SendKeyboardReport(modifier, keys, key_count);
    }
}

int KBD_Mode_SendKeyPress(uint8_t modifier, uint8_t keycode)
{
    uint8_t keys[1] = {keycode};
    int ret;

    ret = KBD_Mode_SendKeyboardReport(modifier, keys, 1);
    if (ret != 0) return ret;

    mDelaymS(20);

    return KBD_Mode_ReleaseAllKeys();
}

int KBD_Mode_ReleaseAllKeys(void)
{
    if (g_current_mode == KBD_WORK_MODE_USB) {
        USB_Keyboard_Release();
        return 0;
    } else {
        return BLE_HID_SendKeyboardReport(0, NULL, 0);
    }
}

int KBD_Mode_SendMouseReport(uint8_t buttons, int8_t x, int8_t y, int8_t wheel)
{
    if (!KBD_Mode_IsConnected()) {
        return -1;
    }

    if (g_current_mode == KBD_WORK_MODE_USB) {
        if (buttons != g_mouse_report[0]) {
            USB_Mouse_Press(buttons);
        }
        if (x != 0 || y != 0 || wheel != 0) {
            USB_Mouse_Move(x, y, wheel);
        }
        g_mouse_report[0] = buttons;
        return 0;
    } else {
        return BLE_HID_SendMouseReport(buttons, x, y, wheel);
    }
}

int KBD_Mode_SendMouseClick(uint8_t buttons)
{
    int ret;

    ret = KBD_Mode_SendMouseReport(buttons, 0, 0, 0);
    if (ret != 0) return ret;

    mDelaymS(50);

    return KBD_Mode_SendMouseReport(0, 0, 0, 0);
}

int KBD_Mode_SendConsumerReport(uint16_t key)
{
    if (!KBD_Mode_IsConnected()) {
        return -1;
    }

    if (g_current_mode == KBD_WORK_MODE_USB) {
        if (key != 0) {
            USB_Consumer_Press(key);
        } else {
            USB_Consumer_Release();
        }
        return 0;
    } else {
        return BLE_HID_SendConsumerReport(key);
    }
}

int KBD_Mode_SendConsumerKey(uint16_t key)
{
    int ret;

    ret = KBD_Mode_SendConsumerReport(key);
    if (ret != 0) return ret;

    mDelaymS(50);

    return KBD_Mode_SendConsumerReport(0);
}

/*============================================================================*/
/* 低功耗实现 */
/*============================================================================*/

void KBD_Mode_EnterSleep(void)
{
    if (g_is_sleeping) return;

    g_is_sleeping = true;
    LOG_I(TAG, "enter sleep");

    KBD_Mode_ReleaseAllKeys();
    KBD_Storage_FlushRuntime();  /* 强制落盘 runtime 热数据（layer/mode），防断电丢失 */
    /* TODO: 关闭 LED，配置唤醒源 */
}

void KBD_Mode_ExitSleep(void)
{
    if (!g_is_sleeping) return;

    g_is_sleeping = false;
    LOG_I(TAG, "exit sleep");
    /* TODO: 恢复外设 */
}

bool KBD_Mode_IsInSleep(void)
{
    return g_is_sleeping;
}

/*============================================================================*/
/* LED 状态实现 */
/*============================================================================*/

uint8_t KBD_Mode_GetKeyboardLEDs(void)
{
    return g_keyboard_leds;
}

/*============================================================================*/
/* 私有函数实现 */
/*============================================================================*/

static void KBD_Mode_BLE_StateCallback(gapRole_States_t newState)
{
    uint8_t state = (newState & GAPROLE_STATE_ADV_MASK);

    if (g_mode_switching || g_current_mode != KBD_WORK_MODE_BLE) {
        return;
    }

    if (state == GAPROLE_CONNECTED || state == GAPROLE_CONNECTED_ADV) {
        LOG_I(TAG, "BLE connected");
        KBD_Storage_DeferRuntimeSave(5000); /* 推迟 Flash 写入，避免打断 BLE 配对握手 */
        KBD_Mode_UpdateConnState(KBD_CONN_CONNECTED);
        return;
    }

    switch (state) {
        case GAPROLE_STARTED:
            LOG_D(TAG, "BLE started");
            break;

        case GAPROLE_ADVERTISING:
            LOG_I(TAG, "BLE advertising");
            KBD_Mode_UpdateConnState(KBD_CONN_ADVERTISING);
            break;

        case GAPROLE_WAITING:
            LOG_I(TAG, "BLE waiting");
            KBD_Mode_UpdateConnState(KBD_CONN_DISCONNECTED);
            break;

        default:
            break;
    }
}

static void KBD_Mode_BLE_LedCallback(uint8_t leds)
{
    g_keyboard_leds = leds;

    if (g_pCallbacks && g_pCallbacks->onLedReport) {
        g_pCallbacks->onLedReport(leds);
    }
}
