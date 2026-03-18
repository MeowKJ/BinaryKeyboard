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
  uint8_t layerFlashActive;
  uint8_t layerFlashBlinksLeft;
  uint8_t layerFlashIsOn;
  uint8_t layerFlashWaitTicks;
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

static uint8_t hue_offset(uint8_t index)
{
#if NUM_LEDS == 4
  static const uint8_t offsets[4] = {0, 63, 127, 191};
  return offsets[index];
#elif NUM_LEDS == 5
  static const uint8_t offsets[5] = {0, 51, 102, 153, 204};
  return offsets[index];
#else
  return (uint8_t)(((uint16_t)index * 255u) / NUM_LEDS);
#endif
}

static uint8_t scale8(uint8_t value, uint8_t scale)
{
  return (uint8_t)(((uint16_t)value * (uint16_t)(scale + 1u)) >> 8);
}

// (a * b) >> 8 using 16-bit decomposition, avoids pulling in __mullong
static uint16_t mul16_shr8(uint16_t a, uint16_t b)
{
  uint8_t ah = (uint8_t)(a >> 8);
  uint16_t lo = (uint16_t)((uint8_t)a) * b;
  return (uint16_t)(ah * b) + (uint16_t)(lo >> 8);
}

static uint16_t speedInterval(uint16_t slow, uint16_t fast)
{
  if (slow <= fast)
    return fast;
  uint16_t diff = (uint16_t)(slow - fast);
  uint16_t scaled = mul16_shr8(diff, (uint16_t)(currentSpeed + 1u));
  return (uint16_t)(slow - scaled);
}

static uint8_t pressFadeStep(void)
{
  return (uint8_t)(3u + (currentSpeed >> 5));
}

static void clearLogicalLeds(void)
{
  memset(logicalLedR, 0, NUM_LEDS);
  memset(logicalLedG, 0, NUM_LEDS);
  memset(logicalLedB, 0, NUM_LEDS);
}

static void setLogicalLed(uint8_t logicalIndex, uint8_t r, uint8_t g, uint8_t b)
{
  if (logicalIndex >= NUM_LEDS)
    return;
  logicalLedR[logicalIndex] = r;
  logicalLedG[logicalIndex] = g;
  logicalLedB[logicalIndex] = b;
}

static void setAllLogicalLeds(uint8_t r, uint8_t g, uint8_t b)
{
  memset(logicalLedR, r, NUM_LEDS);
  memset(logicalLedG, g, NUM_LEDS);
  memset(logicalLedB, b, NUM_LEDS);
}

static void flushLogicalLeds(void)
{
  for (uint8_t i = 0; i < NUM_LEDS; i++)
  {
    uint8_t p = logicalToPhysicalMap[i];
    set_pixel_for_GRB_LED(ledData, p, logicalLedG[i], logicalLedR[i], logicalLedB[i]);
  }
}

static void stepLayerFlash(void)
{
  if (!rgbState.layerFlashActive)
    return;

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
    rgbState.layerFlashBlinksLeft--;

  if (rgbState.layerFlashBlinksLeft == 0)
  {
    rgbState.layerFlashActive = 0;
    return;
  }

  rgbState.layerFlashIsOn = 1;
  rgbState.layerFlashWaitTicks = 10;
}

static void applyPressEffects(void)
{
  if (!rgbEnabled || pressEffect == PRESS_EFFECT_NONE)
    return;

  for (uint8_t i = 0; i < NUM_LEDS; i++)
  {
    uint8_t intensity = rgbState.pressFade[i];
    if (intensity == 0)
      continue;

    if (pressEffect == PRESS_EFFECT_LIGHT_FADE)
    {
      uint8_t pr = scale8(rgbState.pressColorR[i], intensity);
      uint8_t pg = scale8(rgbState.pressColorG[i], intensity);
      uint8_t pb = scale8(rgbState.pressColorB[i], intensity);
      if (pr > logicalLedR[i]) logicalLedR[i] = pr;
      if (pg > logicalLedG[i]) logicalLedG[i] = pg;
      if (pb > logicalLedB[i]) logicalLedB[i] = pb;
    }
    else
    {
      uint8_t s = (uint8_t)(255u - intensity);
      logicalLedR[i] = scale8(logicalLedR[i], s);
      logicalLedG[i] = scale8(logicalLedG[i], s);
      logicalLedB[i] = scale8(logicalLedB[i], s);
    }
  }
}

static void fadePressEffects(void)
{
  uint8_t step = pressFadeStep();
  for (uint8_t i = 0; i < KEY_COUNT; i++)
  {
    if (rgbState.pressFade[i] > step)
      rgbState.pressFade[i] -= step;
    else
      rgbState.pressFade[i] = 0;
  }
}

// ==================== HSV -> RGB ====================
static void hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v,
                       uint8_t *r, uint8_t *g, uint8_t *b) __reentrant
{
  uint8_t region, base;
  if (h < 43)       { region = 0; base = 0; }
  else if (h < 86)  { region = 1; base = 43; }
  else if (h < 129) { region = 2; base = 86; }
  else if (h < 172) { region = 3; base = 129; }
  else if (h < 215) { region = 4; base = 172; }
  else              { region = 5; base = 215; }

  uint8_t remainder = (uint8_t)((h - base) * 6u);
  uint8_t p = (v * (255 - s)) >> 8;
  uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
  uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

  switch (region)
  {
  case 0: *r = v; *g = t; *b = p; break;
  case 1: *r = q; *g = v; *b = p; break;
  case 2: *r = p; *g = v; *b = t; break;
  case 3: *r = p; *g = q; *b = v; break;
  case 4: *r = t; *g = p; *b = v; break;
  default: *r = v; *g = p; *b = q; break;
  }
}

// ==================== Effects ====================
static void effect_static(void)
{
  setAllLogicalLeds(
      scale8(currentColorR, currentBrightness),
      scale8(currentColorG, currentBrightness),
      scale8(currentColorB, currentBrightness));
}

static void effect_breath(void)
{
  uint16_t now = (uint16_t)millis();
  if ((uint16_t)(now - rgbState.baseLastTime) >= speedInterval(28, 5))
  {
    rgbState.baseLastTime = now;
    rgbState.breathPhase = (uint8_t)(rgbState.breathPhase + rgbState.breathDelta);
    if (rgbState.breathPhase == 0 || rgbState.breathPhase == 255)
      rgbState.breathDelta = -rgbState.breathDelta;
  }

  setAllLogicalLeds(
      scale8(scale8(currentColorR, currentBrightness), rgbState.breathPhase),
      scale8(scale8(currentColorG, currentBrightness), rgbState.breathPhase),
      scale8(scale8(currentColorB, currentBrightness), rgbState.breathPhase));
}

static void effect_rainbow(void)
{
  uint16_t sp1 = (uint16_t)(currentSpeed + 1u);
  // (370 * sp1) >> 8, decomposed: 370 = 1*256 + 114
  uint16_t step = (uint16_t)(57u + sp1 + (uint16_t)((sp1 * 114u) >> 8));
  rgbState.rainbowHue16 += step;

  uint8_t hueBase = (uint8_t)(rgbState.rainbowHue16 >> 8);
  for (uint8_t i = 0; i < NUM_LEDS; i++)
  {
    uint8_t r, g, b;
    hsv_to_rgb((uint8_t)(hueBase + hue_offset(i)), 255, currentBrightness, &r, &g, &b);
    setLogicalLed(i, r, g, b);
  }
}

// ==================== Layer flash (fixed white) ====================
void flashLayerColor(uint8_t layer, uint8_t keyIndex)
{
  (void)layer;
  if (keyIndex >= NUM_LEDS)
    keyIndex = 0;
  rgbState.layerFlashKeyIndex = keyIndex;
  rgbState.layerFlashBlinksLeft = 3;
  rgbState.layerFlashIsOn = 1;
  rgbState.layerFlashWaitTicks = 10;
  rgbState.layerFlashActive = 1;
}

void updateLEDs(void)
{
  stepLayerFlash();

  if (rgbState.layerFlashActive)
  {
    clearLogicalLeds();
    if (rgbState.layerFlashIsOn)
      setLogicalLed(rgbState.layerFlashKeyIndex, 255, 255, 255);
    flushLogicalLeds();
    neopixel_show_P1_5(ledData, NUM_LEDS * 3);
    delayMicroseconds(300);
    return;
  }

  if (!rgbEnabled || effectMode == EFFECT_OFF)
  {
    clearLogicalLeds();
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
    case EFFECT_RAINBOW:
      effect_rainbow();
      break;
    default:
      clearLogicalLeds();
      break;
    }
  }

  applyPressEffects();

  if (macro_is_running() && macro_is_looping())
  {
    uint8_t mk = macro_running_key();
    if (mk < NUM_LEDS)
    {
      static __xdata uint8_t macroFlashCnt;
      macroFlashCnt++;
      if (macroFlashCnt & 0x04)
        setLogicalLed(mk, 255, 255, 255);
      else
        setLogicalLed(mk, 0, 0, 0);
    }
  }

  flushLogicalLeds();
  neopixel_show_P1_5(ledData, NUM_LEDS * 3);
  delayMicroseconds(300);
  fadePressEffects();
}

void led_init(void)
{
  memset(ledData, 0, sizeof(ledData));
  memset(&rgbState, 0, sizeof(rgbState));
  rgbState.breathDelta = 1;
  updateLEDs();
}

void rgbRegisterKeyPress(uint8_t keyIndex)
{
  if (!rgbEnabled || pressEffect == PRESS_EFFECT_NONE || keyIndex >= NUM_LEDS)
    return;

  if (pressEffect == PRESS_EFFECT_LIGHT_FADE)
  {
    rgbState.pressColorR[keyIndex] = logicalLedR[keyIndex];
    rgbState.pressColorG[keyIndex] = logicalLedG[keyIndex];
    rgbState.pressColorB[keyIndex] = logicalLedB[keyIndex];
  }

  rgbState.pressFade[keyIndex] = 255;
}

// ==================== Effect cycling ====================
void nextEffect(void)
{
  uint8_t next;
  switch (effectMode)
  {
  case EFFECT_OFF:    next = EFFECT_STATIC;  break;
  case EFFECT_STATIC: next = EFFECT_BREATH;  break;
  case EFFECT_BREATH: next = EFFECT_RAINBOW; break;
  default:            next = EFFECT_OFF;     break;
  }
  effectMode = next;
  rgbEnabled = 1;
  eeprom_write_byte(EEPROM_RGB_MODE_ADDR, effectMode);
  eeprom_write_byte(EEPROM_RGB_ENABLED_ADDR, rgbEnabled);
  updateLEDs();
}
