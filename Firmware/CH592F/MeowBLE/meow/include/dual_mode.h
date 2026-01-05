/********************************** (C) COPYRIGHT *******************************
 * File Name          : dual_mode.h
 * Author             : Custom Keyboard Library
 * Version            : V1.0
 * Date               : 2024/11/07
 * Description        : USB/BLE 双模切换管理器接口
 *******************************************************************************/

#ifndef DUAL_MODE_H
#define DUAL_MODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dual_mode_config.h"
#include <stdint.h>
#include <stdbool.h>

/* ==================== 类型定义 ==================== */

/**
 * @brief 模式切换回调函数类型
 * @param new_mode 新的工作模式
 */
typedef void (*mode_change_cb_t)(work_mode_t new_mode);

/**
 * @brief 连接状态变化回调函数类型
 * @param state 新的连接状态
 */
typedef void (*conn_state_cb_t)(conn_state_t state);

/**
 * @brief LED 输出报告回调函数类型
 * @param leds LED 状态位图
 */
typedef void (*led_report_cb_t)(uint8_t leds);

/**
 * @brief 双模管理器回调结构
 */
typedef struct {
    mode_change_cb_t    onModeChange;       // 模式切换回调
    conn_state_cb_t     onConnStateChange;  // 连接状态变化回调
    led_report_cb_t     onLedReport;        // LED 报告回调
} dual_mode_callbacks_t;

/* ==================== 初始化 API ==================== */

/**
 * @brief 初始化双模管理器
 * @param initial_mode 初始工作模式
 * @param pCBs 回调函数指针（可为 NULL）
 * @return 0 成功，其他失败
 */
int DualMode_Init(work_mode_t initial_mode, dual_mode_callbacks_t *pCBs);

/**
 * @brief 双模管理器主循环处理
 * @note  需要在主循环中周期性调用
 */
void DualMode_Process(void);

/* ==================== 模式切换 API ==================== */

/**
 * @brief 切换工作模式
 * @param mode 目标模式
 * @return 0 成功，其他失败
 */
int DualMode_SwitchMode(work_mode_t mode);

/**
 * @brief 获取当前工作模式
 * @return 当前模式
 */
work_mode_t DualMode_GetMode(void);

/**
 * @brief 切换到另一个模式（USB<->BLE）
 * @return 0 成功，其他失败
 */
int DualMode_ToggleMode(void);

/* ==================== 连接状态 API ==================== */

/**
 * @brief 获取当前连接状态
 * @return 连接状态
 */
conn_state_t DualMode_GetConnState(void);

/**
 * @brief 检查是否已连接（可发送报告）
 * @return true 已连接，false 未连接
 */
bool DualMode_IsConnected(void);

/* ==================== 蓝牙控制 API ==================== */

/**
 * @brief 开始蓝牙广播
 * @return 0 成功，其他失败
 */
int DualMode_BLE_StartAdvertising(void);

/**
 * @brief 停止蓝牙广播
 * @return 0 成功，其他失败
 */
int DualMode_BLE_StopAdvertising(void);

/**
 * @brief 断开蓝牙连接
 * @return 0 成功，其他失败
 */
int DualMode_BLE_Disconnect(void);

/**
 * @brief 清除所有蓝牙配对信息
 * @return 0 成功，其他失败
 */
int DualMode_BLE_ClearBonds(void);

/**
 * @brief 获取蓝牙绑定设备数量
 * @return 绑定设备数量
 */
uint8_t DualMode_BLE_GetBondCount(void);

/* ==================== USB 控制 API ==================== */

/**
 * @brief 检测 USB 是否已插入
 * @return true USB 已插入，false 未插入
 */
bool DualMode_USB_IsPlugged(void);

/**
 * @brief USB 远程唤醒主机
 * @return 0 成功，其他失败
 */
int DualMode_USB_Wakeup(void);

/* ==================== HID 报告发送 API ==================== */

/**
 * @brief 发送键盘报告
 * @param modifier 修饰键位图
 * @param keys 按键数组（最多 6 个）
 * @param key_count 按键数量
 * @return 0 成功，其他失败
 */
int DualMode_SendKeyboardReport(uint8_t modifier, uint8_t *keys, uint8_t key_count);

/**
 * @brief 发送单个按键（按下+释放）
 * @param modifier 修饰键
 * @param keycode 按键码
 * @return 0 成功，其他失败
 */
int DualMode_SendKeyPress(uint8_t modifier, uint8_t keycode);

/**
 * @brief 释放所有键盘按键
 * @return 0 成功，其他失败
 */
int DualMode_ReleaseAllKeys(void);

/**
 * @brief 发送鼠标报告
 * @param buttons 按钮位图
 * @param x X 轴移动量
 * @param y Y 轴移动量
 * @param wheel 滚轮移动量
 * @return 0 成功，其他失败
 */
int DualMode_SendMouseReport(uint8_t buttons, int8_t x, int8_t y, int8_t wheel);

/**
 * @brief 发送鼠标点击
 * @param buttons 按钮位图
 * @return 0 成功，其他失败
 */
int DualMode_SendMouseClick(uint8_t buttons);

/**
 * @brief 发送多媒体控制报告
 * @param key 多媒体控制码
 * @return 0 成功，其他失败
 */
int DualMode_SendConsumerReport(uint16_t key);

/**
 * @brief 发送多媒体按键（按下+释放）
 * @param key 多媒体控制码
 * @return 0 成功，其他失败
 */
int DualMode_SendConsumerKey(uint16_t key);

/* ==================== 低功耗 API ==================== */

/**
 * @brief 进入低功耗模式
 */
void DualMode_EnterSleep(void);

/**
 * @brief 退出低功耗模式
 */
void DualMode_ExitSleep(void);

/**
 * @brief 检查是否处于低功耗模式
 * @return true 低功耗模式，false 正常模式
 */
bool DualMode_IsInSleep(void);

/* ==================== LED 状态 API ==================== */

/**
 * @brief 获取键盘 LED 状态
 * @return LED 状态位图（NumLock, CapsLock, ScrollLock）
 */
uint8_t DualMode_GetKeyboardLEDs(void);

/* ==================== 键盘修饰键定义 ==================== */
#define MOD_LCTRL       0x01
#define MOD_LSHIFT      0x02
#define MOD_LALT        0x04
#define MOD_LGUI        0x08
#define MOD_RCTRL       0x10
#define MOD_RSHIFT      0x20
#define MOD_RALT        0x40
#define MOD_RGUI        0x80

/* ==================== 鼠标按钮定义 ==================== */
#define MOUSE_BTN_LEFT      0x01
#define MOUSE_BTN_RIGHT     0x02
#define MOUSE_BTN_MIDDLE    0x04

/* ==================== 键盘 LED 定义 ==================== */
#define LED_NUM_LOCK        0x01
#define LED_CAPS_LOCK       0x02
#define LED_SCROLL_LOCK     0x04

/* ==================== 多媒体控制码定义 ==================== */
#define CONSUMER_PLAY_PAUSE         0x00CD
#define CONSUMER_STOP               0x00B7
#define CONSUMER_SCAN_NEXT          0x00B5
#define CONSUMER_SCAN_PREV          0x00B6
#define CONSUMER_VOLUME_UP          0x00E9
#define CONSUMER_VOLUME_DOWN        0x00EA
#define CONSUMER_MUTE               0x00E2
#define CONSUMER_BRIGHTNESS_UP      0x006F
#define CONSUMER_BRIGHTNESS_DOWN    0x0070
#define CONSUMER_CALCULATOR         0x0192
#define CONSUMER_BROWSER            0x0196
#define CONSUMER_EMAIL              0x018A

#ifdef __cplusplus
}
#endif

#endif /* DUAL_MODE_H */
