/********************************** (C) COPYRIGHT *******************************
 * File Name          : ble_hid_service.h
 * Author             : Custom Keyboard Library
 * Version            : V1.0
 * Date               : 2024/11/07
 * Description        : 蓝牙 HID 服务定义（支持键盘、鼠标、多媒体）
 *******************************************************************************/

#ifndef BLE_HID_SERVICE_H
#define BLE_HID_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ble_config.h"

/* ==================== 报告数量定义 ==================== */

// HID 报告数量（键盘输入、键盘LED输出、鼠标输入、多媒体输入、Boot键盘输入/输出、特性报告、电池）
#define HID_NUM_REPORTS             7

/* ==================== 报告 ID 定义 ==================== */

#define HID_RPT_ID_KEY_IN           1       // Keyboard input report
#define HID_RPT_ID_MOUSE_IN         2       // Mouse input report
#define HID_RPT_ID_CONSUMER_IN      3       // Consumer input report
#define HID_RPT_ID_LED_OUT          1       // Keyboard LED output report
#define HID_RPT_ID_FEATURE          1       // Feature report

/* ==================== HID 特性标志 ==================== */

#define HID_KBD_FLAGS               HID_FLAGS_REMOTE_WAKE

/* ==================== 函数声明 ==================== */

/**
 * @brief 添加 HID 服务
 * @return 状态
 */
bStatus_t HidKbdMouse_AddService(void);

/**
 * @brief 设置 HID 参数
 */
uint8_t HidKbdMouse_SetParameter(uint8_t id, uint8_t type, uint16_t uuid, 
                                  uint8_t len, void *pValue);

/**
 * @brief 获取 HID 参数
 */
uint8_t HidKbdMouse_GetParameter(uint8_t id, uint8_t type, uint16_t uuid, 
                                  uint16_t *pLen, void *pValue);

/* ==================== 外部变量 ==================== */

// HID 协议模式
extern uint8_t hidProtocolMode;

// HID 报告映射长度
extern uint16_t hidReportMapLen;

#ifdef __cplusplus
}
#endif

#endif /* BLE_HID_SERVICE_H */
