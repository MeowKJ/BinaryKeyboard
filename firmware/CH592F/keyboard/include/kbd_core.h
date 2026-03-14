/**
 * @file    kbd_core.h
 * @brief   MeowKeyboard 核心处理模块
 * @author  MeowKJ
 * @version V2.0.0
 * @date    2024-11-07
 *
 * @details
 * 本模块负责键盘核心逻辑处理，包括：
 * - 按键事件处理与动作执行
 * - FN 功能键处理
 * - 双模状态回调管理
 * - RGB 状态指示联动
 * - 层切换管理
 * - 宏执行（预留）
 *
 * @note 使用 key.h 的事件类型，直接集成按键驱动
 */

#ifndef __KBD_CORE_H
#define __KBD_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include "kbd_types.h"
#include "key.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup KBD_Core_API 核心处理接口
 * @{
 */

/**
 * @brief 初始化键盘核心模块
 * @note  需要在以下模块初始化之后调用：
 *        - Key_Init()
 *        - KBD_Storage_Init()
 *        - KBD_RGB_Init()
 *        - DualMode_Init()
 */
void KBD_Core_Init(void);

/**
 * @brief 核心处理函数（主循环调用）
 * @details 内部处理：
 *          - 普通按键事件
 *          - FN 按键事件
 *          - BOOT 键检测
 *          - RGB 效果更新
 */
void KBD_Core_Process(void);

/**
 * @brief 处理普通按键事件
 * @param[in] evt 按键事件指针
 */
void KBD_Core_HandleKeyEvent(const key_event_t *evt);

/**
 * @brief 处理 FN 按键事件
 * @param[in] evt FN 按键事件指针
 */
void KBD_Core_HandleFnEvent(const fnkey_event_t *evt);

/**
 * @brief 释放所有按键
 * @note  用于模式切换或断开连接时
 */
void KBD_Core_ReleaseAll(void);

/**
 * @brief 获取双模回调结构（供 DualMode_Init 使用）
 * @return 回调结构指针
 */
void *KBD_Core_GetCallbacks(void);

/** @} */ /* end of KBD_Core_API */

#ifdef __cplusplus
}
#endif

#endif /* __KBD_CORE_H */
