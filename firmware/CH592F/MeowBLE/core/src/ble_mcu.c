/********************************** (C) COPYRIGHT *******************************
 * File Name          : MCU.c
 * Author             : WCH
 * Version            : V1.2
 * Date               : 2022/01/18
 * Description        : 硬件任务处理函数及BLE和硬件初始化
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/******************************************************************************/
/* 头文件包含 */
#include "ble_hal.h"
#include "kbd_log.h"

tmosTaskID halTaskID;
uint32_t g_LLE_IRQLibHandlerLocation;

/*******************************************************************************
 * @fn      Lib_Calibration_LSI
 *
 * @brief   内部32k校准
 *
 * @param   None.
 *
 * @return  None.
 */
void Lib_Calibration_LSI (void) {
    Calibration_LSI (Level_64);
}

#if (defined(BLE_SNV)) && (BLE_SNV == TRUE)
/* SNV write trace opcodes for UI DevLog */
#define BLE_DIAG_OP_SNV_WRITE_ENTER   0xA0
#define BLE_DIAG_OP_SNV_READ_DONE     0xA1
#define BLE_DIAG_OP_SNV_MERGE_DONE    0xA2
#define BLE_DIAG_OP_SNV_ERASE_DONE    0xA3
#define BLE_DIAG_OP_SNV_WRITE_DONE    0xA4
#define BLE_DIAG_OP_SNV_WRITE_EXIT    0xA5
#define BLE_DIAG_OP_SNV_WRITE_REJECT  0xA9
#define BLE_SNV_LOG_VERBOSE           0
/* 在 SNV 写回调内不打 BLE 诊断日志，减轻配对期负载，降低配对完成后卡死风险（见 FAQ#6） */
#define BLE_SNV_LOG_IN_WRITE          1

/* Avoid stack pressure in BLE callback context. */
__attribute__ ((aligned (4))) static uint8_t s_snv_flash_buf[BLE_SNV_BLOCK * BLE_SNV_NUM];

static inline void BLE_MCU_LogSnv (uint8_t opcode, uint8_t reason, uint16_t value) {
#if BLE_SNV_LOG_IN_WRITE
    KBD_Log_BleDiagEvent (2, opcode, reason, value);
#else
    (void)opcode;
    (void)reason;
    (void)value;
#endif
}

static inline void BLE_MCU_LogSnvVerbose (uint8_t opcode, uint8_t reason, uint16_t value) {
#if BLE_SNV_LOG_VERBOSE
    KBD_Log_BleDiagEvent (2, opcode, reason, value);
#else
    (void)opcode;
    (void)reason;
    (void)value;
#endif
}

/*******************************************************************************
 * @fn      Lib_Read_Flash
 *
 * @brief   Callback function used for BLE lib.
 *
 * @param   addr - Read start address
 * @param   num - Number of units to read (unit: 4 bytes)
 * @param   pBuf - Buffer to store read data
 *
 * @return  None.
 */
uint32_t Lib_Read_Flash (uint32_t addr, uint32_t num, uint32_t *pBuf) {
    EEPROM_READ (addr, pBuf, num * 4);
    return 0;
}

/*******************************************************************************
 * @fn      Lib_Write_Flash_592A
 *
 * @brief   Callback function used for BLE lib.
 *
 * @param   addr - Write start address
 * @param   num - Number of units to write (unit: 4 bytes)
 * @param   pBuf - Buffer with data to be written
 *
 * @return  None.
 */
static uint32_t Lib_Write_Flash_RMW (uint32_t addr, uint32_t num, uint32_t *pBuf) {
    uint32_t snv_base = BLE_SNV_ADDR;
    uint32_t snv_size = BLE_SNV_BLOCK * BLE_SNV_NUM;
    uint32_t write_len = num * 4;
    uint32_t erase_base;
    uint32_t erase_len;

    BLE_MCU_LogSnv (BLE_DIAG_OP_SNV_WRITE_ENTER, (uint8_t)num, (uint16_t)(addr - snv_base));

    if ((addr < snv_base) || (write_len > snv_size) ||
        ((addr - snv_base) > (snv_size - write_len))) {
        BLE_MCU_LogSnv (BLE_DIAG_OP_SNV_WRITE_REJECT, 1, (uint16_t)write_len);
        return 1;
    }

    EEPROM_READ (snv_base, s_snv_flash_buf, snv_size);
    BLE_MCU_LogSnvVerbose (BLE_DIAG_OP_SNV_READ_DONE, 0, (uint16_t)snv_size);

    tmos_memcpy (&s_snv_flash_buf[addr - snv_base], (uint8_t *)pBuf, write_len);
    BLE_MCU_LogSnvVerbose (BLE_DIAG_OP_SNV_MERGE_DONE, 0, (uint16_t)write_len);

    erase_base = snv_base & (~(EEPROM_BLOCK_SIZE - 1));
    erase_len = ((snv_size + (snv_base - erase_base) + EEPROM_BLOCK_SIZE - 1) / EEPROM_BLOCK_SIZE) * EEPROM_BLOCK_SIZE;
    EEPROM_ERASE (erase_base, erase_len);
    BLE_MCU_LogSnvVerbose (BLE_DIAG_OP_SNV_ERASE_DONE, 0, (uint16_t)erase_len);

    EEPROM_WRITE (snv_base, s_snv_flash_buf, snv_size);
    BLE_MCU_LogSnvVerbose (BLE_DIAG_OP_SNV_WRITE_DONE, 0, (uint16_t)snv_size);
    BLE_MCU_LogSnv (BLE_DIAG_OP_SNV_WRITE_EXIT, 0, 0);

    return 0;
}

void Lib_Write_Flash_592A (uint32_t addr, uint32_t num, uint32_t *pBuf) {
    (void)Lib_Write_Flash_RMW (addr, num, pBuf);
}

/*******************************************************************************
 * @fn      Lib_Write_Flash
 *
 * @brief   Callback function used for BLE lib.
 *
 * @param   addr - Write start address
 * @param   num - Number of units to write (unit: 4 bytes)
 * @param   pBuf - Buffer with data to be written
 *
 * @return  None.
 */
uint32_t Lib_Write_Flash (uint32_t addr, uint32_t num, uint32_t *pBuf) {
    if (((*(uint32_t *)ROM_CFG_VERISON) & 0xFF) == DEF_CHIP_ID_CH592A) {
        return Lib_Write_Flash_RMW (addr, num, pBuf);
    }
    EEPROM_ERASE (addr, num * 4);
    EEPROM_WRITE (addr, pBuf, num * 4);
    return 0;
}
#endif

/*******************************************************************************
 * @fn      CH59x_BLEInit
 *
 * @brief   BLE 库初始化
 *
 * @param   None.
 *
 * @return  None.
 */
void CH59x_BLEInit (void) {
    uint8_t i;
    bleConfig_t cfg;
    if (tmos_memcmp (VER_LIB, VER_FILE, strlen (VER_FILE)) == FALSE) {
        PRINT ("head file error...\n");
        while (1);
    }

    SysTick_Config (SysTick_LOAD_RELOAD_Msk);  // 配置SysTick并打开中断
    PFIC_DisableIRQ (SysTick_IRQn);

    g_LLE_IRQLibHandlerLocation = (uint32_t)LLE_IRQLibHandler;
    PFIC_SetPriority (BLEL_IRQn, 0xF0);
    tmos_memset (&cfg, 0, sizeof (bleConfig_t));
    cfg.MEMAddr = (uint32_t)MEM_BUF;
    cfg.MEMLen = (uint32_t)BLE_MEMHEAP_SIZE;
    cfg.BufMaxLen = (uint32_t)BLE_BUFF_MAX_LEN;
    cfg.BufNumber = (uint32_t)BLE_BUFF_NUM;
    cfg.TxNumEvent = (uint32_t)BLE_TX_NUM_EVENT;
    cfg.TxPower = (uint32_t)BLE_TX_POWER;
#if (defined(BLE_SNV)) && (BLE_SNV == TRUE)
#if (defined(BLE_SNV_PERSIST_ENABLE)) && (BLE_SNV_PERSIST_ENABLE == TRUE)
    if ((BLE_SNV_ADDR + BLE_SNV_BLOCK * BLE_SNV_NUM) > (0x78000 - FLASH_ROM_MAX_SIZE)) {
        PRINT ("SNV config error...\n");
        while (1);
    }
    cfg.SNVAddr = (uint32_t)BLE_SNV_ADDR;
    cfg.SNVBlock = (uint32_t)BLE_SNV_BLOCK;
    cfg.SNVNum = (uint32_t)BLE_SNV_NUM;
    cfg.readFlashCB = Lib_Read_Flash;
    cfg.writeFlashCB = Lib_Write_Flash;
#else
    /* Disable bond persistence to bypass deterministic freeze after bond save. */
    cfg.SNVAddr = 0;
    cfg.SNVBlock = 0;
    cfg.SNVNum = 0;
    cfg.readFlashCB = 0;
    cfg.writeFlashCB = 0;
#endif
#endif
    cfg.ConnectNumber = (PERIPHERAL_MAX_CONNECTION & 3) | (CENTRAL_MAX_CONNECTION << 2);
    cfg.srandCB = SYS_GetSysTickCnt;
#if (defined TEM_SAMPLE) && (TEM_SAMPLE == TRUE)
    cfg.tsCB = HAL_GetInterTempValue;  // 根据温度变化校准RF和内部RC( 大于7摄氏度 )
#if (CLK_OSC32K)
    cfg.rcCB = Lib_Calibration_LSI;    // 内部32K时钟校准
#endif
#endif
#if (defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    cfg.idleCB = CH59x_LowPower;  // 启用睡眠
#endif
#if (defined(BLE_MAC)) && (BLE_MAC == TRUE)
    for (i = 0; i < 6; i++) {
        cfg.MacAddr[i] = MacAddr[5 - i];
    }
#else
    {
        uint8_t MacAddr[6];
        GetMACAddress (MacAddr);
        for (i = 0; i < 6; i++) {
            cfg.MacAddr[i] = MacAddr[i];  // 使用芯片mac地址
        }
    }
#endif
    if (!cfg.MEMAddr || cfg.MEMLen < 4 * 1024) {
        while (1);
    }
    i = BLE_LibInit (&cfg);
    if (i) {
        PRINT ("LIB init error code: %x ...\n", i);
        while (1);
    }
}

/*******************************************************************************
 * @fn      HAL_ProcessEvent
 *
 * @brief   硬件层事务处理
 *
 * @param   task_id - The TMOS assigned task ID.
 * @param   events  - events to process.  This is a bit map and can
 *                      contain more than one event.
 *
 * @return  events.
 */
tmosEvents HAL_ProcessEvent (tmosTaskID task_id, tmosEvents events) {
    uint8_t *msgPtr;

    if (events & SYS_EVENT_MSG) {  // 处理HAL层消息，调用tmos_msg_receive读取消息，处理完成后删除消息。
        msgPtr = tmos_msg_receive (task_id);
        if (msgPtr) {
            /* De-allocate */
            tmos_msg_deallocate (msgPtr);
        }
        return events ^ SYS_EVENT_MSG;
    }
    if (events & LED_BLINK_EVENT) {
#if (defined HAL_LED) && (HAL_LED == TRUE)
        HalLedUpdate();
#endif  // HAL_LED
        return events ^ LED_BLINK_EVENT;
    }
    if (events & HAL_KEY_EVENT) {
#if (defined HAL_KEY) && (HAL_KEY == TRUE)
        HAL_KeyPoll(); /* Check for keys */
        tmos_start_task (halTaskID, HAL_KEY_EVENT, MS1_TO_SYSTEM_TIME (100));
#endif
        return events ^ HAL_KEY_EVENT;
    }
    if (events & HAL_REG_INIT_EVENT) {

#if (defined BLE_CALIBRATION_ENABLE) && (BLE_CALIBRATION_ENABLE == TRUE)  // 校准任务，单次校准耗时小于10ms
        BLE_RegInit();                                                    // 校准RF
                                                                          // #if (CLK_OSC32K)
        Lib_Calibration_LSI();                                            // 校准内部RC
                                                                          // #else
                                                                          //         uint8_t x32Kpw;
                                                                          //         x32Kpw = (R8_XT32K_TUNE & 0xfc) | 0x01;
                                                                          //         sys_safe_access_enable();
                                                                          //         R8_XT32K_TUNE = x32Kpw;  // LSE驱动电流降低到额定电流
                                                                          //         sys_safe_access_disable();
                                                                          // #endif
        tmos_start_task (halTaskID, HAL_REG_INIT_EVENT, MS1_TO_SYSTEM_TIME (BLE_CALIBRATION_PERIOD));
        return events ^ HAL_REG_INIT_EVENT;
#endif
    }
    if (events & HAL_TEST_EVENT) {
        PRINT ("* \n");
        tmos_start_task (halTaskID, HAL_TEST_EVENT, MS1_TO_SYSTEM_TIME (1000));
        return events ^ HAL_TEST_EVENT;
    }
    return 0;
}

/*******************************************************************************
 * @fn      HAL_Init
 *
 * @brief   硬件初始化
 *
 * @param   None.
 *
 * @return  None.
 */
void HAL_Init() {
    halTaskID = TMOS_ProcessEventRegister (HAL_ProcessEvent);
    HAL_TimeInit();
#if (defined HAL_SLEEP) && (HAL_SLEEP == TRUE)
    HAL_SleepInit();
#endif
#if (defined HAL_LED) && (HAL_LED == TRUE)
    HAL_LedInit();
#endif
#if (defined HAL_KEY) && (HAL_KEY == TRUE)
    HAL_KeyInit();
#endif
#if (defined BLE_CALIBRATION_ENABLE) && (BLE_CALIBRATION_ENABLE == TRUE)
    tmos_start_task (halTaskID, HAL_REG_INIT_EVENT, 800);  // 添加校准任务，500ms启动，单次校准耗时小于10ms
#endif
    //  tmos_start_task( halTaskID, HAL_TEST_EVENT, 1600 );    // 添加一个测试任务
}

/*******************************************************************************
 * @fn      HAL_GetInterTempValue
 *
 * @brief   获取内部温感采样值，如果使用了ADC中断采样，需在此函数中暂时屏蔽中断.
 *
 * @return  内部温感采样值.
 */
uint16_t HAL_GetInterTempValue (void) {
    uint8_t sensor, channel, config, tkey_cfg;
    uint16_t adc_data;

    tkey_cfg = R8_TKEY_CFG;
    sensor = R8_TEM_SENSOR;
    channel = R8_ADC_CHANNEL;
    config = R8_ADC_CFG;
    ADC_InterTSSampInit();
    R8_ADC_CONVERT |= RB_ADC_START;
    while (R8_ADC_CONVERT & RB_ADC_START);
    adc_data = R16_ADC_DATA;
    R8_TEM_SENSOR = sensor;
    R8_ADC_CHANNEL = channel;
    R8_ADC_CFG = config;
    R8_TKEY_CFG = tkey_cfg;
    return (adc_data);
}

/******************************** endfile @ mcu ******************************/
