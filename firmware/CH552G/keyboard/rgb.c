#include "rgb.h"

#include "config.h"
#include "KeysDataHandler.h"
#include "MacroStorage.h"

static void hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v,
                       uint8_t *r, uint8_t *g, uint8_t *b) __reentrant;

typedef struct
{
  uint16_t rainbowHue16;
  uint8_t breathPhase;
  int8_t breathDelta;
  uint16_t baseLastTime;
  uint8_t blinkState;
  uint16_t blinkLastTime;
  uint8_t layerFlashActive;
  uint8_t layerFlashBlinksLeft;
  uint8_t layerFlashIsOn;
  uint8_t layerFlashWaitTicks;
  uint8_t layerFlashR;
  uint8_t layerFlashG;
  uint8_t layerFlashB;
  uint8_t layerFlashKeyIndex;
  uint8_t pressFade[KEY_COUNT];
  uint8_t pressColorR[NUM_LEDS];
  uint8_t pressColorG[NUM_LEDS];
  uint8_t pressColorB[NUM_LEDS];
} RgbState;

volatile __xdata uint8_t rgbEnabled = 1;
volatile __xdata uint8_t effectMode = EFFECT_RAINBOW;
volatile __xdata uint8_t currentBrightness = 128;
volatile __xdata uint8_t currentSpeed = 128;
volatile __xdata uint8_t currentColorR = 255;
volatile __xdata uint8_t currentColorG = 255;
volatile __xdata uint8_t currentColorB = 255;
volatile __xdata uint8_t indicatorEnabled = 0;
volatile __xdata uint8_t indicatorBrightness = 13;
volatile __xdata uint8_t pressEffect = PRESS_EFFECT_NONE;

static __xdata uint8_t ledData[NUM_LEDS * 3];
static __xdata uint8_t logicalLedR[NUM_LEDS];
static __xdata uint8_t logicalLedG[NUM_LEDS];
static __xdata uint8_t logicalLedB[NUM_LEDS];
static __xdata RgbState rgbState;

static const uint8_t logicalToPhysicalMap[NUM_LEDS] = RGB_LOGICAL_TO_PHYSICAL_MAP;

static uint16_t millis16(void)
{
  return (uint16_t)millis();
}

static uint8_t hue_offset(uint8_t index)
{
#if NUM_LEDS == 4
  static const uint8_t offsets[4] = {0, 63, 127, 191};
  return offsets[index];
#elif NUM_LEDS == 5
  static const uint8_t offsets[5] = {0, 51, 102, 153, 204};
  return offsets[index];
#else
  // Only used for non-standard LED counts.
  return (uint8_t)(((uint16_t)index * 255u) / NUM_LEDS);
#endif
}

static uint8_t scale8(uint8_t value, uint8_t scale)
{
  // Approximate (value*scale)/255 without pulling in SDCC division helpers.
  return (uint8_t)(((uint16_t)value * (uint16_t)(scale + 1u)) >> 8);
}

static uint16_t speedInterval(uint16_t slow, uint16_t fast)
{
  if (slow <= fast)
  {
    return fast;
  }
  // Approximate /255 with >>8 to avoid 32-bit division helpers.
  {
    uint8_t speed = currentSpeed;
    uint16_t diff = (uint16_t)(slow - fast);
    uint16_t scaled = (uint16_t)(((uint32_t)diff * (uint16_t)(speed + 1u)) >> 8);
    return (uint16_t)(slow - scaled);
  }
}

static uint8_t pressFadeStep(void)
{
  return (uint8_t)(3u + (currentSpeed >> 5));
}

static void clearLogicalLeds(void)
{
  for (uint8_t i = 0; i < NUM_LEDS; i++)
  {
    logicalLedR[i] = 0;
    logicalLedG[i] = 0;
    logicalLedB[i] = 0;
  }
}

static void setLogicalLed(uint8_t logicalIndex, uint8_t r, uint8_t g, uint8_t b)
{
  if (logicalIndex >= NUM_LEDS)
  {
    return;
  }
  logicalLedR[logicalIndex] = r;
  logicalLedG[logicalIndex] = g;
  logicalLedB[logicalIndex] = b;
}

static void setAllLogicalLeds(uint8_t r, uint8_t g, uint8_t b)
{
  for (uint8_t i = 0; i < NUM_LEDS; i++)
  {
    setLogicalLed(i, r, g, b);
  }
}

static void fillAllPhysicalLeds(uint8_t r, uint8_t g, uint8_t b)
{
  for (uint8_t i = 0; i < NUM_LEDS; i++)
  {
    set_pixel_for_GRB_LED(ledData, i, g, r, b);
  }
}

static void flushLogicalLeds(void)
{
  for (uint8_t logicalIndex = 0; logicalIndex < NUM_LEDS; logicalIndex++)
  {
    uint8_t physicalIndex = logicalToPhysicalMap[logicalIndex];
    set_pixel_for_GRB_LED(
        ledData,
        physicalIndex,
        logicalLedG[logicalIndex],
        logicalLedR[logicalIndex],
        logicalLedB[logicalIndex]);
  }
}

static void resolveFallbackColor(uint8_t *r, uint8_t *g, uint8_t *b) __reentrant
{
  *r = scale8(currentColorR, currentBrightness);
  *g = scale8(currentColorG, currentBrightness);
  *b = scale8(currentColorB, currentBrightness);

  if (*r == 0 && *g == 0 && *b == 0)
  {
    *r = currentBrightness;
    *g = currentBrightness;
    *b = currentBrightness;
  }
}

static void sampleBaseColorForLogicalLed(
    uint8_t logicalIndex,
    uint8_t *r,
    uint8_t *g,
    uint8_t *b) __reentrant
{
  *r = 0;
  *g = 0;
  *b = 0;

  switch (effectMode)
  {
  case EFFECT_STATIC:
    *r = scale8(currentColorR, currentBrightness);
    *g = scale8(currentColorG, currentBrightness);
    *b = scale8(currentColorB, currentBrightness);
    break;

  case EFFECT_BREATH:
    *r = scale8(scale8(currentColorR, currentBrightness), rgbState.breathPhase);
    *g = scale8(scale8(currentColorG, currentBrightness), rgbState.breathPhase);
    *b = scale8(scale8(currentColorB, currentBrightness), rgbState.breathPhase);
    break;

  case EFFECT_BLINK:
    if (rgbState.blinkState)
    {
      *r = scale8(currentColorR, currentBrightness);
      *g = scale8(currentColorG, currentBrightness);
      *b = scale8(currentColorB, currentBrightness);
    }
    break;

  case EFFECT_RAINBOW:
  {
    uint8_t hueBase = (uint8_t)(rgbState.rainbowHue16 >> 8);
    uint8_t hue = (uint8_t)(hueBase + hue_offset(logicalIndex));
    hsv_to_rgb(hue, 255, currentBrightness, r, g, b);
    break;
  }

  default:
    break;
  }

  if (*r == 0 && *g == 0 && *b == 0)
  {
    resolveFallbackColor(r, g, b);
  }
}

static void stepLayerFlash(void)
{
  if (!rgbState.layerFlashActive)
  {
    return;
  }

  if (rgbState.layerFlashWaitTicks > 0)
  {
    rgbState.layerFlashWaitTicks--;
    return;
  }

  if (rgbState.layerFlashIsOn)
  {
    rgbState.layerFlashIsOn = 0;
    rgbState.layerFlashWaitTicks = 10;
    return;
  }

  if (rgbState.layerFlashBlinksLeft > 0)
  {
    rgbState.layerFlashBlinksLeft--;
  }

  if (rgbState.layerFlashBlinksLeft == 0)
  {
    rgbState.layerFlashActive = 0;
    return;
  }

  rgbState.layerFlashIsOn = 1;
  rgbState.layerFlashWaitTicks = 10;
}

static void applyPressEffects(void) __reentrant
{
  if (!rgbEnabled || pressEffect == PRESS_EFFECT_NONE)
  {
    return;
  }

  for (uint8_t i = 0; i < NUM_LEDS; i++)
  {
    uint8_t intensity = rgbState.pressFade[i];
    uint8_t baseR = logicalLedR[i];
    uint8_t baseG = logicalLedG[i];
    uint8_t baseB = logicalLedB[i];

    if (intensity == 0)
    {
      continue;
    }

    if (pressEffect == PRESS_EFFECT_LIGHT_FADE)
    {
      uint8_t pr = scale8(rgbState.pressColorR[i], intensity);
      uint8_t pg = scale8(rgbState.pressColorG[i], intensity);
      uint8_t pb = scale8(rgbState.pressColorB[i], intensity);
      if (pr > baseR) logicalLedR[i] = pr;
      if (pg > baseG) logicalLedG[i] = pg;
      if (pb > baseB) logicalLedB[i] = pb;
    }
    else
    {
      uint8_t scale = (uint8_t)(255u - intensity);
      logicalLedR[i] = scale8(baseR, scale);
      logicalLedG[i] = scale8(baseG, scale);
      logicalLedB[i] = scale8(baseB, scale);
    }
  }
}

static void fadePressEffects(void)
{
  uint8_t step = pressFadeStep();
  for (uint8_t i = 0; i < KEY_COUNT; i++)
  {
    if (rgbState.pressFade[i] > step)
    {
      rgbState.pressFade[i] -= step;
    }
    else
    {
      rgbState.pressFade[i] = 0;
    }
  }
}

static void applyLayerFlash(void)
{
  if (!rgbState.layerFlashActive || !rgbState.layerFlashIsOn)
  {
    return;
  }

  setLogicalLed(
      rgbState.layerFlashKeyIndex,
      rgbState.layerFlashR,
      rgbState.layerFlashG,
      rgbState.layerFlashB);
}

// ==================== HSV 转 RGB（SDCC 兼容）====================
static void hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v,
                       uint8_t *r, uint8_t *g, uint8_t *b) __reentrant
{
  uint8_t region;
  uint8_t base;
  if (h < 43)      { region = 0; base = 0; }
  else if (h < 86) { region = 1; base = 43; }
  else if (h < 129){ region = 2; base = 86; }
  else if (h < 172){ region = 3; base = 129; }
  else if (h < 215){ region = 4; base = 172; }
  else             { region = 5; base = 215; }

  uint8_t remainder = (uint8_t)((h - base) * 6u);

  uint8_t p = (v * (255 - s)) >> 8;
  uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
  uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

  switch (region)
  {
  case 0:
    *r = v;
    *g = t;
    *b = p;
    break;
  case 1:
    *r = q;
    *g = v;
    *b = p;
    break;
  case 2:
    *r = p;
    *g = v;
    *b = t;
    break;
  case 3:
    *r = p;
    *g = q;
    *b = v;
    break;
  case 4:
    *r = t;
    *g = p;
    *b = v;
    break;
  default:
    *r = v;
    *g = p;
    *b = q;
    break;
  }
}

// ==================== 光效实现 ====================
static void effect_off()
{
  clearLogicalLeds();
}

static void effect_static()
{
  setAllLogicalLeds(
      scale8(currentColorR, currentBrightness),
      scale8(currentColorG, currentBrightness),
      scale8(currentColorB, currentBrightness));
}

static void effect_breath()
{
  uint16_t now = millis16();
  if ((uint16_t)(now - rgbState.baseLastTime) >= speedInterval(28, 5))
  {
    rgbState.baseLastTime = now;
    rgbState.breathPhase = (uint8_t)(rgbState.breathPhase + rgbState.breathDelta);
    if (rgbState.breathPhase == 0 || rgbState.breathPhase == 255)
    {
      rgbState.breathDelta = -rgbState.breathDelta;
    }
  }

  setAllLogicalLeds(
      scale8(scale8(currentColorR, currentBrightness), rgbState.breathPhase),
      scale8(scale8(currentColorG, currentBrightness), rgbState.breathPhase),
      scale8(scale8(currentColorB, currentBrightness), rgbState.breathPhase));
}

static void effect_blink()
{
  uint16_t now = millis16();
  if ((uint16_t)(now - rgbState.blinkLastTime) >= speedInterval(700, 80))
  {
    rgbState.blinkLastTime = now;
    rgbState.blinkState = !rgbState.blinkState;
  }

  if (rgbState.blinkState)
  {
    effect_static();
  }
  else
  {
    effect_off();
  }
}

static void effect_rainbow()
{
  /* 每帧累加 8.8 定点数色相，消除整数步进的生硬跳变 */
  uint8_t speed = currentSpeed;
  uint16_t step = (uint16_t)(57u + (uint16_t)(((uint32_t)370u * (uint16_t)(speed + 1u)) >> 8));
  rgbState.rainbowHue16 += step;

  uint8_t hueBase = (uint8_t)(rgbState.rainbowHue16 >> 8);
  for (uint8_t i = 0; i < NUM_LEDS; i++)
  {
    uint8_t r, g, b;
    uint8_t hue = (uint8_t)(hueBase + hue_offset(i));
    hsv_to_rgb(hue, 255, currentBrightness, &r, &g, &b);
    setLogicalLed(i, r, g, b);
  }
}

static void effect_indicator()
{
  effect_off();
}

// ==================== 层切换闪烁 ====================
static const uint8_t layerColors[][3] = {
    {0, 100, 255}, /* 层0: 蓝色 */
    {0, 255, 100}, /* 层1: 绿色 */
    {255, 200, 0}, /* 层2: 黄色 */
    {200, 0, 255}, /* 层3: 紫色 */
    {255, 50, 50}, /* 层4: 红色 */
};

void flashLayerColor(uint8_t layer, uint8_t keyIndex)
{
  if (layer >= MAX_LAYERS)
  {
    layer = MAX_LAYERS - 1;
  }
  if (keyIndex >= NUM_LEDS)
  {
    keyIndex = 0;
  }

  rgbState.layerFlashR = layerColors[layer][0];
  rgbState.layerFlashG = layerColors[layer][1];
  rgbState.layerFlashB = layerColors[layer][2];
  rgbState.layerFlashKeyIndex = keyIndex;
  rgbState.layerFlashBlinksLeft = 3;
  rgbState.layerFlashIsOn = 1;
  rgbState.layerFlashWaitTicks = 10;
  rgbState.layerFlashActive = 1;
}

void updateLEDs()
{
  stepLayerFlash();

  // 层闪烁全局强制：暂停所有其他效果
  if (rgbState.layerFlashActive)
  {
    clearLogicalLeds();
    applyLayerFlash();
    flushLogicalLeds();
    neopixel_show_P1_5(ledData, NUM_LEDS * 3);
    delayMicroseconds(300);
    return;
  }

  if (!rgbEnabled || effectMode == EFFECT_OFF)
  {
    effect_off();
  }
  else
  {
    switch (effectMode)
    {
    case EFFECT_STATIC:
      effect_static();
      break;
    case EFFECT_BREATH:
      effect_breath();
      break;
    case EFFECT_BLINK:
      effect_blink();
      break;
    case EFFECT_RAINBOW:
      effect_rainbow();
      break;
    case EFFECT_INDICATOR:
      effect_indicator();
      break;
    default:
      effect_off();
      break;
    }
  }

  applyPressEffects();

  // 循环宏运行时对应按键快速闪烁
  if (macro_is_running() && macro_is_looping())
  {
    uint8_t mk = macro_running_key();
    if (mk < NUM_LEDS)
    {
      // 约 80ms 周期闪烁 (updateLEDs 每 10ms 调用一次)
      static __xdata uint8_t macroFlashCnt;
      macroFlashCnt++;
      if (macroFlashCnt & 0x04) // 每 4 帧切换 ≈ 80ms
      {
        setLogicalLed(mk, 255, 255, 255);
      }
      else
      {
        setLogicalLed(mk, 0, 0, 0);
      }
    }
  }

  flushLogicalLeds();
  neopixel_show_P1_5(ledData, NUM_LEDS * 3);
  delayMicroseconds(300);
  fadePressEffects();
}

void led_init()
{
  fillAllPhysicalLeds(0, 0, 0);
  rgbState.rainbowHue16 = 0;
  rgbState.breathPhase = 0;
  rgbState.breathDelta = 1;
  rgbState.baseLastTime = 0;
  rgbState.blinkState = 1;
  rgbState.blinkLastTime = 0;
  rgbState.layerFlashActive = 0;
  rgbState.layerFlashBlinksLeft = 0;
  rgbState.layerFlashIsOn = 0;
  rgbState.layerFlashWaitTicks = 0;
  rgbState.layerFlashR = 0;
  rgbState.layerFlashG = 0;
  rgbState.layerFlashB = 0;
  rgbState.layerFlashKeyIndex = 0;
  for (uint8_t i = 0; i < KEY_COUNT; i++)
  {
    rgbState.pressFade[i] = 0;
  }
  for (uint8_t i = 0; i < NUM_LEDS; i++)
  {
    rgbState.pressColorR[i] = 0;
    rgbState.pressColorG[i] = 0;
    rgbState.pressColorB[i] = 0;
  }
  updateLEDs();
}

void rgbRegisterKeyPress(uint8_t keyIndex)
{
  if (!rgbEnabled || pressEffect == PRESS_EFFECT_NONE || keyIndex >= NUM_LEDS)
  {
    return;
  }

  if (pressEffect == PRESS_EFFECT_LIGHT_FADE)
  {
    sampleBaseColorForLogicalLed(
        keyIndex,
        &rgbState.pressColorR[keyIndex],
        &rgbState.pressColorG[keyIndex],
        &rgbState.pressColorB[keyIndex]);
  }

  rgbState.pressFade[keyIndex] = 255;
}

// ==================== 灯效控制函数封装 ====================
void nextEffect()
{
  uint8_t next = (uint8_t)(effectMode + 1u);
  if (next > EFFECT_RAINBOW)
  {
    next = EFFECT_OFF;
  }
  effectMode = next;
  rgbEnabled = 1;
  eeprom_write_byte(EEPROM_RGB_MODE_ADDR, effectMode);
  eeprom_write_byte(EEPROM_RGB_ENABLED_ADDR, rgbEnabled);
  updateLEDs();
}
