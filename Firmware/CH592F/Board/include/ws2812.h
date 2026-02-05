#ifndef __WS2812_H
#define __WS2812_H

#include "CH59x_common.h"
#include "kbd_config.h"

/** LED 配置 */
#ifndef WS2812_LED_NUM
    #ifdef WS2812_INDICATOR_ONLY
        #define WS2812_LED_NUM      1               /**< 仅指示灯 */
    #else
        #define WS2812_LED_NUM      (1 + KBD_NUM_KEYS)  /**< 指示灯 + 按键灯 */
    #endif
#endif

#define WS2812_CLK_FREQ     60                    /**< 60MHz 时钟 */

/** 时序计算 (基于 60MHz 时钟) */
#define WS2812_T0H          21   /**< 0.35µs = 21 cycles @ 60MHz */
#define WS2812_T1H          51   /**< 0.85µs = 51 cycles @ 60MHz */
#define WS2812_BIT_CYCLE    75   /**< 1.25µs = 75 cycles @ 60MHz */
#define WS2812_RESET_CYCLES 80   /**< 复位周期数 (>50µs = >3000 cycles, 用 80 个 PWM 周期) */

/** 缓冲区长度：LED 数据 + 复位信号 */
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