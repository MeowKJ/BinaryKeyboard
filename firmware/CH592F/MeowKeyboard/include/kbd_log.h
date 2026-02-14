/**
 * @file    kbd_log.h
 * @brief   MeowKeyboard HID 日志系统
 * @author  MeowKJ
 * @version V1.2.0
 * @date    2024-11-07
 *
 * @details
 * 通过 USB HID EP4 异步推送结构化日志到上位机。
 * 内部使用环形队列缓冲，主循环调用 Flush 发送。
 * 类别过滤由上位机 UI 完成，固件端只控制开关。
 *
 * @copyright Copyright (c) 2024 MeowKJ. All rights reserved.
 */

#ifndef __KBD_LOG_H
#define __KBD_LOG_H

#include <stdint.h>
#include "kbd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ======================= 初始化与主循环 ======================= */

/**
 * @brief 初始化 HID 日志系统，从系统配置加载参数
 */
void KBD_Log_Init(void);

/**
 * @brief 刷新日志队列，将缓冲的日志通过 EP4 发送
 * @note  在主循环中周期调用
 */
void KBD_Log_Flush(void);

/* ======================= 运行时配置 ======================= */

/**
 * @brief 设置日志开关（同时更新系统配置 RAM 副本）
 * @param enabled 0=关, 非0=开
 */
void KBD_Log_SetEnabled(uint8_t enabled);

/**
 * @brief 检查日志是否启用
 * @return 1=启用, 0=禁用
 */
uint8_t KBD_Log_IsEnabled(void);

/* ======================= 日志记录函数 ======================= */

/**
 * @brief 记录按键事件
 * @param key_index  按键索引
 * @param pressed    1=按下, 0=释放
 * @param action_type 动作类型 (kbd_action_type_t)
 * @param param      动作参数 (键码等)
 */
void KBD_Log_KeyEvent(uint8_t key_index, uint8_t pressed, uint8_t action_type, uint8_t param);

/**
 * @brief 记录 FN 键事件
 * @param fn_id    FN 键 ID (0-3)
 * @param is_long  0=单击, 1=长按
 * @param action   FN 动作码 (kbd_fn_action_t)
 * @param param    动作参数
 */
void KBD_Log_FnEvent(uint8_t fn_id, uint8_t is_long, uint8_t action, uint8_t param);

/**
 * @brief 记录层切换事件
 * @param old_layer 旧层号
 * @param new_layer 新层号
 */
void KBD_Log_LayerEvent(uint8_t old_layer, uint8_t new_layer);

/**
 * @brief 记录模式切换事件
 * @param old_mode 旧模式 (0=USB, 1=BLE)
 * @param new_mode 新模式
 */
void KBD_Log_ModeEvent(uint8_t old_mode, uint8_t new_mode);

/**
 * @brief 记录蓝牙事件
 * @param state 蓝牙状态 (0=断开, 1=广播, 2=已连接)
 */
void KBD_Log_BleEvent(uint8_t state);

void KBD_Log_BleDiagEvent(uint8_t state, uint8_t opcode, uint8_t reason, uint16_t connHandle);

/**
 * @brief 记录 RGB 事件
 * @param mode       RGB 模式
 * @param brightness 亮度
 */
void KBD_Log_RgbEvent(uint8_t mode, uint8_t brightness);

/**
 * @brief 记录系统事件
 * @param event 事件码 (kbd_log_sys_event_t)
 */
void KBD_Log_SystemEvent(uint8_t event);

#ifdef __cplusplus
}
#endif

#endif /* __KBD_LOG_H */
