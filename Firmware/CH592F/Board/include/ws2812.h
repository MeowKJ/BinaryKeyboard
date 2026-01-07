#ifndef __WS2812_H
#define __WS2812_H

#include "CH59x_common.h"

/**
 * @file ws2812.h
 * @brief CH5xx TMR PWM + DMA 驱动 WS2812 RGB LED
 * @author
 * @date 2026-01-07
 */

/** @defgroup WS2812_Config WS2812 配置参数
 * @{
 */
#define WS2812_LED_NUM 10  /**< LED 数量 */
#define WS2812_CLK_FREQ 60 /**< 系统时钟频率(MHz)，需与实际时钟一致 */
#define WS2812_RESET_US 50 /**< WS2812 复位信号最小时间(us) */

#define WS2812_T0H (uint32_t)(0.4 * WS2812_CLK_FREQ)                               /**< 0码高电平 */
#define WS2812_T1H (uint32_t)(0.8 * WS2812_CLK_FREQ)                               /**< 1码高电平 */
#define WS2812_BIT_CYCLE (uint32_t)(1.25 * WS2812_CLK_FREQ)                        /**< 单bit总周期 */
#define WS2812_T0L (WS2812_BIT_CYCLE - WS2812_T0H)                                 /**< 0码低电平 */
#define WS2812_T1L (WS2812_BIT_CYCLE - WS2812_T1H)                                 /**< 1码低电平 */
#define WS2812_RESET_CYCLES (uint32_t)((WS2812_RESET_US * WS2812_CLK_FREQ) / 1000) /**< 复位周期数 */

#define WS2812_BUF_LEN (WS2812_RESET_CYCLES + WS2812_LED_NUM * 24 + WS2812_RESET_CYCLES / 2) /**< 缓冲区总长度 */
/** @} */

/**
 * @brief 初始化 WS2812（GPIO + 定时器 + 缓冲区）
 */
void WS2812_Init(void);

/**
 * @brief 设置单个 LED 颜色
 * @param index LED 索引 (0~WS2812_LED_NUM-1)
 * @param g 绿色分量 (0-255)
 * @param r 红色分量 (0-255)
 * @param b 蓝色分量 (0-255)
 */
void WS2812_Set(uint8_t index, uint8_t g, uint8_t r, uint8_t b);

/**
 * @brief 填充所有 LED 为同一颜色
 * @param g 绿色分量 (0-255)
 * @param r 红色分量 (0-255)
 * @param b 蓝色分量 (0-255)
 */
void WS2812_Fill(uint8_t g, uint8_t r, uint8_t b);

/**
 * @brief 刷新 LED 数据（启动 DMA + PWM）
 */
void WS2812_Update(void);

/**
 * @brief 设置指示灯的颜色
 * @param g 绿色分量(0-255)
 * @param r 红色分量(0-255)
 * @param b 蓝色分量(0-255)
 *
 * @note 第一个RGB作为指示灯使用
 */

void WS2812_Set_Indicator(uint8_t g, uint8_t r, uint8_t b)
#endif
