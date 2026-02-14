/********************************** 最小化测试 *********************************
 * 用于逐步排查崩溃问题
 * 使用方法：在 MounRiver 中把 Main.c 从编译中排除，把 Main_Test.c 加入编译
 ******************************************************************************/

#include "ble_config.h"
#include "ble_hal.h"
#include "hiddev.h"
#include "kbd_mode.h"
#include "key.h"
#include "hal_utils.h"
#include "ws2812.h"
#include "debug.h"
#include "kbd_storage.h"
#include "kbd_command.h"
#include "kbd_rgb.h"
#include "kbd_core.h"

#define TAG "TEST"

/* ============== 选择测试模式（只启用一个） ============== */
#define TEST_UART_ONLY      0   // 测试1: 仅UART日志
#define TEST_BLE_INIT       0   // 测试2: BLE初始化 + 日志
#define TEST_MODE_INIT      0   // 测试3: Mode初始化（不进入主循环）
#define TEST_MAIN_LOOP      1   // 测试4: 完整主循环（无RGB动画）

/* ============== TMOS 内存池 ============== */
__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

#if (defined(BLE_MAC)) && (BLE_MAC == TRUE)
const uint8_t MacAddr[6] = {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};
#endif

/* ============== 主循环 ============== */
__HIGH_CODE
__attribute__((noinline)) void Main_Circulation(void)
{
    while (1) {
        TMOS_SystemProcess();
        KBD_Mode_Process();
        KBD_Core_Process();
        /* 不打印任何日志 */
    }
}

/* ============== 测试入口 ============== */
int main(void)
{
    SetSysClock(CLK_SOURCE_PLL_60MHz);

#if (defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
#endif

#ifdef DEBUG
    Debug_Init();
#endif

    LOG_I(TAG, "=== Test Start ===");

#if TEST_UART_ONLY
    /* 测试1: 仅测试 UART 日志 */
    LOG_I(TAG, "UART test 1");
    DelayMs(100);
    LOG_I(TAG, "UART test 2");
    DelayMs(100);
    LOG_I(TAG, "UART test 3");
    DelayMs(100);
    LOG_I(TAG, "UART OK - looping");
    while (1) {
        LOG_I(TAG, "tick");
        DelayMs(1000);
    }
#endif

#if TEST_BLE_INIT
    /* 测试2: BLE 初始化 */
    LOG_I(TAG, "BLE init...");
    CH59x_BLEInit();
    LOG_I(TAG, "HAL init...");
    HAL_Init();
    LOG_I(TAG, "GAP init...");
    GAPRole_PeripheralInit();
    LOG_I(TAG, "HID init...");
    HidDev_Init();
    LOG_I(TAG, "BLE OK - looping TMOS");
    while (1) {
        TMOS_SystemProcess();
    }
#endif

#if TEST_MODE_INIT
    /* 测试3: Mode 初始化（不进入主循环） */
    LOG_I(TAG, "Full init start");
    
    CH59x_BLEInit();
    HAL_Init();
    GAPRole_PeripheralInit();
    HidDev_Init();
    LOG_I(TAG, "BLE done");
    
    Key_Init();
    LOG_I(TAG, "Key done");
    
    KBD_Storage_Init();
    LOG_I(TAG, "Storage done");
    
    KBD_Command_Init();
    LOG_I(TAG, "Cmd done");
    
    KBD_RGB_Init();
    LOG_I(TAG, "RGB done");
    
    KBD_Core_Init();
    LOG_I(TAG, "Core done");
    
    kbd_work_mode_t mode = KBD_WORK_MODE_USB;
    LOG_I(TAG, "Mode init...");
    
    if (KBD_Mode_Init(mode, KBD_Core_GetCallbacks()) != 0) {
        LOG_E(TAG, "Mode FAIL!");
        while (1);
    }
    
    /* 关键点：Mode_Init 返回后的第一条日志 */
    LOG_I(TAG, "Mode done!");
    LOG_I(TAG, "Test OK");
    
    while (1) {
        TMOS_SystemProcess();
        DelayMs(1000);
        LOG_I(TAG, "idle");
    }
#endif

#if TEST_MAIN_LOOP
    /* 测试4: 完整流程（无日志直接进入循环） */
    CH59x_BLEInit();
    HAL_Init();
    GAPRole_PeripheralInit();
    HidDev_Init();
    Key_Init();
    KBD_Storage_Init();
    KBD_Command_Init();
    KBD_RGB_Init();
    KBD_Core_Init();
    
    kbd_work_mode_t mode = KBD_WORK_MODE_USB;
    KBD_Mode_Init(mode, KBD_Core_GetCallbacks());
    
    /* 不打印任何日志，直接进入主循环 */
    Main_Circulation();
#endif

    return 0;
}
