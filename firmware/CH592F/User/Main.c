/********************************** (C) COPYRIGHT *******************************
 * File Name          : Main.c
 * Author             : MeowKJ
 * Version            : V2.0
 * Date               : 2024/11/07
 * Description        : CH592F 键盘主程序入口
 *******************************************************************************/

/* BLE 相关头文件（包含 CH59x_common.h） */
#include "ble_config.h"
#include "ble_hal.h"
#include "hiddev.h"

/* 键盘核心模块 */
#include "kbd_mode.h"
#include "kbd_core.h"
#include "kbd_storage.h"
#include "kbd_command.h"
#include "kbd_rgb.h"
#include "kbd_log.h"

/* 硬件抽象层 */
#include "key.h"
#include "kbd_battery.h"
#include "hal_utils.h"
#include "ws2812.h"
#include "debug.h"

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
        KBD_Log_Flush();
    }
}

/* ============== 主函数 ============== */
int main(void)
{
    /* 设置系统时钟 */
    SetSysClock(CLK_SOURCE_PLL_60MHz);

#if (defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    /* 低功耗模式：配置所有GPIO为上拉输入 */
    GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
#endif

#ifdef DEBUG
    /* 调试串口初始化 */
    Debug_Init();
#endif

    /* BLE 初始化 */
    CH59x_BLEInit();
    
    /* HAL 初始化 */
    HAL_Init();
    
    /* GAP 角色初始化（外设模式） */
    GAPRole_PeripheralInit();
    
    /* HID 设备初始化 */
    HidDev_Init();
    
    /* 按键驱动初始化 */
    Key_Init();
    
    /* 存储系统初始化 */
    KBD_Storage_Init();
    
    /* 命令处理初始化 */
    KBD_Command_Init();
    
    /* RGB 灯效初始化 */
    KBD_RGB_Init();

    /* 电池检测初始化 */
    KBD_Battery_Init();

    /* HID 日志系统初始化 */
    KBD_Log_Init();

    /* 键盘核心模块初始化 */
    KBD_Core_Init();
    
    /* 模式管理器初始化（默认USB模式） */
    kbd_work_mode_t initial_mode = KBD_WORK_MODE_USB;
    KBD_Mode_Init(initial_mode, KBD_Core_GetCallbacks());

    /* 记录启动事件 */
    KBD_Log_SystemEvent(KBD_LOG_SYS_BOOT);

    /* 进入主循环 */
    Main_Circulation();
    
    return 0;
}
