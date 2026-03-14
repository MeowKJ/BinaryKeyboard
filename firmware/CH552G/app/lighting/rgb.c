#include "rgb.h"

#include "config.h"

typedef struct {
  uint8_t rainbowHue;
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
  uint8_t pressFade[KEY_COUNT];
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

static uint16_t millis16(void) {
  return (uint16_t)millis();
}

static uint8_t scale8(uint8_t value, uint8_t scale) {
  return (uint8_t)(((uint16_t)value * scale) / 255u);
}

static uint16_t speedInterval(uint16_t slow, uint16_t fast) {
  if (slow <= fast) {
    return fast;
  }
  return (uint16_t)(slow - (((uint32_t)(slow - fast) * currentSpeed) / 255u));
}

static uint8_t pressFadeStep(void) {
  return (uint8_t)(4u + (currentSpeed >> 4));
}

static void clearLogicalLeds(void) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    logicalLedR[i] = 0;
    logicalLedG[i] = 0;
    logicalLedB[i] = 0;
  }
}

static void setLogicalLed(uint8_t logicalIndex, uint8_t r, uint8_t g, uint8_t b) {
  if (logicalIndex >= NUM_LEDS) {
    return;
  }
  logicalLedR[logicalIndex] = r;
  logicalLedG[logicalIndex] = g;
  logicalLedB[logicalIndex] = b;
}

static void setAllLogicalLeds(uint8_t r, uint8_t g, uint8_t b) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    setLogicalLed(i, r, g, b);
  }
}

static void fillAllPhysicalLeds(uint8_t r, uint8_t g, uint8_t b) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    set_pixel_for_GRB_LED(ledData, i, g, r, b);
  }
}

static void flushLogicalLeds(void) {
  for (uint8_t logicalIndex = 0; logicalIndex < NUM_LEDS; logicalIndex++) {
    uint8_t physicalIndex = logicalToPhysicalMap[logicalIndex];
    set_pixel_for_GRB_LED(
      ledData,
      physicalIndex,
      logicalLedG[logicalIndex],
      logicalLedR[logicalIndex],
      logicalLedB[logicalIndex]);
  }
}

static void resolveFallbackColor(uint8_t *r, uint8_t *g, uint8_t *b) __reentrant {
  *r = scale8(currentColorR, currentBrightness);
  *g = scale8(currentColorG, currentBrightness);
  *b = scale8(currentColorB, currentBrightness);

  if (*r == 0 && *g == 0 && *b == 0) {
    *r = currentBrightness;
    *g = currentBrightness;
    *b = currentBrightness;
  }
}

static void stepLayerFlash(void) {
  if (!rgbState.layerFlashActive) {
    return;
  }

  if (rgbState.layerFlashWaitTicks > 0) {
    rgbState.layerFlashWaitTicks--;
    return;
  }

  if (rgbState.layerFlashIsOn) {
    rgbState.layerFlashIsOn = 0;
    rgbState.layerFlashWaitTicks = 10;
    return;
  }

  if (rgbState.layerFlashBlinksLeft > 0) {
    rgbState.layerFlashBlinksLeft--;
  }

  if (rgbState.layerFlashBlinksLeft == 0) {
    rgbState.layerFlashActive = 0;
    return;
  }

  rgbState.layerFlashIsOn = 1;
  rgbState.layerFlashWaitTicks = 10;
}

static void applyPressEffects(void) __reentrant {
  if (!rgbEnabled || pressEffect == PRESS_EFFECT_NONE) {
    return;
  }

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t intensity = rgbState.pressFade[i];
    uint8_t baseR = logicalLedR[i];
    uint8_t baseG = logicalLedG[i];
    uint8_t baseB = logicalLedB[i];

    if (intensity == 0) {
      continue;
    }

    if (pressEffect == PRESS_EFFECT_LIGHT_FADE) {
      if (baseR == 0 && baseG == 0 && baseB == 0) {
        resolveFallbackColor(&baseR, &baseG, &baseB);
        logicalLedR[i] = scale8(baseR, intensity);
        logicalLedG[i] = scale8(baseG, intensity);
        logicalLedB[i] = scale8(baseB, intensity);
      } else {
        uint16_t r = baseR + scale8(baseR, intensity);
        uint16_t g = baseG + scale8(baseG, intensity);
        uint16_t b = baseB + scale8(baseB, intensity);
        logicalLedR[i] = (uint8_t)(r > 255u ? 255u : r);
        logicalLedG[i] = (uint8_t)(g > 255u ? 255u : g);
        logicalLedB[i] = (uint8_t)(b > 255u ? 255u : b);
      }
    } else {
      uint8_t scale = (uint8_t)(255u - intensity);
      logicalLedR[i] = scale8(baseR, scale);
      logicalLedG[i] = scale8(baseG, scale);
      logicalLedB[i] = scale8(baseB, scale);
    }
  }
}

static void fadePressEffects(void) {
  uint8_t step = pressFadeStep();
  for (uint8_t i = 0; i < KEY_COUNT; i++) {
    if (rgbState.pressFade[i] > step) {
      rgbState.pressFade[i] -= step;
    } else {
      rgbState.pressFade[i] = 0;
    }
  }
}

static void applyLayerFlash(void) {
  if (!rgbState.layerFlashActive || !rgbState.layerFlashIsOn) {
    return;
  }

  setAllLogicalLeds(
    rgbState.layerFlashR,
    rgbState.layerFlashG,
    rgbState.layerFlashB);
}

// ==================== HSV 转 RGB（SDCC 兼容）====================
void hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v,
                uint8_t *r, uint8_t *g, uint8_t *b) __reentrant {
  uint8_t region = h / 43;
  uint8_t remainder = (h - (region * 43)) * 6;

  uint8_t p = (v * (255 - s)) >> 8;
  uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
  uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

  switch (region) {
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
void effect_off() {
  clearLogicalLeds();
}

void effect_static() {
  setAllLogicalLeds(
    scale8(currentColorR, currentBrightness),
    scale8(currentColorG, currentBrightness),
    scale8(currentColorB, currentBrightness));
}

void effect_breath() {
  uint16_t now = millis16();
  if ((uint16_t)(now - rgbState.baseLastTime) >= speedInterval(28, 5)) {
    rgbState.baseLastTime = now;
    rgbState.breathPhase = (uint8_t)(rgbState.breathPhase + rgbState.breathDelta);
    if (rgbState.breathPhase == 0 || rgbState.breathPhase == 255) {
      rgbState.breathDelta = -rgbState.breathDelta;
    }
  }

  setAllLogicalLeds(
    scale8(scale8(currentColorR, currentBrightness), rgbState.breathPhase),
    scale8(scale8(currentColorG, currentBrightness), rgbState.breathPhase),
    scale8(scale8(currentColorB, currentBrightness), rgbState.breathPhase));
}

void effect_blink() {
  uint16_t now = millis16();
  if ((uint16_t)(now - rgbState.blinkLastTime) >= speedInterval(700, 80)) {
    rgbState.blinkLastTime = now;
    rgbState.blinkState = !rgbState.blinkState;
  }

  if (rgbState.blinkState) {
    effect_static();
  } else {
    effect_off();
  }
}

void effect_rainbow() {
  uint16_t now = millis16();
  if ((uint16_t)(now - rgbState.baseLastTime) >= speedInterval(45, 6)) {
    rgbState.baseLastTime = now;
    rgbState.rainbowHue++;
  }

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t r, g, b;
    uint8_t hue = (uint8_t)(rgbState.rainbowHue + (uint8_t)((i * 255u) / NUM_LEDS));
    hsv_to_rgb(hue, 255, currentBrightness, &r, &g, &b);
    setLogicalLed(i, r, g, b);
  }
}

void effect_indicator() {
  effect_off();
}

// ==================== 层切换闪烁 ====================
static const uint8_t layerColors[][3] = {
  {0, 100, 255},   /* 层0: 蓝色 */
  {0, 255, 100},   /* 层1: 绿色 */
  {255, 200, 0},   /* 层2: 黄色 */
  {200, 0, 255},   /* 层3: 紫色 */
  {255, 50, 50},   /* 层4: 红色 */
};

void flashLayerColor(uint8_t layer) {
  if (layer >= MAX_LAYERS) {
    layer = MAX_LAYERS - 1;
  }

  rgbState.layerFlashR = layerColors[layer][0];
  rgbState.layerFlashG = layerColors[layer][1];
  rgbState.layerFlashB = layerColors[layer][2];
  rgbState.layerFlashBlinksLeft = 3;
  rgbState.layerFlashIsOn = 1;
  rgbState.layerFlashWaitTicks = 10;
  rgbState.layerFlashActive = 1;
}

static uint8_t hasPressActivity(void) {
  for (uint8_t i = 0; i < KEY_COUNT; i++) {
    if (rgbState.pressFade[i] > 0) return 1;
  }
  return 0;
}

void updateLEDs() {
  stepLayerFlash();

  // 层闪烁全局强制：暂停所有其他效果
  if (rgbState.layerFlashActive) {
    clearLogicalLeds();
    applyLayerFlash();
    flushLogicalLeds();
    neopixel_show_P1_5(ledData, NUM_LEDS * 3);
    delayMicroseconds(300);
    return;
  }

  // 当附魔效果激活且有按键余晖时，基础层保持全灭，让附魔效果独占显示
  uint8_t enchantOnly = (rgbEnabled && pressEffect != PRESS_EFFECT_NONE && hasPressActivity());

  if (!rgbEnabled || effectMode == EFFECT_OFF || enchantOnly) {
    effect_off();
  } else {
    switch (effectMode) {
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
  flushLogicalLeds();
  neopixel_show_P1_5(ledData, NUM_LEDS * 3);
  delayMicroseconds(300);
  fadePressEffects();
}

void led_init() {
  fillAllPhysicalLeds(0, 0, 0);
  rgbState.rainbowHue = 0;
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
  for (uint8_t i = 0; i < KEY_COUNT; i++) {
    rgbState.pressFade[i] = 0;
  }
  updateLEDs();
}

void rgbRegisterKeyPress(uint8_t keyIndex) {
  if (!rgbEnabled || pressEffect == PRESS_EFFECT_NONE || keyIndex >= NUM_LEDS) {
    return;
  }
  rgbState.pressFade[keyIndex] = 255;
}

// ==================== 灯效控制函数封装 ====================
void increaseBrightness() {
  uint16_t next = (uint16_t)currentBrightness + 16u;
  currentBrightness = (uint8_t)(next > 255u ? 255u : next);
}

void decreaseBrightness() {
  currentBrightness = (currentBrightness > 16u) ? (uint8_t)(currentBrightness - 16u) : 0u;
}

void increaseSpeed() {
  uint16_t next = (uint16_t)currentSpeed + 16u;
  currentSpeed = (uint8_t)(next > 255u ? 255u : next);
}

void decreaseSpeed() {
  currentSpeed = (currentSpeed > 16u) ? (uint8_t)(currentSpeed - 16u) : 0u;
}

void nextEffect() {
  uint8_t next = (uint8_t)(effectMode + 1u);
  if (next > EFFECT_RAINBOW) {
    next = EFFECT_OFF;
  }
  effectMode = next;
  updateLEDs();
}
