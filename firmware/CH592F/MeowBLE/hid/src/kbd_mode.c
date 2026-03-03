/********************************** (C) COPYRIGHT *******************************
 * File Name          : kbd_mode.c
 * Author             : MeowKJ
 * Version            : V2.0
 * Date               : 2024/11/07
 * Description        : 键盘模式管理器实现（支持 USB/BLE/2.4G 多模）
 *******************************************************************************/

#include "kbd_mode.h"
#include "ble_hid.h"
#include "usb_device.h"
#include "usb_hid.h"
#include "kbd_storage.h"
#include "debug.h"
#include <string.h>

#define TAG "MODE"
#define BLE_BOND_CLEAR_SETTLE_MS 50

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
static int KBD_Mode_USB_Init(void);
static int KBD_Mode_BLE_InitInternal(void);

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

    /* 初始化 BLE（总是初始化，因为需要 TMOS） */
    ret = KBD_Mode_BLE_InitInternal();
    if (ret != 0) {
        LOG_E(TAG, "BLE init failed %d", ret);
        return ret;
    }

    /* 初始化 USB 硬件（无论初始模式均先 init，再按模式决定是否 deinit） */
    ret = KBD_Mode_USB_Init();
    if (ret != 0) {
        LOG_E(TAG, "USB init failed %d", ret);
        return ret;
    }

    if (initial_mode == KBD_WORK_MODE_USB) {
        BLE_HID_Disable();
        /* USB 枚举由 KBD_Mode_Process 轮询完成后自动置 CONNECTED */
    } else {
        BLE_HID_Enable();
        /* 蓝牙模式：关闭 USB PHY，消除中断冲突 */
        USB_Device_Deinit();
#if KBD_AUTO_START_ADVERTISING
        BLE_HID_StartAdvertising();
#endif
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

#if KBD_AUTO_SWITCH_TO_USB_ON_PLUG
    /* 检测 USB 插入状态变化 */
    static bool last_usb_plugged = false;
    bool usb_plugged = KBD_Mode_USB_IsPlugged();

    if (usb_plugged && !last_usb_plugged) {
        /* USB 刚插入，自动切换到 USB 模式 */
        if (g_current_mode != KBD_WORK_MODE_USB) {
            LOG_I(TAG, "USB plugged, auto switch");
            KBD_Mode_Switch(KBD_WORK_MODE_USB);
        }
    }
    last_usb_plugged = usb_plugged;
#endif
}

/*============================================================================*/
/* 模式切换实现 */
/*============================================================================*/

int KBD_Mode_Switch(kbd_work_mode_t mode)
{
    if (g_mode_switching) {
        return -1;
    }

    if (mode == g_current_mode) {
        return 0;
    }

    LOG_I(TAG, "switch %d -> %d", g_current_mode, mode);
    g_mode_switching = true;

    /* 释放当前所有按键 */
    KBD_Mode_ReleaseAllKeys();

    if (mode == KBD_WORK_MODE_USB) {
        /* 切换到 USB 模式：先标记模式，屏蔽 BLE 回调中的反向切换 */
        g_current_mode = KBD_WORK_MODE_USB;
        BLE_HID_Disable();

        /* 重新初始化 USB PHY */
        KBD_Mode_USB_Init();
        KBD_SetLastMode(0);
        /* 不立即设 CONNECTED，由 KBD_Mode_Process 轮询 USB 枚举完成后再置位 */
        KBD_Mode_UpdateConnState(KBD_CONN_DISCONNECTED);

    } else {
        /* 切换到蓝牙模式：关闭 USB PHY，消除中断冲突 */
        USB_Device_Deinit();
        BLE_HID_Enable();
        g_current_mode = KBD_WORK_MODE_BLE;
        KBD_SetLastMode(1);

#if KBD_AUTO_START_ADVERTISING
        if (BLE_HID_StartAdvertising() != 0) {
            KBD_Mode_UpdateConnState(KBD_CONN_DISCONNECTED);
        } else {
            KBD_Mode_UpdateConnState(KBD_CONN_ADVERTISING);
        }
#else
        KBD_Mode_UpdateConnState(KBD_CONN_DISCONNECTED);
#endif
    }
    g_mode_switching = false;

    /* 调用模式切换回调 */
    if (g_pCallbacks && g_pCallbacks->onModeChange) {
        g_pCallbacks->onModeChange(mode);
    }

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

    BLE_HID_Enable();
    BLE_HID_StopAdvertising();

    if (BLE_HID_IsConnected()) {
        BLE_HID_Disconnect();
        mDelaymS(BLE_BOND_CLEAR_SETTLE_MS);
    }

    ret = BLE_HID_ClearBonds();
    mDelaymS(BLE_BOND_CLEAR_SETTLE_MS);

#if KBD_AUTO_START_ADVERTISING
    if (BLE_HID_StartAdvertising() == 0) {
        KBD_Mode_UpdateConnState(KBD_CONN_ADVERTISING);
    } else {
        KBD_Mode_UpdateConnState(KBD_CONN_DISCONNECTED);
    }
#else
    KBD_Mode_UpdateConnState(KBD_CONN_DISCONNECTED);
#endif

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

static int KBD_Mode_USB_Init(void)
{
    USB_Device_Init();
    return 0;
}

static int KBD_Mode_BLE_InitInternal(void)
{
    return BLE_HID_Init(&g_ble_callbacks);
}

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
