#include "ws2812.h"
#include <string.h>

/** * 内部缓冲区说明：
 * CH592 的 TMR DMA 每次搬运 4 字节到 FIFO。
 * 布局：[Reset(全0)] + [LED1(24字节)] + [LED2(24字节)] ... + [Reset(全0)]
 */
__attribute__((aligned(4))) static uint32_t ws2812_buf[WS2812_BUF_LEN];
static uint8_t s_brightness = 100; // 默认亮度 (0-255)

/**
 * @brief 内部函数：填充单字节数据到缓冲区 (MSB First)
 */
static void WS2812_Fill_Byte(uint32_t *p_buf, uint8_t data) {
    for (int8_t i = 7; i >= 0; i--) {
        if (data & (1 << i)) {
            *p_buf = WS2812_T1H;
        } else {
            *p_buf = WS2812_T0H;
        }
        p_buf++;
    }
}

/**
 * @brief 初始化 WS2812 GPIO 和定时器
 */
void WS2812_Init(void) {
    // 1. 配置 PA10 为推挽输出
    GPIOA_ModeCfg(GPIO_Pin_10, GPIO_ModeOut_PP_5mA);
    
    // 2. 配置定时器 1 为 PWM 模式
    TMR1_PWMInit(High_Level, PWM_Times_1);
    TMR1_PWMCycleCfg(WS2812_BIT_CYCLE);
    
    // 3. 清空缓冲区 (初始化为复位信号)
    memset(ws2812_buf, 0, sizeof(ws2812_buf));
}

/**
 * @brief 设置全局亮度
 */
void WS2812_SetBrightness(uint8_t brightness) {
    s_brightness = brightness;
}

/**
 * @brief 设置特定 LED 颜色 (内部自动处理 GRB 顺序)
 */
void WS2812_Set(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (index >= WS2812_LED_NUM) return;

    // 亮度缩放
    if (s_brightness != 255) {
        r = (uint16_t)r * s_brightness / 255;
        g = (uint16_t)g * s_brightness / 255;
        b = (uint16_t)b * s_brightness / 255;
    }

    // 计算在缓冲区中的位置 (跳过开头的复位信号区)
    uint32_t *p = &ws2812_buf[WS2812_RESET_CYCLES + (index * 24)];
    
    // WS2812 标准发送顺序：G -> R -> B
    WS2812_Fill_Byte(p, g);      // Green
    WS2812_Fill_Byte(p + 8, r);  // Red
    WS2812_Fill_Byte(p + 16, b); // Blue
}

/**
 * @brief 填充所有 LED
 */
void WS2812_Fill(uint8_t r, uint8_t g, uint8_t b) {
    for (uint8_t i = 0; i < WS2812_LED_NUM; i++) {
        WS2812_Set(i, r, g, b);
    }
}

/**
 * @brief 刷新 LED 数据（你要求的保持不变的部分）
 */
void WS2812_Update(void) {
    TMR1_Disable();
    TMR1_DMACfg(ENABLE,
                (uint16_t)(uint32_t)ws2812_buf,
                (uint16_t)(uint32_t)(ws2812_buf + WS2812_BUF_LEN),
                Mode_Single);
    TMR1_PWMEnable();
    TMR1_Enable();
}

// =========================== 辅助功能 ===========================

/**
 * @brief HSV 转换成 RGB
 * @param h 色调 0-359
 * @param s 饱和度 0-255
 * @param v 明度 0-255
 */
WS2812_Color WS2812_HSVtoRGB(uint16_t h, uint8_t s, uint8_t v) {
    WS2812_Color color;
    uint8_t f = (h % 60) * 255 / 60;
    uint8_t p = (uint16_t)v * (255 - s) / 255;
    uint8_t q = (uint16_t)v * (255 - (uint16_t)s * f / 255) / 255;
    uint8_t t = (uint16_t)v * (255 - (uint16_t)s * (255 - f) / 255) / 255;

    switch ((h / 60) % 6) {
        case 0: color.r = v; color.g = t; color.b = p; break;
        case 1: color.r = q; color.g = v; color.b = p; break;
        case 2: color.r = p; color.g = v; color.b = t; break;
        case 3: color.r = p; color.g = q; color.b = v; break;
        case 4: color.r = t; color.g = p; color.b = v; break;
        case 5: color.r = v; color.g = p; color.b = q; break;
    }
    return color;
}

/**
 * @brief 色轮函数，输入 0-255 返回平滑变化的 RGB
 */
WS2812_Color WS2812_Wheel(uint8_t pos) {
    WS2812_Color c;
    if (pos < 85) {
        c.r = pos * 3; c.g = 255 - pos * 3; c.b = 0;
    } else if (pos < 170) {
        pos -= 85;
        c.r = 255 - pos * 3; c.g = 0; c.b = pos * 3;
    } else {
        pos -= 170;
        c.r = 0; c.g = pos * 3; c.b = 255 - pos * 3;
    }
    return c;
}

void WS2812_Set_Indicator(uint8_t r, uint8_t g, uint8_t b) {
    WS2812_Set(0, r, g, b); // 默认第一个灯为指示灯
}

void WS2812_Clear_Indicator(void) {
    WS2812_Set(0, 0, 0, 0);
}