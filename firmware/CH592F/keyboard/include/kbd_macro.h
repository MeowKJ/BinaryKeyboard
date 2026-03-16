/**
 * @file    kbd_macro.h
 * @brief   MeowKeyboard 宏回放引擎
 * @author  MeowKJ
 */

#ifndef __KBD_MACRO_H
#define __KBD_MACRO_H

#include "kbd_types.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化宏引擎（注册 TMOS 任务）
 */
void KBD_Macro_Init(void);

/**
 * @brief 启动宏回放
 * @param slot    宏槽位 (0~7)
 * @param trigger 触发模式
 * @return 0 成功，-1 槽位无效，-2 Flash 读取失败
 */
int KBD_Macro_Execute(uint8_t slot, kbd_macro_trigger_t trigger);

/**
 * @brief 通知宏引擎：触发键已松开
 */
void KBD_Macro_OnKeyRelease(void);

/**
 * @brief 取消当前宏（释放所有宏按键并停止）
 */
void KBD_Macro_Cancel(void);

/**
 * @brief 查询宏是否正在运行
 */
bool KBD_Macro_IsRunning(void);

#ifdef __cplusplus
}
#endif

#endif /* __KBD_MACRO_H */
