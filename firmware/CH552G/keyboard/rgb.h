#ifndef __RGB_H__
#define __RGB_H__

#include <Arduino.h>
#include <WS2812.h>

enum {
  EFFECT_OFF,
  EFFECT_STATIC,
  EFFECT_BREATH,
  EFFECT_BLINK,
  EFFECT_RAINBOW,
  EFFECT_INDICATOR,
  EFFECT_COUNT
};

enum {
  PRESS_EFFECT_NONE,
  PRESS_EFFECT_LIGHT_FADE,
  PRESS_EFFECT_DARK_FADE,
  PRESS_EFFECT_COUNT
};


// ==================== HSV 转 RGB（SDCC 兼容）====================
void hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v,
                uint8_t *r, uint8_t *g, uint8_t *b) __reentrant;

// ==================== 光效实现 ====================
void effect_off();
void effect_static();
void effect_breath();
void effect_blink();
void effect_rainbow();
void effect_indicator();

// ==================== RGB 状态（可外部读写）====================
extern volatile __xdata uint8_t rgbEnabled;
extern volatile __xdata uint8_t effectMode;
extern volatile __xdata uint8_t currentBrightness;
extern volatile __xdata uint8_t currentSpeed;
extern volatile __xdata uint8_t currentColorR;
extern volatile __xdata uint8_t currentColorG;
extern volatile __xdata uint8_t currentColorB;
extern volatile __xdata uint8_t indicatorEnabled;
extern volatile __xdata uint8_t indicatorBrightness;
extern volatile __xdata uint8_t pressEffect;

// ==================== 灯效控制函数 ====================
void increaseBrightness(void);  // 增加亮度
void decreaseBrightness(void);  // 减少亮度
void increaseSpeed(void);       // 增加速度
void decreaseSpeed(void);       // 减少速度
void nextEffect(void);          // 切换灯效

void updateLEDs();

void led_init();
void rgbRegisterKeyPress(uint8_t keyIndex);

// ==================== 层切换闪烁 ====================
void flashLayerColor(uint8_t layer, uint8_t keyIndex);  // 触发层切换闪烁（仅指定按键闪烁）
#endif
