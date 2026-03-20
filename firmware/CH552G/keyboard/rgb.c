#include "rgb.h"

#include "config.h"
#include "KeyScanner.h"
#include "KeysDataHandler.h"
#include "MacroStorage.h"

static void hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v,
                       uint8_t *r, uint8_t *g, uint8_t *b) __reentrant;

typedef struct
{
  uint16_t rainbowHue16;
  uint8_t breathPhase;
  int8_t breathDelta;
  uint8_t layerFlashActive;
  uint8_t layerFlashBlinksLeft;
  uint8_t layerFlashIsOn;
  uint8_t layerFlashWaitTicks;
  uint8_t layerFlashKeyIndex;
  uint8_t pressOverlay[NUM_LEDS]; // 0-255 亮度叠加层
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

// ==================== HSV -> RGB ====================
static void hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v,
                       uint8_t *r, uint8_t *g, uint8_t *b) __reentrant
{
  uint8_t region, base;
  if (h < 43)
  {
    region = 0;
    base = 0;
  }
  else if (h < 86)
  {
    region = 1;
    base = 43;
  }
  else if (h < 129)
  {
    region = 2;
    base = 86;
  }
  else if (h < 172)
  {
    region = 3;
    base = 129;
  }
  else if (h < 215)
  {
    region = 4;
    base = 172;
  }
  else
  {
    region = 5;
    base = 215;
  }

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
  uint8_t step = (uint8_t)(1u + (currentSpeed >> 5));

  if (rgbState.breathDelta > 0)
  {
    if ((uint8_t)(255u - rgbState.breathPhase) <= step)
    {
      rgbState.breathPhase = 255;
      rgbState.breathDelta = -1;
    }
    else
    {
      rgbState.breathPhase += step;
    }
  }
  else
  {
    if (rgbState.breathPhase <= step)
    {
      rgbState.breathPhase = 0;
      rgbState.breathDelta = 1;
    }
    else
    {
      rgbState.breathPhase -= step;
    }
  }

  uint8_t phase = rgbState.breathPhase;
  uint8_t gamma = (uint8_t)(((uint16_t)phase * phase) >> 8);
  uint8_t brightness = scale8(currentBrightness, gamma);

  setAllLogicalLeds(
      scale8(currentColorR, brightness),
      scale8(currentColorG, brightness),
      scale8(currentColorB, brightness));
}

static void effect_rainbow(void)
{
  uint16_t sp1 = (uint16_t)(currentSpeed + 1u);
  uint16_t step = (uint16_t)(57u + sp1 + (uint16_t)((sp1 * 114u) >> 8));
  rgbState.rainbowHue16 += step;

  uint8_t hue = (uint8_t)(rgbState.rainbowHue16 >> 8);
  uint8_t r, g, b;
  hsv_to_rgb(hue, 255, currentBrightness, &r, &g, &b);
  setAllLogicalLeds(r, g, b);
}

static void effect_neon(void)
{
  uint16_t sp1 = (uint16_t)(currentSpeed + 1u);
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

// ==================== Press overlay ====================
// 每帧根据物理按键状态更新 overlay 并调制背景灯效输出
// LIGHT_FADE: overlay 默认0（黑），按住升→255，松开降→0
// DARK_FADE:  overlay 默认255（亮），按住降→0，松开升→255
// 固定 100ms 间隔更新 overlay，每帧仅做调制
#define PRESS_OVERLAY_STEP 28u // 255/28 ≈ 9步 × 100ms ≈ 0.9s 全程
#define PRESS_OVERLAY_TICKS 6u // 16ms × 6 ≈ 100ms (按键叠加层以 60fps 更新)

static void applyPressOverlay(void)
{
  static __xdata uint8_t pressTick;
  uint8_t doStep = 0;

  if (++pressTick >= PRESS_OVERLAY_TICKS)
  {
    pressTick = 0;
    doStep = 1;
  }

  for (uint8_t i = 0; i < NUM_LEDS; i++)
  {
    uint8_t ov = rgbState.pressOverlay[i];
    uint8_t held = KeyScanner_isPressed(i);
    uint8_t goUp = (pressEffect == PRESS_EFFECT_LIGHT_FADE) ? held : !held;
    // 按下瞬间立即响应：overlay 处于静止态时跳过 tick 门控
    uint8_t atRest = (pressEffect == PRESS_EFFECT_LIGHT_FADE) ? (ov == 0) : (ov == 255);
    if (doStep || (held && atRest))
    {
      if (goUp)
      {
        if ((uint8_t)(255u - ov) <= PRESS_OVERLAY_STEP)
          ov = 255;
        else
          ov += PRESS_OVERLAY_STEP;
      }
      else
      {
        if (ov <= PRESS_OVERLAY_STEP)
          ov = 0;
        else
          ov -= PRESS_OVERLAY_STEP;
      }
      rgbState.pressOverlay[i] = ov;
    }
    // overlay 调制背景：final = background * overlay / 255
    logicalLedR[i] = scale8(logicalLedR[i], ov);
    logicalLedG[i] = scale8(logicalLedG[i], ov);
    logicalLedB[i] = scale8(logicalLedB[i], ov);
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
  static __xdata uint8_t bgFrame; // 背景帧计数，偶数帧推进动画
  uint8_t advanceBg = !(bgFrame & 1); // 每2帧推进一次 → 背景30fps
  bgFrame++;

  if (advanceBg)
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

  // 1. 背景灯效
  if (!rgbEnabled || effectMode == EFFECT_OFF)
  {
    clearLogicalLeds();
  }
  else
  {
    // 保存动画状态（奇数帧不推进，重算后还原）
    uint16_t savedHue;
    uint8_t savedPhase;
    int8_t savedDelta;
    if (!advanceBg)
    {
      savedHue = rgbState.rainbowHue16;
      savedPhase = rgbState.breathPhase;
      savedDelta = rgbState.breathDelta;
    }

    switch (effectMode)
    {
    case EFFECT_STATIC:
      effect_static();
      break;
    case EFFECT_BREATH:
      effect_breath();
      break;
    case EFFECT_NEON:
      effect_neon();
      break;
    case EFFECT_RAINBOW:
      effect_rainbow();
      break;
    default:
      clearLogicalLeds();
      break;
    }

    // 奇数帧还原动画状态
    if (!advanceBg)
    {
      rgbState.rainbowHue16 = savedHue;
      rgbState.breathPhase = savedPhase;
      rgbState.breathDelta = savedDelta;
    }
  }

  // 2. 按键叠加层（每帧都更新 → 60fps）
  if (rgbEnabled && pressEffect != PRESS_EFFECT_NONE)
  {
    applyPressOverlay();
  }

  // 3. 宏循环闪烁
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
}

// updatePressFrame 已废弃，60fps 统一由 updateLEDs 处理
void updatePressFrame(void)
{
  updateLEDs();
}

void led_init(void)
{
  memset(ledData, 0, sizeof(ledData));
  memset(&rgbState, 0, sizeof(rgbState));
  rgbState.breathDelta = 1;
  updateLEDs();
}

// rgbRegisterKeyPress 已废弃，overlay 由每帧物理按键状态驱动
void rgbRegisterKeyPress(uint8_t keyIndex)
{
  (void)keyIndex;
}

// ==================== Effect cycling ====================
void nextEffect(void)
{
  uint8_t next;
  switch (effectMode)
  {
  case EFFECT_OFF:
    next = EFFECT_STATIC;
    break;
  case EFFECT_STATIC:
    next = EFFECT_BREATH;
    break;
  case EFFECT_BREATH:
    next = EFFECT_RAINBOW;
    break;
  default:
    next = EFFECT_OFF;
    break;
  }
  effectMode = next;
  rgbEnabled = 1;
  eeprom_write_byte(EEPROM_RGB_MODE_ADDR, effectMode);
  eeprom_write_byte(EEPROM_RGB_ENABLED_ADDR, rgbEnabled);
  updateLEDs();
}
