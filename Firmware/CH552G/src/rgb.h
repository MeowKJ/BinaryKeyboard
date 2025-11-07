#ifndef __RGB_H__
#define __RGB_H__

#include <Arduino.h>
#include <WS2812.h>

enum {
  EFFECT_OFF,
  EFFECT_RAINBOW,
  EFFECT_BREATH,
  EFFECT_CHASE,
  EFFECT_WAVE,
  EFFECT_GRADIENT,
  EFFECT_FLASH,
  EFFECT_COUNT
};


// ==================== HSV 转 RGB（SDCC 兼容）====================
void hsv_to_rgb(uint32_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b);

// ==================== 光效实现 ====================
void effect_off();

void effect_rainbow();
void effect_breath();
void effect_chase();
void effect_wave();
void effect_gradient();
void effect_flash();

// ==================== 灯效控制函数 ====================
void increaseBrightness(void);  // 增加亮度
void decreaseBrightness(void);  // 减少亮度
void increaseSpeed(void);       // 增加速度
void decreaseSpeed(void);       // 减少速度
void nextEffect(void);          // 切换灯效

void updateLEDs();

void led_init();
#endif