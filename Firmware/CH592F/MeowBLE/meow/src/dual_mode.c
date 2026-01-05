/********************************** (C) COPYRIGHT *******************************
 * File Name          : dual_mode.c
 * Author             : Custom Keyboard Library
 * Version            : V1.0
 * Date               : 2024/11/07
 * Description        : USB/BLE 双模切换管理器实现
 *******************************************************************************/

#include "dual_mode.h"
#include "ble_hid.h"
#include "usb_device.h"
#include "usb_hid.h"
#include <string.h>

/* ==================== 私有变量 ==================== */

static work_mode_t g_current_mode = WORK_MODE_USB;
static conn_state_t g_conn_state = CONN_STATE_DISCONNECTED;
static dual_mode_callbacks_t *g_pCallbacks = NULL;
static uint8_t g_keyboard_leds = 0;
static bool g_is_sleeping = false;

/* USB 报告缓冲区 */
static uint8_t g_kbd_report[HID_KEYBOARD_REPORT_LEN];
static uint8_t g_mouse_report[HID_MOUSE_REPORT_LEN];
static uint16_t g_consumer_report;

/* ==================== 私有函数声明 ==================== */

static void DualMode_UpdateConnState(conn_state_t state);
static void DualMode_BLE_StateCallback(gapRole_States_t newState);
static void DualMode_BLE_LedCallback(uint8_t leds);
static int DualMode_USB_Init(void);
static int DualMode_BLE_Init(void);

/* ==================== BLE 回调 ==================== */

static ble_hid_callbacks_t g_ble_callbacks = {
    .onStateChange = DualMode_BLE_StateCallback,
    .onLedReport = DualMode_BLE_LedCallback,
};

/* ==================== 初始化实现 ==================== */

int DualMode_Init(work_mode_t initial_mode, dual_mode_callbacks_t *pCBs)
{
    int ret = 0;
    
    g_pCallbacks = pCBs;
    g_current_mode = initial_mode;
    g_conn_state = CONN_STATE_DISCONNECTED;
    g_is_sleeping = false;
    
    // 清空报告缓冲区
    memset(g_kbd_report, 0, sizeof(g_kbd_report));
    memset(g_mouse_report, 0, sizeof(g_mouse_report));
    g_consumer_report = 0;
    
    DM_PRINT("DualMode: Init mode=%d\n", initial_mode);
    
    // 初始化 BLE（总是初始化，因为需要 TMOS）
    ret = DualMode_BLE_Init();
    if (ret != 0) {
        DM_PRINT("DualMode: BLE init failed %d\n", ret);
        return ret;
    }
    
    // 根据初始模式决定是否初始化 USB
    if (initial_mode == WORK_MODE_USB) {
        ret = DualMode_USB_Init();
        if (ret != 0) {
            DM_PRINT("DualMode: USB init failed %d\n", ret);
            return ret;
        }
        DualMode_UpdateConnState(CONN_STATE_CONNECTED);
    } else {
        // 蓝牙模式，开始广播
#if AUTO_START_ADVERTISING
        BLE_HID_StartAdvertising();
#endif
    }
    
    return 0;
}

void DualMode_Process(void)
{
    // TMOS 系统处理在主循环中调用
    // 这里可以添加其他周期性处理
    
#if AUTO_SWITCH_TO_USB_ON_PLUG
    // 检测 USB 插入状态变化
    static bool last_usb_plugged = false;
    bool usb_plugged = DualMode_USB_IsPlugged();
    
    if (usb_plugged && !last_usb_plugged) {
        // USB 刚插入，自动切换到 USB 模式
        if (g_current_mode != WORK_MODE_USB) {
            DM_PRINT("DualMode: USB plugged, auto switch\n");
            DualMode_SwitchMode(WORK_MODE_USB);
        }
    }
    last_usb_plugged = usb_plugged;
#endif
}

/* ==================== 模式切换实现 ==================== */

int DualMode_SwitchMode(work_mode_t mode)
{
    if (mode == g_current_mode) {
        return 0;  // 已经是目标模式
    }
    
    DM_PRINT("DualMode: Switch %d -> %d\n", g_current_mode, mode);
    
    // 释放当前所有按键
    DualMode_ReleaseAllKeys();
    
    if (mode == WORK_MODE_USB) {
        // 切换到 USB 模式
        
        // 停止蓝牙广播/断开连接
        if (BLE_HID_IsConnected()) {
            BLE_HID_Disconnect();
        } else {
            BLE_HID_StopAdvertising();
        }
        
        // 初始化 USB
        DualMode_USB_Init();
        
        g_current_mode = WORK_MODE_USB;
        DualMode_UpdateConnState(CONN_STATE_CONNECTED);
        
    } else {
        // 切换到蓝牙模式
        
        // TODO: 停止 USB（如果硬件支持动态切换）
        
        g_current_mode = WORK_MODE_BLE;
        
#if AUTO_START_ADVERTISING
        BLE_HID_StartAdvertising();
        DualMode_UpdateConnState(CONN_STATE_ADVERTISING);
#else
        DualMode_UpdateConnState(CONN_STATE_DISCONNECTED);
#endif
    }
    
    // 调用模式切换回调
    if (g_pCallbacks && g_pCallbacks->onModeChange) {
        g_pCallbacks->onModeChange(mode);
    }
    
    return 0;
}

work_mode_t DualMode_GetMode(void)
{
    return g_current_mode;
}

int DualMode_ToggleMode(void)
{
    work_mode_t new_mode = (g_current_mode == WORK_MODE_USB) ? 
                           WORK_MODE_BLE : WORK_MODE_USB;
    return DualMode_SwitchMode(new_mode);
}

/* ==================== 连接状态实现 ==================== */

conn_state_t DualMode_GetConnState(void)
{
    return g_conn_state;
}

bool DualMode_IsConnected(void)
{
    return (g_conn_state == CONN_STATE_CONNECTED);
}

static void DualMode_UpdateConnState(conn_state_t state)
{
    if (state == g_conn_state) {
        return;
    }
    
    g_conn_state = state;
    DM_PRINT("DualMode: ConnState=%d\n", state);
    
    if (g_pCallbacks && g_pCallbacks->onConnStateChange) {
        g_pCallbacks->onConnStateChange(state);
    }
}

/* ==================== 蓝牙控制实现 ==================== */

int DualMode_BLE_StartAdvertising(void)
{
    if (g_current_mode != WORK_MODE_BLE) {
        return -1;
    }
    return BLE_HID_StartAdvertising();
}

int DualMode_BLE_StopAdvertising(void)
{
    return BLE_HID_StopAdvertising();
}

int DualMode_BLE_Disconnect(void)
{
    return BLE_HID_Disconnect();
}

int DualMode_BLE_ClearBonds(void)
{
    return BLE_HID_ClearBonds();
}

uint8_t DualMode_BLE_GetBondCount(void)
{
    return BLE_HID_GetBondCount();
}

/* ==================== USB 控制实现 ==================== */

bool DualMode_USB_IsPlugged(void)
{
    // 检测 USB VBUS 或连接状态
    // 这里需要根据实际硬件实现
    // 简单实现：检查 USB 设备状态
    extern USB_DeviceState_t g_USB_DeviceState;
    return (g_USB_DeviceState >= USB_STATE_POWERED);
}

int DualMode_USB_Wakeup(void)
{
    if (g_current_mode != WORK_MODE_USB) {
        return -1;
    }
    USB_Device_Wakeup();
    return 0;
}

/* ==================== HID 报告发送实现 ==================== */

int DualMode_SendKeyboardReport(uint8_t modifier, uint8_t *keys, uint8_t key_count)
{
    if (!DualMode_IsConnected()) {
        return -1;
    }
    
    // 构建报告
    memset(g_kbd_report, 0, sizeof(g_kbd_report));
    g_kbd_report[0] = modifier;
    g_kbd_report[1] = 0;  // Reserved
    
    uint8_t count = (key_count > 6) ? 6 : key_count;
    if (keys && count > 0) {
        memcpy(&g_kbd_report[2], keys, count);
    }
    
    if (g_current_mode == WORK_MODE_USB) {
        USB_Keyboard_Press(modifier, keys, key_count);
        return 0;
    } else {
        return BLE_HID_SendKeyboardReport(modifier, keys, key_count);
    }
}

int DualMode_SendKeyPress(uint8_t modifier, uint8_t keycode)
{
    uint8_t keys[1] = {keycode};
    int ret;
    
    // 按下
    ret = DualMode_SendKeyboardReport(modifier, keys, 1);
    if (ret != 0) return ret;
    
    // 延时
    mDelaymS(20);
    
    // 释放
    return DualMode_ReleaseAllKeys();
}

int DualMode_ReleaseAllKeys(void)
{
    if (g_current_mode == WORK_MODE_USB) {
        USB_Keyboard_Release();
        return 0;
    } else {
        return BLE_HID_SendKeyboardReport(0, NULL, 0);
    }
}

int DualMode_SendMouseReport(uint8_t buttons, int8_t x, int8_t y, int8_t wheel)
{
    if (!DualMode_IsConnected()) {
        return -1;
    }
    
    if (g_current_mode == WORK_MODE_USB) {
        // 设置按钮状态
        if (buttons != g_mouse_report[0]) {
            USB_Mouse_Press(buttons);
        }
        // 移动
        if (x != 0 || y != 0 || wheel != 0) {
            USB_Mouse_Move(x, y, wheel);
        }
        g_mouse_report[0] = buttons;
        return 0;
    } else {
        return BLE_HID_SendMouseReport(buttons, x, y, wheel);
    }
}

int DualMode_SendMouseClick(uint8_t buttons)
{
    int ret;
    
    // 按下
    ret = DualMode_SendMouseReport(buttons, 0, 0, 0);
    if (ret != 0) return ret;
    
    // 延时
    mDelaymS(50);
    
    // 释放
    return DualMode_SendMouseReport(0, 0, 0, 0);
}

int DualMode_SendConsumerReport(uint16_t key)
{
    if (!DualMode_IsConnected()) {
        return -1;
    }
    
    if (g_current_mode == WORK_MODE_USB) {
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

int DualMode_SendConsumerKey(uint16_t key)
{
    int ret;
    
    // 按下
    ret = DualMode_SendConsumerReport(key);
    if (ret != 0) return ret;
    
    // 延时
    mDelaymS(50);
    
    // 释放
    return DualMode_SendConsumerReport(0);
}

/* ==================== 低功耗实现 ==================== */

void DualMode_EnterSleep(void)
{
    if (g_is_sleeping) return;
    
    g_is_sleeping = true;
    DM_PRINT("DualMode: Enter sleep\n");
    
    // 释放所有按键
    DualMode_ReleaseAllKeys();
    
    // TODO: 关闭 LED，配置唤醒源
}

void DualMode_ExitSleep(void)
{
    if (!g_is_sleeping) return;
    
    g_is_sleeping = false;
    DM_PRINT("DualMode: Exit sleep\n");
    
    // TODO: 恢复外设
}

bool DualMode_IsInSleep(void)
{
    return g_is_sleeping;
}

/* ==================== LED 状态实现 ==================== */

uint8_t DualMode_GetKeyboardLEDs(void)
{
    return g_keyboard_leds;
}

/* ==================== 私有函数实现 ==================== */

static int DualMode_USB_Init(void)
{
    USB_Device_Init();
    USB_Keyboard_Init();
    USB_Mouse_Init();
    USB_Consumer_Init();
    USB_Config_Init();
    return 0;
}

static int DualMode_BLE_Init(void)
{
    // BLE 初始化在 BLE_HID_Init 中完成
    return BLE_HID_Init(&g_ble_callbacks);
}

static void DualMode_BLE_StateCallback(gapRole_States_t newState)
{
    if (g_current_mode != WORK_MODE_BLE) {
        return;
    }
    
    switch (newState & GAPROLE_STATE_ADV_MASK) {
        case GAPROLE_STARTED:
            DM_PRINT("DualMode: BLE Started\n");
            break;
            
        case GAPROLE_ADVERTISING:
            DM_PRINT("DualMode: BLE Advertising\n");
            DualMode_UpdateConnState(CONN_STATE_ADVERTISING);
            break;
            
        case GAPROLE_CONNECTED:
            DM_PRINT("DualMode: BLE Connected\n");
            DualMode_UpdateConnState(CONN_STATE_CONNECTED);
            break;
            
        case GAPROLE_WAITING:
            DM_PRINT("DualMode: BLE Waiting\n");
            DualMode_UpdateConnState(CONN_STATE_DISCONNECTED);
            break;
            
        default:
            break;
    }
}

static void DualMode_BLE_LedCallback(uint8_t leds)
{
    g_keyboard_leds = leds;
    
    if (g_pCallbacks && g_pCallbacks->onLedReport) {
        g_pCallbacks->onLedReport(leds);
    }
}
