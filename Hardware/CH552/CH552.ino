#ifndef USER_USB_RAM
#error "This program needs to be compiled with a USER USB setting"
#endif

#include "src/config.h"
#include "src/KeysDataHandler.h"
#include "src/CustomUSBHID.h"
#include "src/USBHandler.h"
#include <Arduino.h>
#include <WS2812.h>



// ==================== 新增 RGB_PIN（config.h 未定义）====================
#define RGB_PIN     15      // WS2812 接 P1.5

// ==================== WS2812 LED 设置 ====================
#define NUM_LEDS    4
static __xdata uint8_t ledData[NUM_LEDS * 3];  // GRB 缓冲区

// 光效模式
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
volatile uint8_t effectMode = 0;
volatile uint8_t currentBrightness = 128; // 亮度控制 (0-255)
volatile uint8_t currentSpeed = 5; // 速度控制 (1-10)

// 全局按键状态
bool keyState[KEY_COUNT] = { false };  // 当前按键状态
bool keyPressPrev[KEY_COUNT] = { false };  // 上一次按键状态
bool funcPressedPrev = false;

// 按键检测缓冲区
bool keyBuffer[KEY_COUNT] = { false };

// 按键去抖计数器
uint8_t debounceCount[KEY_COUNT] = { 0 };
#define DEBOUNCE_THRESHOLD 5  // 去抖阈值（约50ms）

// ⚠️ 必须在全局作用域定义 keyPins，并适配 config.h 中的配置
const uint8_t keyPins[KEY_COUNT] = {
#ifdef USE_BASIC
  KEY0_PIN, KEY1_PIN, KEY2_PIN, KEY3_PIN
#endif

#ifdef USE_5KEYS
  KEY0_PIN, KEY1_PIN, KEY2_PIN, KEY3_PIN, KEY4_PIN
#endif

#ifdef USE_KNOB
  KEY0_PIN, KEY1_PIN, KEY2_PIN, KEY3_PIN, ENCODER_KEY
#endif
};

// ==================== HSV 转 RGB（SDCC 兼容）====================
void hsv_to_rgb(uint32_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b) {
  h %= 255;
  uint8_t region = h / 43;
  uint8_t remainder = (h - (region * 43)) * 6;

  uint8_t p = (v * (255 - s)) >> 8;
  uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
  uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

  switch (region) {
    case 0: *r = v; *g = t; *b = p; break;
    case 1: *r = q; *g = v; *b = p; break;
    case 2: *r = p; *g = v; *b = t; break;
    case 3: *r = p; *g = q; *b = v; break;
    case 4: *r = t; *g = p; *b = v; break;
    default: *r = v; *g = p; *b = q; break;
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
    case EFFECT_OFF:     effect_off();     break;
    case EFFECT_RAINBOW: effect_rainbow(); break;
    case EFFECT_BREATH:  effect_breath();  break;
    case EFFECT_CHASE:   effect_chase();   break;
    case EFFECT_WAVE:    effect_wave();    break;
    case EFFECT_GRADIENT: effect_gradient(); break;
    case EFFECT_FLASH:   effect_flash();   break;
  }
  neopixel_show_P1_5(ledData, NUM_LEDS * 3);
  delayMicroseconds(300);
}

// ==================== 按键扫描函数（全键无冲带去抖）====================
void scanKeys() {
  // 读取所有按键状态
  for (uint8_t i = 0; i < KEY_COUNT; i++) {
    bool currentState = !digitalRead(keyPins[i]);

    // 去抖处理
    if (currentState != keyBuffer[i]) {
      // 状态变化，重置计数器
      debounceCount[i] = 0;
      keyBuffer[i] = currentState;
    } else {
      // 状态未变化，增加计数器
      if (debounceCount[i] < DEBOUNCE_THRESHOLD) {
        debounceCount[i]++;
      } else if (debounceCount[i] == DEBOUNCE_THRESHOLD) {
        // 状态稳定，更新按键状态
        keyState[i] = currentState;
        debounceCount[i]++;
      }
    }
  }
}

// ==================== 全键无冲按键缓冲区 ====================
#define MAX_KEYS_REPORT 6  // USB HID 标准最多支持同时6个按键
static __near uint8_t keyReportBuffer[MAX_KEYS_REPORT] = {0}; // 按键报告缓冲区 (使用near存储类)
static uint8_t modReport = 0; // 修饰键报告

// ==================== 处理所有普通按键（全键无冲实现）====================
void handleCommonKeyPress() {
  scanKeys();

  // 重置报告缓冲区
  memset(keyReportBuffer, 0, sizeof(keyReportBuffer));
  modReport = 0;
  uint8_t keyCount = 0;

  // 收集所有按下的按键
  for (uint8_t i = 0; i < KEY_COUNT; i++) {
    if (keyState[i]) {
      uint16_t keyValue = getKeyValue(i);
      uint8_t keyType = getKeyType(i);

      if (keyType == KEY_TYPE_KB) {
        uint8_t keycode = keyValue & 0xFF;
        uint8_t mod = (keyValue >> 8) & 0xFF;

        // 处理修饰键
        modReport |= mod;

        // 处理普通按键（如果未超过最大报告数量）
        if (keycode != 0 && keyCount < MAX_KEYS_REPORT) {
          keyReportBuffer[keyCount++] = keycode;
        }
      } else if (keyType == KEY_TYPE_MEDIA) {
        // 媒体键仍然需要单独处理
        if (keyState[i] != keyPressPrev[i]) {
          if (keyState[i]) {
            Consumer_press(keyValue);
          } else {
            Consumer_release(keyValue);
          }
        }
      } else if (keyType == KEY_TYPE_MOUSE) {
        // 鼠标键仍然需要单独处理
        if (keyState[i] != keyPressPrev[i]) {
          uint8_t keycode = keyValue & 0xFF;
          int8_t scroll = (int8_t)((keyValue >> 8) & 0xFF);
          if (keyState[i]) {
            Mouse_press(keycode);
            Mouse_scroll(scroll);
          } else {
            Mouse_release(keycode);
          }
        }
      }
    }
    // 更新按键状态历史
    keyPressPrev[i] = keyState[i];
  }

  // 统一发送键盘报告
  Keyboard_sendReport(modReport, keyReportBuffer);
}

// ==================== 处理灯效控制 ====================
void handleEffectControl() {
  bool funcCurrent = !digitalRead(FUNC_PIN);

  // 使用 FUNC_PIN 切换灯效
  if (funcCurrent && !funcPressedPrev) {
    effectMode = (effectMode + 1) % EFFECT_COUNT;
    updateLEDs();
  }

  // 检查是否同时按下FUNC键和其他键来调整亮度和速度
  if (funcCurrent) {
    for (uint8_t i = 0; i < KEY_COUNT; i++) {
      if (keyState[i]) {
        // 按键0: 增加亮度
        if (i == 0) {
          currentBrightness = min(currentBrightness + 16, 255);
        }
        // 按键1: 减少亮度
        else if (i == 1) {
          currentBrightness = max(currentBrightness - 16, 0);
        }
        // 按键2: 增加速度
        else if (i == 2) {
          currentSpeed = min(currentSpeed + 1, 10);
        }
        // 按键3: 减少速度
        else if (i == 3) {
          currentSpeed = max(currentSpeed - 1, 1);
        }
      }
    }
  }

  funcPressedPrev = funcCurrent;
}

// ==================== 主循环 ====================
void loop() {
  handleCommonKeyPress();
  handleEffectControl();

  updateLEDs();

  digitalWrite(LED_PIN, HIGH);  // 状态 LED 常亮

  delay(10);
}

// ==================== 初始化 ====================
void setup() {
  USBInit();
  KeysDataInit();

  // 初始化所有按键引脚
  for (uint8_t i = 0; i < KEY_COUNT; i++) {
    pinMode(keyPins[i], INPUT_PULLUP);
  }
  pinMode(FUNC_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RGB_PIN, OUTPUT);

  // 初始化 WS2812
  for (int i = 0; i < NUM_LEDS * 3; i++) {
    ledData[i] = 0;
  }
  effectMode = 0;
  updateLEDs();

  // 初始化状态
  for (uint8_t i = 0; i < KEY_COUNT; i++) {
    keyPressPrev[i] = false;
    keyState[i] = false;
    keyBuffer[i] = false;
    debounceCount[i] = 0;
  }
  funcPressedPrev = false;

  digitalWrite(LED_PIN, HIGH);
}