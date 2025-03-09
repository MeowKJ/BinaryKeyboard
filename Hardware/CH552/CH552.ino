#ifndef USER_USB_RAM
#error "This program needs to be compiled with a USER USB setting"
#endif

#include "src/config.h"
#include "src/KeysDataHandler.h"
#include "src/CustomUSBHID.h"
#include "src/USBHandler.h"
#include <Arduino.h>  // 提供 millis()

#ifdef USE_KNOB
  const uint8_t keyPins[KEY_COUNT] = { KEY0_PIN, KEY1_PIN, KEY2_PIN, KEY3_PIN, ENCODER_KEY };
  const uint8_t key = KEY_HOME;
#endif

#ifdef USE_BASIC
  const uint8_t keyPins[KEY_COUNT] = { KEY0_PIN, KEY1_PIN, KEY2_PIN, KEY3_PIN };
#endif  

#ifdef USE_5KEYS
  const uint8_t keyPins[KEY_COUNT] = {KEY0_PIN, KEY1_PIN, KEY2_PIN, KEY3_PIN, KEY4_PIN};
#endif

// 全局按键状态变量
bool keyPressPrev[KEY_COUNT] = { false };

/**
 * @brief 处理按键状态变化
 */
void handleCommonKeyPress() {
  for (uint8_t i = 0; i < KEY_COUNT; i++) {
    bool currentKeyState = !digitalRead(keyPins[i]);
    if (currentKeyState != keyPressPrev[i]) {
      keyPressPrev[i] = currentKeyState;
      uint16_t keyValue = getKeyValue(i);
      uint8_t keyType = getKeyType(i);

      if (keyType == KEY_TYPE_KB) {
        uint8_t keycode = keyValue & 0xFF;  // 取低8位
        uint8_t mod = (keyValue >> 8) & 0xFF;
        if (currentKeyState) {
          Keyboard_rawPress(keycode, mod);
        } else {
          Keyboard_rawRelease(keycode, mod);
        }
      } else if (keyType == KEY_TYPE_MEDIA) {
        if (currentKeyState) {
          Consumer_press(keyValue);
        } else {
          Consumer_release(keyValue);
        }
      } else if (keyType == KEY_TYPE_MOUSE) {
        uint8_t keycode = keyValue & 0xFF;
        int8_t scroll = (int8_t)((keyValue >> 8) & 0xFF);
        if (currentKeyState) {
          Mouse_press(keycode);
          Mouse_scroll(scroll);
        } else {
          Mouse_release(keycode);
        }
      }
    }
  }
}

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

void setup() {
  USBInit();
  KeysDataInit();
  pinMode(LED_PIN, OUTPUT);

  // 初始化按键引脚
  for (uint8_t i = 0; i < KEY_COUNT; i++) {
    pinMode(keyPins[i], INPUT_PULLUP);
  }

#ifdef USE_KNOB
  pinMode(ENCODER_LEFT, INPUT_PULLUP);
  pinMode(ENCODER_RIGHT, INPUT_PULLUP);
#endif
}

void loop() {
  handleCommonKeyPress();

#ifdef USE_KNOB
  handleEncoderRotation();
#endif
}
