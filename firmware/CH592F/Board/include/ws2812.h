#ifndef __WS2812_H
#define __WS2812_H

#include "CH59x_common.h"
#include "kbd_config.h"

/** ============================================================================
 *  WS2812 RGB LED 驱动配置
 *  ============================================================================
 */

/** 配置参数 */
#ifndef WS2812_LED_NUM
#define WS2812_LED_NUM (1 + KBD_NUM_KEYS) /**< 键盘 LED 数量 */
#endif

#define WS2812_PIN GPIO_Pin_10 /**< WS2812 数据引脚 PA10 */
#define WS2812_INDICATOR_MIN_BRIGHTNESS                                        \
  16                       /**< 指示灯最低亮度 (~6%)，不可完全关闭 */
#define WS2812_CLK_FREQ 60 /**< 系统时钟频率 (MHz) */

/** WS2812 时序参数 (基于 60MHz 系统时钟)
 *  T0H: 0.35μs (21 cycles)  - 逻辑0的高电平时间
 *  T1H: 0.85μs (51 cycles)  - 逻辑1的高电平时间
 *  周期: 1.25μs (75 cycles) - 单个bit的总周期
 *  复位: ≥50μs 低电平       - 数据锁存信号
 */
#define WS2812_T0H (uint32_t)(0.35 * WS2812_CLK_FREQ)
#define WS2812_T1H (uint32_t)(0.85 * WS2812_CLK_FREQ)
#define WS2812_BIT_CYCLE (uint32_t)(1.25 * WS2812_CLK_FREQ)
#define WS2812_RESET_CYCLES 300 /**< 复位信号周期数 (≈300μs @ 1.25μs/cycle) */

/** 缓冲区长度：LED数据(每个LED 24bit) + 复位信号 */
#define WS2812_BUF_LEN (WS2812_LED_NUM * 24 + WS2812_RESET_CYCLES)

/** ============================================================================
 *  数据结构
 *  ============================================================================
 */

/**
 * @brief RGB 颜色结构体
 */
typedef struct {
  uint8_t r; /**< 红色分量 (0-255) */
  uint8_t g; /**< 绿色分量 (0-255) */
  uint8_t b; /**< 蓝色分量 (0-255) */
} WS2812_Color;

/** ============================================================================
 *  核心 API
 *  ============================================================================
 */

/**
 * @brief 初始化 WS2812 驱动
 * @note 配置 GPIO、定时器和 DMA
 */
void WS2812_Init(void);

/**
 * @brief 设置指定 LED 的颜色
 * @param index LED 索引 (0 ~ WS2812_LED_NUM-1)
 * @param r 红色分量 (0-255)
 * @param g 绿色分量 (0-255)
 * @param b 蓝色分量 (0-255)
 */
void WS2812_Set(uint8_t index, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 填充所有 LED 为相同颜色
 * @param r 红色分量 (0-255)
 * @param g 绿色分量 (0-255)
 * @param b 蓝色分量 (0-255)
 */
void WS2812_Fill(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 填充按键灯（多 LED 时跳过 index 0 的指示灯）
 * @param r 红色分量 (0-255)
 * @param g 绿色分量 (0-255)
 * @param b 蓝色分量 (0-255)
 * @note 单 LED 时等同于 WS2812_Fill，多 LED 时只填充 index 1 开始
 */
void WS2812_FillKeys(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 刷新 LED 显示
 * @note 通过 DMA 将缓冲区数据发送到 WS2812
 */
void WS2812_Update(void);

/**
 * @brief 设置 RGB/按键灯亮度
 * @param brightness 亮度值 (0-255)
 */
void WS2812_SetBrightness(uint8_t brightness);

/**
 * @brief 设置指示灯亮度
 * @param brightness 亮度值 (0-255)
 */
void WS2812_SetIndicatorBrightness(uint8_t brightness);

/** ============================================================================
 *  辅助函数
 *  ============================================================================
 */

/**
 * @brief HSV 转 RGB 颜色空间转换
 * @param h 色调 (0-359)
 * @param s 饱和度 (0-255)
 * @param v 明度 (0-255)
 * @return WS2812_Color RGB 颜色
 */
WS2812_Color WS2812_HSVtoRGB(uint16_t h, uint8_t s, uint8_t v);

/**
 * @brief 色轮函数 - 生成彩虹色
 * @param pos 位置 (0-255)
 * @return WS2812_Color RGB 颜色
 * @note 0=红, 85=绿, 170=蓝, 255=红
 */
WS2812_Color WS2812_Wheel(uint8_t pos);

/**
 * @brief 设置指示灯颜色 (默认为第一个 LED)
 * @param r 红色分量 (0-255)
 * @param g 绿色分量 (0-255)
 * @param b 蓝色分量 (0-255)
 */
void WS2812_Set_Indicator(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 清除指示灯
 */
void WS2812_Clear_Indicator(void);

#endif /* __WS2812_H */