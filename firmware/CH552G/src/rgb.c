#include "rgb.h"
#include "config.h"

typedef struct {
  uint8_t rainbowHue;
  uint8_t breathBrightness;
  int8_t breathDelta;
  uint8_t chasePos;
  uint16_t chaseLastTime;
  uint8_t wavePhase;
  uint16_t waveLastTime;
  uint8_t gradientStartHue;
  uint16_t gradientLastTime;
  uint8_t flashState;
  uint16_t flashLastTime;
  uint8_t layerFlashBlinksLeft;
  uint8_t layerFlashIsOn;
  uint8_t layerFlashWaitTicks;
  uint8_t layerFlashR;
  uint8_t layerFlashG;
  uint8_t layerFlashB;
} RgbState;

volatile __xdata uint8_t effectMode = 0;
volatile __xdata uint8_t currentBrightness = 128; // 亮度控制 (0-255)
volatile __xdata uint8_t currentSpeed = 5; // 速度控制 (1-10)

// ==================== WS2812 LED 设置 ====================
static __xdata uint8_t ledData[NUM_LEDS * 3];  // GRB 缓冲区
static __xdata RgbState rgbState;

static uint16_t millis16(void) {
  return (uint16_t)millis();
}


// ==================== HSV 转 RGB（SDCC 兼容）====================
void hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v,
                uint8_t *__data r, uint8_t *__data g, uint8_t *__data b) {
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
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    set_pixel_for_GRB_LED(ledData, i, 0, 0, 0);
  }
}

void effect_rainbow() {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t r, g, b;
    hsv_to_rgb(rgbState.rainbowHue + i * 64, 255, currentBrightness, &r, &g, &b);
    set_pixel_for_GRB_LED(ledData, i, g, r, b);
  }
  rgbState.rainbowHue++;
}

void effect_breath() {
  rgbState.breathBrightness += rgbState.breathDelta;
  if (rgbState.breathBrightness == 0 || rgbState.breathBrightness == 255) {
    rgbState.breathDelta = -rgbState.breathDelta;
  }

  uint8_t v = rgbState.breathBrightness / 3;
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    set_pixel_for_GRB_LED(ledData, i, v, v, v);
  }
}

void effect_chase() {
  uint16_t now = millis16();
  if ((uint16_t)(now - rgbState.chaseLastTime) > (uint16_t)(300 / currentSpeed)) {
    rgbState.chasePos = (rgbState.chasePos + 1) % NUM_LEDS;
    rgbState.chaseLastTime = now;
  }
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t val = (i == rgbState.chasePos) ? currentBrightness : currentBrightness / 6;
    set_pixel_for_GRB_LED(ledData, i, 0, val, 0);
  }
}

void effect_wave() {
  uint16_t now = millis16();
  if ((uint16_t)(now - rgbState.waveLastTime) > (uint16_t)(50 / currentSpeed)) {
    rgbState.wavePhase++;
    rgbState.waveLastTime = now;
  }
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t r, g, b;
    uint8_t hue = rgbState.wavePhase + i * 64;
    hsv_to_rgb(hue, 255, currentBrightness, &r, &g, &b);
    set_pixel_for_GRB_LED(ledData, i, g, r, b);
  }
}

void effect_gradient() {
  uint16_t now = millis16();
  if ((uint16_t)(now - rgbState.gradientLastTime) > (uint16_t)(100 / currentSpeed)) {
    rgbState.gradientStartHue++;
    rgbState.gradientLastTime = now;
  }
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t r, g, b;
    uint8_t hue = rgbState.gradientStartHue + (i * 255 / NUM_LEDS);
    hsv_to_rgb(hue, 255, currentBrightness, &r, &g, &b);
    set_pixel_for_GRB_LED(ledData, i, g, r, b);
  }
}

void effect_flash() {
  uint16_t now = millis16();
  if ((uint16_t)(now - rgbState.flashLastTime) > (uint16_t)(200 / currentSpeed)) {
    rgbState.flashState = !rgbState.flashState;
    rgbState.flashLastTime = now;
  }
  uint8_t brightness = rgbState.flashState ? currentBrightness : 0;
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    set_pixel_for_GRB_LED(ledData, i, brightness, brightness, brightness);
  }
}

// ==================== 层切换闪烁 ====================
// CH592F 对齐: 层0=蓝, 层1=绿, 层2=黄, 层3=紫, 固定闪 3 次
void flashLayerColor(uint8_t layer) {
  switch (layer) {
    case 0:  rgbState.layerFlashR = 0;   rgbState.layerFlashG = 100; rgbState.layerFlashB = 255; break;
    case 1:  rgbState.layerFlashR = 0;   rgbState.layerFlashG = 255; rgbState.layerFlashB = 100; break;
    case 2:  rgbState.layerFlashR = 255; rgbState.layerFlashG = 200; rgbState.layerFlashB = 0;   break;
    default: rgbState.layerFlashR = 200; rgbState.layerFlashG = 0;   rgbState.layerFlashB = 255; break;
  }
  rgbState.layerFlashBlinksLeft = 3;
  rgbState.layerFlashIsOn = 1;
  rgbState.layerFlashWaitTicks = 10;  // ~100ms (10ms loop × 10)
}

void updateLEDs() {
  if (rgbState.layerFlashBlinksLeft > 0) {
    if (rgbState.layerFlashWaitTicks > 0) {
      rgbState.layerFlashWaitTicks--;
    } else if (rgbState.layerFlashIsOn) {
      rgbState.layerFlashIsOn = 0;
      rgbState.layerFlashWaitTicks = 10;
    } else {
      rgbState.layerFlashBlinksLeft--;
      if (rgbState.layerFlashBlinksLeft > 0) {
        rgbState.layerFlashIsOn = 1;
        rgbState.layerFlashWaitTicks = 10;
      }
    }

    for (uint8_t i = 0; i < NUM_LEDS; i++) {
      if (rgbState.layerFlashIsOn) {
        set_pixel_for_GRB_LED(ledData, i,
                              rgbState.layerFlashG,
                              rgbState.layerFlashR,
                              rgbState.layerFlashB);
      } else {
        set_pixel_for_GRB_LED(ledData, i, 0, 0, 0);
      }
    }
  } else {
    switch (effectMode) {
      case EFFECT_OFF: effect_off(); break;
      case EFFECT_RAINBOW: effect_rainbow(); break;
      case EFFECT_BREATH: effect_breath(); break;
      case EFFECT_CHASE: effect_chase(); break;
      case EFFECT_WAVE: effect_wave(); break;
      case EFFECT_GRADIENT: effect_gradient(); break;
      case EFFECT_FLASH: effect_flash(); break;
    }
  }
  neopixel_show_P1_5(ledData, NUM_LEDS * 3);
  delayMicroseconds(300);
}



void led_init() {
  for (uint8_t i = 0; i < NUM_LEDS * 3; i++) {
    ledData[i] = 0;
  }
  rgbState.rainbowHue = 0;
  rgbState.breathBrightness = 0;
  rgbState.breathDelta = 1;
  rgbState.chasePos = 0;
  rgbState.chaseLastTime = 0;
  rgbState.wavePhase = 0;
  rgbState.waveLastTime = 0;
  rgbState.gradientStartHue = 0;
  rgbState.gradientLastTime = 0;
  rgbState.flashState = 0;
  rgbState.flashLastTime = 0;
  rgbState.layerFlashBlinksLeft = 0;
  rgbState.layerFlashIsOn = 0;
  rgbState.layerFlashWaitTicks = 0;
  rgbState.layerFlashR = 0;
  rgbState.layerFlashG = 0;
  rgbState.layerFlashB = 0;
  effectMode = 0;
  updateLEDs();
}


// ==================== 灯效控制函数封装 ====================

// 增加亮度
void increaseBrightness() {
  currentBrightness = min(currentBrightness + 16, 255);
}

// 减少亮度
void decreaseBrightness() {
  currentBrightness = max(currentBrightness - 16, 0);
}

// 增加速度
void increaseSpeed() {
  currentSpeed = min(currentSpeed + 1, 10);
}

// 减少速度
void decreaseSpeed() {
  currentSpeed = max(currentSpeed - 1, 1);
}

// 切换灯效
void nextEffect() {
  effectMode = (effectMode + 1) % EFFECT_COUNT;
  updateLEDs();
}
