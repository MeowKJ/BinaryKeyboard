#ifndef __RGB_H__
#define __RGB_H__

#include "ch552_defs.h"
#include <WS2812.h>

enum
{
  EFFECT_OFF,
  EFFECT_STATIC,
  EFFECT_BREATH,
  EFFECT_BLINK,     // removed, kept for protocol compat
  EFFECT_RAINBOW,
  EFFECT_INDICATOR, // removed, kept for protocol compat
  EFFECT_COUNT
};

enum
{
  PRESS_EFFECT_NONE,
  PRESS_EFFECT_LIGHT_FADE,
  PRESS_EFFECT_DARK_FADE,
  PRESS_EFFECT_COUNT
};

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
void nextEffect(void); // 切换灯效

void updateLEDs();

void led_init();
void rgbRegisterKeyPress(uint8_t keyIndex);

// ==================== 层切换闪烁 ====================
void flashLayerColor(uint8_t layer, uint8_t keyIndex);
#endif
