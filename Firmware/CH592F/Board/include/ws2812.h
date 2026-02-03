#ifndef __WS2812_H
#define __WS2812_H

#include "CH59x_common.h"
#include "kbd_config.h"

/** 配置参数 */
#define WS2812_LED_NUM      1 + KBD_NUM_KEYS   /**< 键盘 LED 数量 */
#define WS2812_CLK_FREQ     60
#define WS2812_RESET_US     300 /**< 增加复位时间，确保部分国产灯珠稳定锁存 */

/** 时序计算 */
#define WS2812_T0H          (uint32_t)(0.35 * WS2812_CLK_FREQ)
#define WS2812_T1H          (uint32_t)(0.85 * WS2812_CLK_FREQ)
#define WS2812_BIT_CYCLE    (uint32_t)(1.25 * WS2812_CLK_FREQ)
#define WS2812_RESET_CYCLES 300 // 逻辑低电平周期数

/** 缓冲区长度：复位 + 数据 */
#define WS2812_BUF_LEN      (WS2812_LED_NUM * 24 + WS2812_RESET_CYCLES)

typedef struct {
    uint8_t r, g, b;
} WS2812_Color;

/** 核心 API */
void WS2812_Init(void);
void WS2812_Set(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void WS2812_Fill(uint8_t r, uint8_t g, uint8_t b);
void WS2812_Update(void);
void WS2812_SetBrightness(uint8_t brightness); // 0-255

/** 新增辅助函数 */
WS2812_Color WS2812_HSVtoRGB(uint16_t h, uint8_t s, uint8_t v);
WS2812_Color WS2812_Wheel(uint8_t wheelPos);
void WS2812_Set_Indicator(uint8_t r, uint8_t g, uint8_t b);
void WS2812_Clear_Indicator(void);

#endif