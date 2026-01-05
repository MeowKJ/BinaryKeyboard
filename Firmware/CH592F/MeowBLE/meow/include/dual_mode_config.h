/********************************** (C) COPYRIGHT *******************************
 * File Name          : dual_mode_config.h
 * Author             : Custom Keyboard Library
 * Version            : V1.0
 * Date               : 2024/11/07
 * Description        : USB/BLE 双模键盘配置文件
 *******************************************************************************/

#ifndef DUAL_MODE_CONFIG_H
#define DUAL_MODE_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "CH59x_common.h"

/* ==================== 工作模式定义 ==================== */
typedef enum {
    WORK_MODE_USB = 0,      // USB 有线模式
    WORK_MODE_BLE = 1,      // 蓝牙无线模式
} work_mode_t;

/* ==================== 连接状态定义 ==================== */
typedef enum {
    CONN_STATE_DISCONNECTED = 0,    // 未连接
    CONN_STATE_ADVERTISING  = 1,    // 广播中（仅蓝牙）
    CONN_STATE_CONNECTED    = 2,    // 已连接
    CONN_STATE_SUSPENDED    = 3,    // 挂起
} conn_state_t;

/* ==================== 模式切换配置 ==================== */

// 长按 FN 键切换模式的阈值（毫秒）
#define MODE_SWITCH_HOLD_MS         800

// 切换到蓝牙后自动开始广播
#define AUTO_START_ADVERTISING      1

// USB 插入时自动切换到 USB 模式
#define AUTO_SWITCH_TO_USB_ON_PLUG  0

/* ==================== 蓝牙配置 ==================== */

// 设备名称
#define BLE_DEVICE_NAME             "DualMode Keyboard"
#define BLE_DEVICE_NAME_LEN         17

// 广播参数（单位：0.625ms）
#define BLE_ADV_INT_MIN             48      // 30ms
#define BLE_ADV_INT_MAX             80      // 50ms
#define BLE_ADV_TIMEOUT             60      // 60秒超时

// 连接参数（单位：1.25ms）
#define BLE_CONN_INT_MIN            8       // 10ms
#define BLE_CONN_INT_MAX            8       // 10ms
#define BLE_SLAVE_LATENCY           20      // 从机延迟
#define BLE_CONN_TIMEOUT            500     // 5秒超时

// 配对模式
#define BLE_PAIRING_MODE            GAPBOND_PAIRING_MODE_WAIT_FOR_REQ
#define BLE_MITM_MODE               FALSE
#define BLE_BONDING_MODE            TRUE
#define BLE_IO_CAPABILITIES         GAPBOND_IO_CAP_NO_INPUT_NO_OUTPUT

// HID 空闲超时（毫秒），0 表示不超时
#define BLE_HID_IDLE_TIMEOUT        60000

/* ==================== USB 配置 ==================== */

// USB VID/PID
#define USB_VENDOR_ID               0x413D
#define USB_PRODUCT_ID              0x2107

// USB 设备字符串
#define USB_MANUFACTURER_STRING     "Custom Device"
#define USB_PRODUCT_STRING          "DualMode HID Keyboard"

/* ==================== HID 报告配置 ==================== */

// HID 报告 ID
#define HID_RPT_ID_KEYBOARD         0       // 键盘报告 ID
#define HID_RPT_ID_MOUSE            1       // 鼠标报告 ID
#define HID_RPT_ID_CONSUMER         2       // 多媒体报告 ID
#define HID_RPT_ID_LED_OUT          0       // LED 输出报告 ID

// 报告长度
#define HID_KEYBOARD_REPORT_LEN     8       // 键盘报告长度
#define HID_MOUSE_REPORT_LEN        4       // 鼠标报告长度
#define HID_CONSUMER_REPORT_LEN     2       // 多媒体报告长度

/* ==================== LED 指示配置 ==================== */

// LED 指示功能开关
#define LED_INDICATOR_ENABLE        1

// LED 闪烁模式
#define LED_BLINK_ADVERTISING       1       // 广播中闪烁
#define LED_BLINK_CONNECTED         2       // 连接成功快闪
#define LED_SOLID_USB               3       // USB模式常亮
#define LED_BLINK_ERROR             4       // 错误快闪

// LED 闪烁参数
#define LED_BLINK_FAST_MS           100     // 快闪周期
#define LED_BLINK_SLOW_MS           1000    // 慢闪周期

/* ==================== 低功耗配置 ==================== */

// 启用低功耗模式
#define LOW_POWER_ENABLE            1

// 空闲进入低功耗的时间（毫秒）
#define IDLE_SLEEP_TIMEOUT_MS       30000   // 30秒无操作进入休眠

// 唤醒方式
#define WAKEUP_BY_KEY               1       // 按键唤醒
#define WAKEUP_BY_USB               1       // USB 插入唤醒

/* ==================== 调试配置 ==================== */

// 调试输出
#ifdef DEBUG
#define DUAL_MODE_DEBUG             1
#else
#define DUAL_MODE_DEBUG             0
#endif

#if DUAL_MODE_DEBUG
#define DM_PRINT(...)               PRINT(__VA_ARGS__)
#else
#define DM_PRINT(...)
#endif

#ifdef __cplusplus
}
#endif

#endif /* DUAL_MODE_CONFIG_H */
