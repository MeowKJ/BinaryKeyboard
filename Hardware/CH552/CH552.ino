#ifndef USER_USB_RAM
#error "This program needs to be compiled with a USER USB setting"
#endif
#include <Arduino.h>

#include "src/config.h"
#include "src/KeysDataHandler.h"
#include "src/CustomUSBHID.h"
#include "src/USBHandler.h"
#include "src/rgb.h"



// 全局按键状态
bool keyState[KEY_COUNT] = { false };  // 当前按键状态
bool keyPressPrev[KEY_COUNT] = { false };  // 上一次按键状态
bool funcPressedPrev = false;

// 按键检测缓冲区
bool keyBuffer[KEY_COUNT] = { false };

// 按键去抖计数器
uint8_t debounceCount[KEY_COUNT] = { 0 };

// ⚠️如果您准备将键盘用于节奏游戏场景，可将去抖阈值设为0
#define DEBOUNCE_THRESHOLD 5  // 去抖阈值（约50ms）

//RGB



// 必须在全局作用域定义 keyPins，并适配 config.h 中的配置
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

// ==================== 旋钮处理 ====================
#ifdef USE_KNOB
// 编码器旋转状态变量
bool encoderLeftPrev = false;

/**
 * @brief 处理编码器旋转事件
 */
void handleEncoderRotation() {
  bool encoderLeft = digitalRead(ENCODER_LEFT);
  // 检测下降沿
  if (encoderLeftPrev && !encoderLeft) {
    // 根据 ENCODER_RIGHT 的电平判断旋转方向
    if (digitalRead(ENCODER_RIGHT)) {
      uint8_t encoderLeftType = getKeyType(ENCODER_LEFT_INDEX);
      uint16_t encoderLeftValue = getKeyValue(ENCODER_LEFT_INDEX);
      if (encoderLeftType == KEY_TYPE_MEDIA) {
        Consumer_write(encoderLeftValue);
      } else if (encoderLeftType == KEY_TYPE_KB) {
        uint8_t keycode = encoderLeftValue & 0xFF;  // 取低8位
        uint8_t mod = (encoderLeftValue >> 8) & 0xFF;
        Keyboard_rawPress(keycode, mod);
        Keyboard_rawRelease(keycode, mod);
      } else if (encoderLeftType == KEY_TYPE_MOUSE) {
        uint8_t keycode = encoderLeftValue & 0xFF;
        int8_t scroll = (int8_t)((encoderLeftValue >> 8) & 0xFF);
        Mouse_press(keycode);
        Mouse_release(keycode);
        Mouse_scroll(scroll);
      }
    } else {
      uint8_t encoderRightType = getKeyType(ENCODER_RIGHT_INDEX);
      uint16_t encoderRightValue = getKeyValue(ENCODER_RIGHT_INDEX);
      if (encoderRightType == KEY_TYPE_MEDIA) {
        Consumer_write(encoderRightValue);
      } else if (encoderRightType == KEY_TYPE_KB) {
        uint8_t keycode = encoderRightValue & 0xFF;  // 取低8位
        uint8_t mod = (encoderRightValue >> 8) & 0xFF;
        Keyboard_rawPress(keycode, mod);
        Keyboard_rawRelease(keycode, mod);
      } else if (encoderRightType == KEY_TYPE_MOUSE) {
        uint8_t keycode = encoderRightValue & 0xFF;
        int8_t scroll = (int8_t)((encoderRightValue >> 8) & 0xFF);
        Mouse_press(keycode);
        Mouse_release(keycode);
        Mouse_scroll(scroll);
      }
    }
  }
  encoderLeftPrev = encoderLeft;  // 更新状态

}
#endif

// ==================== 处理灯效控制 ====================
void handleEffectControl() {
  bool funcCurrent = !digitalRead(FUNC_PIN);

  // 使用 FUNC_PIN 切换灯效
  if (funcCurrent && !funcPressedPrev) {
    nextEffect();
  }
  // FUNC键按下时调整亮度和速度
  if (funcCurrent) {
    for (uint8_t i = 0; i < KEY_COUNT; i++) {
      if (keyState[i]) {
        switch (i) {
          case 0: increaseBrightness(); break;
          case 1: decreaseBrightness(); break;
          case 2: increaseSpeed(); break;
          case 3: decreaseSpeed(); break;
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
#ifdef USE_KNOB
  handleEncoderRotation();
#endif
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

  // 初始化旋钮款编码器
#ifdef USE_KNOB
  pinMode(ENCODER_LEFT, INPUT_PULLUP);
  pinMode(ENCODER_RIGHT, INPUT_PULLUP);
#endif

  // 初始化 WS2812
  led_init();

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