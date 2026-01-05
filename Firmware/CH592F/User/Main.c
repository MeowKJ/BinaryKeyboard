/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : Custom Keyboard Library
 * Version            : V1.0
 * Date               : 2024/11/07
 * Description        : USB/BLE 双模键盘主程序示例
 *******************************************************************************/

#include "ble_config.h"
#include "ble_hal.h"
#include "hiddev.h"
#include "dual_mode.h"
#include "key.h"
#include "hal_utils.h"

/* ==================== 全局变量 ==================== */

// TMOS 内存池
__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

// MAC 地址（如果使用自定义地址）
#if(defined(BLE_MAC)) && (BLE_MAC == TRUE)
const uint8_t MacAddr[6] = {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};
#endif

/* ==================== 按键到 HID 码映射 ==================== */

// 5 键映射表（根据需要修改）
static const uint8_t g_key_map[KBD_NUM_KEYS] = {
    HID_KEYBOARD_A,         // K1 -> A
    HID_KEYBOARD_B,         // K2 -> B
    HID_KEYBOARD_C,         // K3 -> C
    HID_KEYBOARD_D,         // K4 -> D
    HID_KEYBOARD_E,         // K5 -> E
};

/* ==================== 回调函数 ==================== */

/**
 * @brief 模式切换回调
 */
static void OnModeChange(work_mode_t new_mode)
{
    PRINT("Mode changed to: %s\n", 
          (new_mode == WORK_MODE_USB) ? "USB" : "BLE");
    
    // LED 指示
#if LED_INDICATOR_ENABLE
    if (new_mode == WORK_MODE_USB) {
        // USB 模式：LED 常亮
        //HalLedSet(HAL_LED_1, HAL_LED_MODE_ON);
    } else {
        // BLE 模式：LED 慢闪（等待连接）
        //HalLedBlink(HAL_LED_1, 0, 50, LED_BLINK_SLOW_MS);
    }
#endif
}

/**
 * @brief 连接状态变化回调
 */
static void OnConnStateChange(conn_state_t state)
{
    PRINT("Connection state: %d\n", state);
    
#if LED_INDICATOR_ENABLE
    switch (state) {
        case CONN_STATE_DISCONNECTED:
           // HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);
            break;
            
        case CONN_STATE_ADVERTISING:
            // 广播中：慢闪
            //HalLedBlink(HAL_LED_1, 0, 50, LED_BLINK_SLOW_MS);
            break;
            
        case CONN_STATE_CONNECTED:
            // 已连接：常亮
            //HalLedSet(HAL_LED_1, HAL_LED_MODE_ON);
            break;
            
        case CONN_STATE_SUSPENDED:
            // 挂起：快闪
            //HalLedBlink(HAL_LED_1, 0, 50, LED_BLINK_FAST_MS);
            break;
    }
#endif
}

/**
 * @brief LED 报告回调
 */
static void OnLedReport(uint8_t leds)
{
    PRINT("LED Report: %02X\n", leds);
    
    // 可以在这里控制物理 LED
    // if (leds & LED_CAPS_LOCK) { ... }
}

/* ==================== 按键处理 ==================== */

/**
 * @brief 处理普通按键事件
 */
static void ProcessKeyEvent(key_event_t *evt)
{
    if (evt->key >= KBD_NUM_KEYS) return;
    
    uint8_t keycode = g_key_map[evt->key];
    
    if (evt->type == KEY_EVT_PRESS) {
        // 按下：发送按键
        uint8_t keys[1] = {keycode};
        DualMode_SendKeyboardReport(0, keys, 1);
        PRINT("Key %d pressed, code=%02X\n", evt->key, keycode);
    }
    else if (evt->type == KEY_EVT_RELEASE) {
        // 释放：清空按键
        DualMode_ReleaseAllKeys();
        PRINT("Key %d released\n", evt->key);
    }
}

/**
 * @brief 处理 FN 按键事件
 */
static void ProcessFnKeyEvent(fnkey_event_t *evt)
{
    if (evt->id == 0) {
        // FN1: 模式切换
        if (evt->type == FNKEY_EVT_LONG) {
            // 长按：切换模式
            PRINT("FN%d long press: switch mode\n", evt->id + 1);
            DualMode_ToggleMode();
        }
        else if (evt->type == FNKEY_EVT_CLICK) {
            // 短按：蓝牙模式下重新广播
            if (DualMode_GetMode() == WORK_MODE_BLE) {
                if (!DualMode_IsConnected()) {
                    PRINT("FN%d click: start advertising\n", evt->id + 1);
                    DualMode_BLE_StartAdvertising();
                }
            }
        }
    }
    else if (evt->id == 1) {
        // FN2: 其他功能
        if (evt->type == FNKEY_EVT_LONG) {
            // 长按 FN2：清除配对
            PRINT("FN2 long press: clear bonds\n");
            DualMode_BLE_ClearBonds();
        }
        else if (evt->type == FNKEY_EVT_CLICK) {
            // 短按 FN2：发送多媒体键（示例：播放/暂停）
            PRINT("FN2 click: play/pause\n");
            DualMode_SendConsumerKey(CONSUMER_PLAY_PAUSE);
        }
    }
}

/**
 * @brief 处理 BOOT 键
 */
static void CheckBootKey(void)
{
    if (BootKey_IsPressed()) {
        PRINT("BOOT key pressed!\n");
        // 跳转到 Bootloader
        Hal_JumpToBootloader();
    }
}

/* ==================== 主函数 ==================== */

/**
 * @brief 主循环
 */
__HIGH_CODE
__attribute__((noinline))
void Main_Circulation(void)
{
    key_event_t key_evt;
    fnkey_event_t fn_evt;
    
    while (1) {
        // TMOS 系统处理
        TMOS_SystemProcess();
        
        // 双模管理器处理
        DualMode_Process();
        
        // 处理普通按键事件
        while (Key_GetEvent(&key_evt)) {
            ProcessKeyEvent(&key_evt);
        }
        
        // 处理 FN 按键事件
        while (FnKey_GetEvent(&fn_evt)) {
            ProcessFnKeyEvent(&fn_evt);
        }
        
        // 检查 BOOT 键
        CheckBootKey();
    }
}

/**
 * @brief 主函数
 */
int main(void)
{
    work_mode_t initial_mode;
    
    // 启用 DCDC
#if(defined(DCDC_ENABLE)) && (DCDC_ENABLE == TRUE)
    PWR_DCDCCfg(ENABLE);
#endif

    // 设置系统时钟
    SetSysClock(CLK_SOURCE_PLL_60MHz);
    
    // 低功耗配置：所有 GPIO 上拉输入
#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
#endif

    // 初始化调试串口
#ifdef DEBUG
    GPIOA_SetBits(bTXD1);
    GPIOA_ModeCfg(bTXD1, GPIO_ModeOut_PP_5mA);
    UART1_DefInit();
#endif

    PRINT("\n\n");
    PRINT("=================================\n");
    PRINT("  USB/BLE Dual Mode Keyboard\n");
    PRINT("  Version: 1.0\n");
    PRINT("=================================\n");
    PRINT("%s\n", VER_LIB);
    
    // 初始化 BLE 库
    CH59x_BLEInit();
    
    // 初始化 HAL
    HAL_Init();
    
    // 初始化 GAP 角色
    GAPRole_PeripheralInit();
    
    // 初始化 HID 设备
    HidDev_Init();
    
    // 初始化按键驱动
    Key_Init();
    
    // 检测 USB 是否插入，决定初始模式
#if AUTO_SWITCH_TO_USB_ON_PLUG
    // 简单检测：如果 USB 已供电则使用 USB 模式
    // 实际应用中需要检测 VBUS 引脚
    initial_mode = WORK_MODE_USB;  // 默认 USB
    
    // 如果检测到没有 USB 连接，切换到 BLE
    // if (!USB_VBUS_Detect()) {
    //     initial_mode = WORK_MODE_BLE;
    // }
#else
    initial_mode = WORK_MODE_BLE;  // 默认蓝牙
#endif
    
    // 初始化双模管理器
    dual_mode_callbacks_t callbacks = {
        .onModeChange = OnModeChange,
        .onConnStateChange = OnConnStateChange,
        .onLedReport = OnLedReport,
    };
    
    if (DualMode_Init(initial_mode, &callbacks) != 0) {
        PRINT("DualMode init failed!\n");
        while (1);
    }
    
    PRINT("Initial mode: %s\n", 
          (initial_mode == WORK_MODE_USB) ? "USB" : "BLE");
    PRINT("System ready!\n\n");
    
    // 进入主循环
    Main_Circulation();
    
    return 0;
}
