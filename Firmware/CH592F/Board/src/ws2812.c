#include "ws2812.h"

/**
 * @file ws2812.c
 * @brief CH5xx TMR PWM + DMA 驱动 WS2812 RGB LED（统一 WS2812_XXX 接口）
 * @author
 * @date 2026-01-07
 */

/** 内部 WS2812 数据缓冲区 */
__attribute__((aligned(4))) static uint32_t ws2812_buf[WS2812_BUF_LEN];

/**
 * @brief 内部函数：填充单个 LED 的 24bit RGB 数据
 * @param buf 指向 LED 缓冲区起始位置
 * @param g 绿色分量
 * @param r 红色分量
 * @param b 蓝色分量
 */
static void WS2812_Set_RGB_Bits(uint32_t *buf, uint8_t g, uint8_t r, uint8_t b)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        buf[i] = (g & 0x01) ? WS2812_T1H : WS2812_T0H;
        buf[i + 8] = (r & 0x01) ? WS2812_T1H : WS2812_T0H;
        buf[i + 16] = (b & 0x01) ? WS2812_T1H : WS2812_T0H;
        g >>= 1;
        r >>= 1;
        b >>= 1;
    }
}

/**
 * @brief 内部函数：初始化 WS2812 缓冲区 (填充复位信号)
 */
static void WS2812_Buf_Init(void)
{
    for (uint32_t i = 0; i < WS2812_RESET_CYCLES; i++)
        ws2812_buf[i] = 0;

    uint32_t data_end = WS2812_RESET_CYCLES + WS2812_LED_NUM * 24;
    for (uint32_t i = data_end; i < WS2812_BUF_LEN; i++)
        ws2812_buf[i] = 0;
}

/**
 * @brief 初始化 WS2812（GPIO + 定时器 + 缓冲区）
 */
void WS2812_Init(void)
{
    // GPIO 配置 (PB11 推挽输出)
    GPIOA_ModeCfg(GPIO_Pin_11, GPIO_ModeOut_PP_5mA);

    // PWM 定时器配置
    TMR2_PWMCycleCfg(WS2812_BIT_CYCLE);
    TMR2_PWMInit(High_Level, PWM_Times_1);

    // 缓冲区初始化
    WS2812_Buf_Init();
}

/**
 * @brief 设置单个 LED 颜色
 */
void WS2812_Set(uint8_t index, uint8_t g, uint8_t r, uint8_t b)
{
    if (index >= WS2812_LED_NUM)
        return;
    uint32_t *led_buf = ws2812_buf + WS2812_RESET_CYCLES + index * 24;
    WS2812_Set_RGB_Bits(led_buf, g, r, b);
}

/**
 * @brief 填充所有 LED 为同一颜色
 */
void WS2812_Fill(uint8_t g, uint8_t r, uint8_t b)
{
    for (uint8_t i = 0; i < WS2812_LED_NUM; i++)
        WS2812_Set(i, g, r, b);
}

/**
 * @brief 刷新 LED 数据（启动 DMA + PWM）
 */
void WS2812_Update(void)
{
    TMR2_DMACfg(ENABLE,
                (uint16_t)(uint32_t)ws2812_buf,
                (uint16_t)(uint32_t)(ws2812_buf + WS2812_BUF_LEN),
                Mode_Single);
    TMR2_PWMEnable();
    TMR2_Enable();
}

// =========================== 自定义功能 ===========================

/**
 * @brief 设置指示灯的颜色
 * @param g 绿色分量(0-255)
 * @param r 红色分量(0-255)
 * @param b 蓝色分量(0-255)
 *
 * @note 第一个RGB作为指示灯使用
 */

void WS2812_Set_Indicator(uint8_t g, uint8_t r, uint8_t b)
{
    WS2812_Set(0, g, r, b);
}