/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : MeowKJ
 * Version            : V2.0
 * Date               : 2024/11/07
 * Description        : USB/BLE 双模键盘主程序
 *******************************************************************************/

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

#define TAG "MAIN"

/*============================================================================*/
/* 全局变量 */
/*============================================================================*/

/** TMOS 内存池 */
__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

/** MAC 地址（如果使用自定义地址） */
#if (defined(BLE_MAC)) && (BLE_MAC == TRUE)
const uint8_t MacAddr[6] = {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};
#endif

/*============================================================================*/
/* 主循环 */
/*============================================================================*/

/**
 * @brief 主循环
 */
__HIGH_CODE
__attribute__((noinline)) void Main_Circulation(void)
{
    while (1) {
        /* TMOS 系统处理 */
        TMOS_SystemProcess();

        /* 模式管理器处理 */
        KBD_Mode_Process();

        /* 键盘核心处理（按键、FN、RGB） */
        KBD_Core_Process();
    }
}

/*============================================================================*/
/* 主函数 */
/*============================================================================*/

/**
 * @brief 主函数
 */
int main(void)
{
    kbd_work_mode_t initial_mode;

    /* 设置系统时钟 */
    SetSysClock(CLK_SOURCE_PLL_60MHz);

    /* 低功耗配置：所有 GPIO 上拉输入 */
#if (defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
#endif

    /* 初始化调试串口 */
#ifdef DEBUG
    Debug_Init();
#endif

    LOG_I(TAG, "=== USB/BLE Dual Mode Keyboard v2.0 ===");

    /* 初始化 BLE 库 */
    CH59x_BLEInit();

    /* 初始化 HAL */
    HAL_Init();

    /* 初始化 GAP 角色 */
    GAPRole_PeripheralInit();

    /* 初始化 HID 设备 */
    HidDev_Init();

    /* 初始化按键驱动 */
    Key_Init();

    /* 初始化配置存储 */
    KBD_Storage_Init();

    /* 初始化配置命令处理 */
    KBD_Command_Init();

    /* 初始化 RGB 效果引擎（必须在 Mode 之前，因为回调会用到） */
    KBD_RGB_Init();

    /* 初始化键盘核心（必须在 Mode 之前，因为回调会用到） */
    KBD_Core_Init();

    /* 检测初始模式 */
#if KBD_AUTO_SWITCH_TO_USB_ON_PLUG
    initial_mode = KBD_WORK_MODE_USB;
#else
    initial_mode = KBD_WORK_MODE_BLE;
#endif

    /* 初始化模式管理器（USB/BLE） */
    if (KBD_Mode_Init(initial_mode, KBD_Core_GetCallbacks()) != 0) {
        LOG_E(TAG, "Mode init failed!");
        while (1);
    }
    LOG_I(TAG, "Mode OK");

    /* RGB 启动动画 */
    KBD_RGB_Flash(255, 0, 0, 200);
    DelayMs(200);
    KBD_RGB_Flash(0, 255, 0, 200);
    DelayMs(200);
    KBD_RGB_Flash(0, 0, 255, 200);
    DelayMs(200);

    LOG_I(TAG, "Ready");

    /* 进入主循环 */
    Main_Circulation();

    return 0;
}
