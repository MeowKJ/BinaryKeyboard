#include "rgb.h"
#include "config.h"

volatile uint8_t effectMode = 0;
volatile uint8_t currentBrightness = 128; // 亮度控制 (0-255)
volatile uint8_t currentSpeed = 5; // 速度控制 (1-10)

// ==================== WS2812 LED 设置 ====================
static __xdata uint8_t ledData[NUM_LEDS * 3];  // GRB 缓冲区


// ==================== HSV 转 RGB（SDCC 兼容）====================
void hsv_to_rgb(uint32_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b) {
  h %= 255;
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
  for (int i = 0; i < NUM_LEDS; i++) {
    set_pixel_for_GRB_LED(ledData, i, 0, 0, 0);
  }
}

void effect_rainbow() {
  static uint8_t hue = 0;
  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t r, g, b;
    hsv_to_rgb(hue + i * 64, 255, currentBrightness, &r, &g, &b);
    set_pixel_for_GRB_LED(ledData, i, g, r, b);
  }
  hue++;
}

void effect_breath() {
  static uint8_t brightness = 0;
  static int8_t delta = 1;
  brightness += delta;
  if (brightness == 0 || brightness == 255) delta = -delta;

  uint8_t v = brightness / 3;
  for (int i = 0; i < NUM_LEDS; i++) {
    set_pixel_for_GRB_LED(ledData, i, v, v, v);
  }
}

void effect_chase() {
  static uint8_t pos = 0;
  static uint32_t lastTime = 0;
  uint32_t now = millis();
  if (now - lastTime > 300 / currentSpeed) {
    pos = (pos + 1) % NUM_LEDS;
    lastTime = now;
  }
  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t val = (i == pos) ? currentBrightness : currentBrightness / 6;
    set_pixel_for_GRB_LED(ledData, i, 0, val, 0);
  }
}

void effect_wave() {
  static uint8_t phase = 0;
  static uint32_t lastTime = 0;
  uint32_t now = millis();
  if (now - lastTime > 50 / currentSpeed) {
    phase++;
    lastTime = now;
  }
  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t r, g, b;
    uint8_t hue = phase + i * 64;
    hsv_to_rgb(hue, 255, currentBrightness, &r, &g, &b);
    set_pixel_for_GRB_LED(ledData, i, g, r, b);
  }
}

void effect_gradient() {
  static uint8_t startHue = 0;
  static uint32_t lastTime = 0;
  uint32_t now = millis();
  if (now - lastTime > 100 / currentSpeed) {
    startHue++;
    lastTime = now;
  }
  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t r, g, b;
    uint8_t hue = startHue + (i * 255 / NUM_LEDS);
    hsv_to_rgb(hue, 255, currentBrightness, &r, &g, &b);
    set_pixel_for_GRB_LED(ledData, i, g, r, b);
  }
}

void effect_flash() {
  static uint8_t flashState = 0;
  static uint32_t lastTime = 0;
  uint32_t now = millis();
  if (now - lastTime > 200 / currentSpeed) {
    flashState = !flashState;
    lastTime = now;
  }
  uint8_t brightness = flashState ? currentBrightness : 0;
  for (int i = 0; i < NUM_LEDS; i++) {
    set_pixel_for_GRB_LED(ledData, i, brightness, brightness, brightness);
  }
}

void updateLEDs() {
  switch (effectMode) {
    case EFFECT_OFF: effect_off(); break;
    case EFFECT_RAINBOW: effect_rainbow(); break;
    case EFFECT_BREATH: effect_breath(); break;
    case EFFECT_CHASE: effect_chase(); break;
    case EFFECT_WAVE: effect_wave(); break;
    case EFFECT_GRADIENT: effect_gradient(); break;
    case EFFECT_FLASH: effect_flash(); break;
  }
  neopixel_show_P1_5(ledData, NUM_LEDS * 3);
  delayMicroseconds(300);
}



void led_init() {
  for (int i = 0; i < NUM_LEDS * 3; i++) {
    ledData[i] = 0;
  }
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

