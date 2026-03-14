/**
 * ============================================================================
 * WS2812 RGB LED 驱动实现
 * ============================================================================
 *
 * 硬件连接：
 *   - PA10: WS2812 数据线 (DIN)
 *   - 通过 TMR1 PWM + DMA 生成时序信号
 *
 * 数据格式：
 *   - 每个 LED 需要 24bit 数据 (GRB 顺序)
 *   - 数据传输后需要 ≥50μs 低电平复位信号
 *
 * 缓冲区布局：
 *   [LED0_24bit] [LED1_24bit] ... [LEDn_24bit] [RESET_300cycles]
 *
 * ============================================================================
 */

#include "ws2812.h"
#include <string.h>

/** ============================================================================
 *  内部变量
 *  ============================================================================
 */

/**
 * @brief WS2812 数据缓冲区
 * @note 4字节对齐，用于 DMA 传输
 *       布局：[LED数据区] + [复位信号区(全0)]
 */
__attribute__((aligned(4))) static uint32_t ws2812_buf[WS2812_BUF_LEN];

/**
 * @brief RGB/按键灯亮度 (0-255)
 */
static uint8_t s_rgb_brightness = 255;

/**
 * @brief 指示灯亮度 (0-255)
 */
static uint8_t s_indicator_brightness = 255;

/** ============================================================================
 *  内部函数
 *  ============================================================================
 */

/**
 * @brief 填充单字节数据到缓冲区 (MSB First)
 * @param p_buf 缓冲区指针 (需要指向8个连续的uint32_t空间)
 * @param data 要编码的字节数据
 *
 * @note WS2812 时序编码规则：
 *       - 逻辑 0: T0H=0.35μs (21 cycles)
 *       - 逻辑 1: T1H=0.85μs (51 cycles)
 *       - 总周期: 1.25μs (75 cycles)
 */
static void WS2812_Fill_Byte(uint32_t *p_buf, uint8_t data) {
  for (int8_t i = 7; i >= 0; i--) {
    if (data & (1 << i)) {
      *p_buf = WS2812_T1H; // 逻辑1: 高电平持续51个周期
    } else {
      *p_buf = WS2812_T0H; // 逻辑0: 高电平持续21个周期
    }
    p_buf++;
  }
}

/** ============================================================================
 *  核心 API 实现
 *  ============================================================================
 */

void WS2812_Init(void) {
  // 1. ⭐⭐ 核心修复：关闭 LSE，释放 PA10 给 PWM 使用 ⭐⭐
  // 这一句是之前的代码里漏掉的，导致 PA10 没输出
  PWR_UnitModCfg(DISABLE, UNIT_SYS_LSE);

  // 2. 配置 PA10 为推挽输出
  GPIOA_ModeCfg(GPIO_Pin_10, GPIO_ModeOut_PP_5mA);

  // 3. 初始化定时器1 PWM模式
  // 先关闭以防万一
  TMR1_Disable();
  TMR1_PWMInit(High_Level, PWM_Times_1);

  // 4. 设置周期 (1.25us = 75 ticks @ 60MHz)
  TMR1_PWMCycleCfg(75);

  // 6. 开启 PWM 引擎 (官方例程有这一步)
  TMR1_PWMEnable();

  // 7. 开启定时器 (WS2812_Update 会再次操作这个，但这里初始化一下也好)
  TMR1_Enable();

  // 8. 清空数据缓冲区
  memset(ws2812_buf, 0, sizeof(ws2812_buf));
}

/**
 * @brief 设置 RGB/按键灯亮度
 * @param brightness 亮度值 (0-255)
 */
void WS2812_SetBrightness(uint8_t brightness) { s_rgb_brightness = brightness; }

/**
 * @brief 设置指示灯亮度
 * @param brightness 亮度值 (0-255)，低于最低值时自动提升，确保指示灯常亮
 */
void WS2812_SetIndicatorBrightness(uint8_t brightness) {
  s_indicator_brightness = (brightness < KBD_INDICATOR_MIN_BRIGHTNESS)
      ? KBD_INDICATOR_MIN_BRIGHTNESS : brightness;
}

/**
 * @brief 设置特定 LED 颜色 (内部自动处理 GRB 顺序)
 * @param index LED 索引 (0 ~ WS2812_LED_NUM-1)
 * @param r 红色分量 (0-255)
 * @param g 绿色分量 (0-255)
 * @param b 蓝色分量 (0-255)
 */
void WS2812_Set(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
  if (index >= WS2812_LED_NUM)
    return;

  // 根据 LED 类型应用对应亮度：
  // - 仅 1 颗 LED 时（指示灯模式）：使用 RGB 亮度，便于 FN 键和网页 "RGB 亮度"
  // 控制
  // - 多颗 LED 时：index 0 为指示灯用 indicator，其余为按键灯用 rgb
  uint8_t br;
  if (WS2812_LED_NUM == 1) {
    br = s_rgb_brightness;
  } else {
    br = (index == 0) ? s_indicator_brightness : s_rgb_brightness;
  }
  if (br != 255) {
    r = (uint16_t)r * br / 255;
    g = (uint16_t)g * br / 255;
    b = (uint16_t)b * br / 255;
  }

  // 计算在缓冲区中的位置
  // 关键修复：LED数据从缓冲区起始位置开始，复位信号在最后
  uint32_t *p = &ws2812_buf[index * 24];

  // WS2812 标准发送顺序：G -> R -> B
  WS2812_Fill_Byte(p, g);      // Green (字节0-7)
  WS2812_Fill_Byte(p + 8, r);  // Red   (字节8-15)
  WS2812_Fill_Byte(p + 16, b); // Blue  (字节16-23)
}

/**
 * @brief 填充所有 LED 为相同颜色
 * @param r 红色分量 (0-255)
 * @param g 绿色分量 (0-255)
 * @param b 蓝色分量 (0-255)
 */
void WS2812_Fill(uint8_t r, uint8_t g, uint8_t b) {
  for (uint8_t i = 0; i < WS2812_LED_NUM; i++) {
    WS2812_Set(i, r, g, b);
  }
}

/**
 * @brief 填充按键灯（多 LED 时跳过 index 0 的指示灯）
 * @param r 红色分量 (0-255)
 * @param g 绿色分量 (0-255)
 * @param b 蓝色分量 (0-255)
 * @note 单 LED 时等同于 WS2812_Fill，多 LED 时只填充 index 1 开始
 */
void WS2812_FillKeys(uint8_t r, uint8_t g, uint8_t b) {
  if (WS2812_LED_NUM == 1) {
    WS2812_Fill(r, g, b);
  } else {
    for (uint8_t i = 1; i < WS2812_LED_NUM; i++) {
      WS2812_Set(i, r, g, b);
    }
  }
}

/**
 * @brief 刷新 LED 数据
 * @note 通过 DMA 将缓冲区数据发送到 TMR1 PWM FIFO
 */
void WS2812_Update(void) {
  // 1. 停止定时器
  TMR1_Disable();

  // 2. 配置 DMA 传输
  //    - 起始地址：ws2812_buf (LED数据从这里开始)
  //    - 结束地址：ws2812_buf + WS2812_BUF_LEN (包含复位信号)
  //    - 模式：单次传输
  TMR1_DMACfg(ENABLE, (uint16_t)(uint32_t)ws2812_buf,
              (uint16_t)(uint32_t)(ws2812_buf + WS2812_BUF_LEN), Mode_Single);

  // 3. 启动 PWM 和定时器
  TMR1_PWMEnable();
  TMR1_Enable();
}

/** ============================================================================
 *  辅助功能实现
 *  ============================================================================
 */

/**
 * @brief HSV 转换成 RGB
 * @param h 色调 0-359
 * @param s 饱和度 0-255
 * @param v 明度 0-255
 * @return WS2812_Color RGB 颜色
 *
 * @note HSV 颜色空间：
 *       - H (Hue): 色调，0=红, 120=绿, 240=蓝
 *       - S (Saturation): 饱和度，0=灰色, 255=纯色
 *       - V (Value): 明度，0=黑色, 255=最亮
 */
WS2812_Color WS2812_HSVtoRGB(uint16_t h, uint8_t s, uint8_t v) {
  WS2812_Color color;

  // 计算色调区间内的偏移
  uint8_t f = (h % 60) * 255 / 60;

  // 计算中间值
  uint8_t p = (uint16_t)v * (255 - s) / 255;
  uint8_t q = (uint16_t)v * (255 - (uint16_t)s * f / 255) / 255;
  uint8_t t = (uint16_t)v * (255 - (uint16_t)s * (255 - f) / 255) / 255;

  // 根据色调区间选择 RGB 值
  switch ((h / 60) % 6) {
  case 0:
    color.r = v;
    color.g = t;
    color.b = p;
    break; // 红->黄
  case 1:
    color.r = q;
    color.g = v;
    color.b = p;
    break; // 黄->绿
  case 2:
    color.r = p;
    color.g = v;
    color.b = t;
    break; // 绿->青
  case 3:
    color.r = p;
    color.g = q;
    color.b = v;
    break; // 青->蓝
  case 4:
    color.r = t;
    color.g = p;
    color.b = v;
    break; // 蓝->品红
  case 5:
    color.r = v;
    color.g = p;
    color.b = q;
    break; // 品红->红
  }

  return color;
}

/**
 * @brief 色轮函数，输入 0-255 返回平滑变化的 RGB
 * @param pos 位置 (0-255)
 * @return WS2812_Color RGB 颜色
 *
 * @note 颜色分布：
 *       - 0-84:   红 -> 绿
 *       - 85-169: 绿 -> 蓝
 *       - 170-255: 蓝 -> 红
 */
WS2812_Color WS2812_Wheel(uint8_t pos) {
  WS2812_Color c;

  if (pos < 85) {
    // 红 -> 绿
    c.r = pos * 3;
    c.g = 255 - pos * 3;
    c.b = 0;
  } else if (pos < 170) {
    // 绿 -> 蓝
    pos -= 85;
    c.r = 255 - pos * 3;
    c.g = 0;
    c.b = pos * 3;
  } else {
    // 蓝 -> 红
    pos -= 170;
    c.r = 0;
    c.g = pos * 3;
    c.b = 255 - pos * 3;
  }

  return c;
}

/**
 * @brief 设置指示灯颜色
 * @param r 红色分量 (0-255)
 * @param g 绿色分量 (0-255)
 * @param b 蓝色分量 (0-255)
 * @note 默认使用第一个 LED 作为指示灯
 */
void WS2812_Set_Indicator(uint8_t r, uint8_t g, uint8_t b) {
  WS2812_Set(0, r, g, b);
}

/**
 * @brief 清除指示灯
 */
void WS2812_Clear_Indicator(void) { WS2812_Set(0, 0, 0, 0); }