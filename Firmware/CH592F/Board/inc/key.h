#ifndef __KEY_H__
#define __KEY_H__

#include "CH59x_common.h"

void Key_Init();


// 按键扫描函数
void Key_Scan(void);

// 检测按键是否被按下(单次触发)
uint8_t Key_IsPressed(uint8_t key_index);

// 检测按键是否被释放(单次触发)
uint8_t Key_IsReleased(uint8_t key_index);

// 获取按键当前状态(持续状态)
uint8_t Key_GetState(uint8_t key_index);

#endif