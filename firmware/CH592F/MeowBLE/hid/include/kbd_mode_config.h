/********************************** (C) COPYRIGHT *******************************
 * File Name          : kbd_mode_config.h
 * Author             : MeowKJ
 * Version            : V2.0
 * Date               : 2024/11/07
 * Description        : 键盘模式管理配置文件（支持 USB/BLE/2.4G 多模）
 *******************************************************************************/

#ifndef __KBD_MODE_CONFIG_H
#define __KBD_MODE_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "CH59x_common.h"

/*============================================================================*/
/* 工作模式定义 */
/*============================================================================*/

typedef enum {
    KBD_WORK_MODE_USB = 0,      /**< USB 有线模式 */
    KBD_WORK_MODE_BLE = 1,      /**< 蓝牙无线模式 */
    /* KBD_WORK_MODE_2G4 = 2,   预留：2.4G 无线模式 */
} kbd_work_mode_t;

/*============================================================================*/
/* 连接状态定义 */
/*============================================================================*/

typedef enum {
    KBD_CONN_DISCONNECTED = 0,  /**< 未连接 */
    KBD_CONN_ADVERTISING  = 1,  /**< 广播中（仅蓝牙） */
    KBD_CONN_CONNECTED    = 2,  /**< 已连接 */
    KBD_CONN_SUSPENDED    = 3,  /**< 挂起 */
} kbd_conn_state_t;

/*============================================================================*/
/* 模式切换配置 */
/*============================================================================*/

/** 长按 FN 键切换模式的阈值（毫秒） */
#define KBD_MODE_SWITCH_HOLD_MS         800

/** 切换到蓝牙后自动开始广播 */
#define KBD_AUTO_START_ADVERTISING      1

/** USB 插入时自动切换到 USB 模式（默认启用，开机即为 USB 模式） */
#define KBD_AUTO_SWITCH_TO_USB_ON_PLUG  1

/*============================================================================*/
/* 蓝牙配置 */
/*============================================================================*/

/** 设备名称 */
#define KBD_BLE_DEVICE_NAME             "MeowKeyboard"
#define KBD_BLE_DEVICE_NAME_LEN         12

/** 广播参数（单位：0.625ms） */
#define KBD_BLE_ADV_INT_MIN             48      /**< 30ms */
#define KBD_BLE_ADV_INT_MAX             80      /**< 50ms */
#define KBD_BLE_ADV_TIMEOUT             60      /**< 60秒超时 */

/** 连接参数（单位：1.25ms） */
#define KBD_BLE_CONN_INT_MIN            8       /**< 10ms */
#define KBD_BLE_CONN_INT_MAX            8       /**< 10ms */
#define KBD_BLE_SLAVE_LATENCY           20      /**< 从机延迟 */
#define KBD_BLE_CONN_TIMEOUT            500     /**< 5秒超时 */

/** 配对模式 */
#define KBD_BLE_PAIRING_MODE            GAPBOND_PAIRING_MODE_WAIT_FOR_REQ
#define KBD_BLE_MITM_MODE               FALSE
#define KBD_BLE_BONDING_MODE            TRUE
#define KBD_BLE_IO_CAPABILITIES         GAPBOND_IO_CAP_NO_INPUT_NO_OUTPUT

/** HID 空闲超时（毫秒），0 表示不超时 */
#define KBD_BLE_HID_IDLE_TIMEOUT        60000

/*============================================================================*/
/* USB 配置 */
/*============================================================================*/

/** USB VID/PID */
#define KBD_USB_VENDOR_ID               0x413D
#define KBD_USB_PRODUCT_ID              0x2107

/** USB 设备字符串 */
#define KBD_USB_MANUFACTURER_STRING     "MeowKJ"
#define KBD_USB_PRODUCT_STRING          "MeowKeyboard HID"

/*============================================================================*/
/* HID 报告配置 */
/*============================================================================*/

/** HID 报告 ID */
#define KBD_HID_RPT_ID_KEYBOARD         0       /**< 键盘报告 ID */
#define KBD_HID_RPT_ID_MOUSE            1       /**< 鼠标报告 ID */
#define KBD_HID_RPT_ID_CONSUMER         2       /**< 多媒体报告 ID */
#define KBD_HID_RPT_ID_LED_OUT          0       /**< LED 输出报告 ID */

/** 报告长度 */
#define KBD_HID_KEYBOARD_REPORT_LEN     8       /**< 键盘报告长度 */
#define KBD_HID_MOUSE_REPORT_LEN        4       /**< 鼠标报告长度 */
#define KBD_HID_CONSUMER_REPORT_LEN     2       /**< 多媒体报告长度 */

/*============================================================================*/
/* 低功耗配置 */
/*============================================================================*/

/** 启用低功耗模式 */
#define KBD_LOW_POWER_ENABLE            1

/** 空闲进入低功耗的时间（毫秒） */
#define KBD_IDLE_SLEEP_TIMEOUT_MS       30000   /**< 30秒无操作进入休眠 */

/** 唤醒方式 */
#define KBD_WAKEUP_BY_KEY               1       /**< 按键唤醒 */
#define KBD_WAKEUP_BY_USB               1       /**< USB 插入唤醒 */

/*============================================================================*/
/* 兼容旧命名（过渡期使用） */
/*============================================================================*/

/* 工作模式 */
#define work_mode_t                     kbd_work_mode_t
#define WORK_MODE_USB                   KBD_WORK_MODE_USB
#define WORK_MODE_BLE                   KBD_WORK_MODE_BLE

/* 连接状态 */
#define conn_state_t                    kbd_conn_state_t
#define CONN_STATE_DISCONNECTED         KBD_CONN_DISCONNECTED
#define CONN_STATE_ADVERTISING          KBD_CONN_ADVERTISING
#define CONN_STATE_CONNECTED            KBD_CONN_CONNECTED
#define CONN_STATE_SUSPENDED            KBD_CONN_SUSPENDED

/* 配置宏 */
#define AUTO_START_ADVERTISING          KBD_AUTO_START_ADVERTISING
#define AUTO_SWITCH_TO_USB_ON_PLUG      KBD_AUTO_SWITCH_TO_USB_ON_PLUG
#define HID_KEYBOARD_REPORT_LEN         KBD_HID_KEYBOARD_REPORT_LEN
#define HID_MOUSE_REPORT_LEN            KBD_HID_MOUSE_REPORT_LEN

/* BLE 配置兼容 */
#define BLE_DEVICE_NAME                 KBD_BLE_DEVICE_NAME
#define BLE_DEVICE_NAME_LEN             KBD_BLE_DEVICE_NAME_LEN
#define BLE_ADV_INT_MIN                 KBD_BLE_ADV_INT_MIN
#define BLE_ADV_INT_MAX                 KBD_BLE_ADV_INT_MAX
#define BLE_ADV_TIMEOUT                 KBD_BLE_ADV_TIMEOUT
#define BLE_CONN_INT_MIN                KBD_BLE_CONN_INT_MIN
#define BLE_CONN_INT_MAX                KBD_BLE_CONN_INT_MAX
#define BLE_SLAVE_LATENCY               KBD_BLE_SLAVE_LATENCY
#define BLE_CONN_TIMEOUT                KBD_BLE_CONN_TIMEOUT
#define BLE_PAIRING_MODE                KBD_BLE_PAIRING_MODE
#define BLE_MITM_MODE                   KBD_BLE_MITM_MODE
#define BLE_BONDING_MODE                KBD_BLE_BONDING_MODE
#define BLE_IO_CAPABILITIES             KBD_BLE_IO_CAPABILITIES
#define BLE_HID_IDLE_TIMEOUT            KBD_BLE_HID_IDLE_TIMEOUT

#ifdef __cplusplus
}
#endif

#endif /* __KBD_MODE_CONFIG_H */
