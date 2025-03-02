#ifndef USER_USB_RAM
#error "This program needs to be compiled with a USER USB setting"
#endif

#include "config.h"
#include "src/KeysDataHandler/KeysDataHandler.h"
#include "src/CustomUSBHID/CustomUSBHID.h"

#ifdef USE_KNOB

#define KEY_COUNT 5

const uint8_t keyPins[KEY_COUNT] = { KEY0_PIN, KEY1_PIN, KEY2_PIN, KEY3_PIN, ENCODER_KEY };
bool encoderLeftPrev = false;

#endif


bool keyPressPrev[KEY_COUNT] = { false };


/**
 * @brief 处理按键状态变化
 */
void handleCommonKeyPress() {
  for (uint8_t i = 0; i < KEY_COUNT; i++) {
    bool keyPress = !digitalRead(keyPins[i]);

    if (keyPressPrev[i] != keyPress) {
      keyPressPrev[i] = keyPress;
      uint16_t keyValue = getKeyValue(i);
      uint8_t keyType = getKeyType(i);

      if (keyType == KEY_TYPE_KB) {
        // 键盘
        keyPress ? Keyboard_press((uint8_t)keyValue) : Keyboard_release((uint8_t)keyValue);

      } else if (keyType == KEY_TYPE_MEDIA) {
        //媒体
        keyPress ? Consumer_press(keyValue) : Consumer_release(keyValue);

      } else if (keyType == KEY_TYPE_MOUSE) {
        // 鼠标
        keyPress ? Mouse_press((uint8_t)keyValue) : Mouse_release((uint8_t)keyValue);
      }
    }
  }
}


#ifdef USE_KNOB
/**
 * @brief 处理编码器旋转事件
 */
void handleEncoderRotation() {
  bool encoderLeft = digitalRead(ENCODER_LEFT);
  if (encoderLeftPrev && !encoderLeft)  // 检测从1变0
  {
    if (digitalRead(ENCODER_RIGHT)) {
      uint16_t encoderLeftValue = getKeyValue(ENCODER_LEFT_INDEX);
      uint8_t encoderLeftType = getKeyType(ENCODER_LEFT_INDEX);
      if (encoderLeftType == KEY_TYPE_MEDIA) {
        Consumer_write(encoderLeftValue);
      } else if (encoderLeftType == KEY_TYPE_KB) {
        Keyboard_write((uint8_t)encoderLeftValue);
      } else if (encoderLeftType == KEY_TYPE_MOUSE) {
        Mouse_click((uint8_t)encoderLeftValue);
      }
    } else {
      uint16_t encoderRightValue = getKeyValue(ENCODER_RIGHT_INDEX);
      uint8_t encoderRightType = getKeyType(ENCODER_RIGHT_INDEX);
      if (encoderRightType == KEY_TYPE_MEDIA) {
        Consumer_write(encoderRightValue);
      } else if (encoderRightType == KEY_TYPE_KB) {
        Keyboard_write((uint8_t)encoderRightValue);
      } else if (encoderRightType == KEY_TYPE_MOUSE) {
        Mouse_click((uint8_t)encoderRightValue);
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

  // 初始化按键
  for (uint8_t i = 0; i < KEY_COUNT; i++) {
    pinMode(keyPins[i], INPUT_PULLUP);
  }

#ifdef USE_KNOB
  pinMode(ENCODER_LEFT, INPUT_PULLUP);
  pinMode(ENCODER_RIGHT, INPUT_PULLUP);
#endif
}

void loop() {
  handleCommonKeyPress();  // 处理基础按键

#ifdef USE_KNOB
  handleEncoderRotation();
#endif

  // // LED状态指示（心跳功能）
  // static uint32_t ledTimer = 0;
  // if (millis() - ledTimer > 1000) {
  //   digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  //   ledTimer = millis();
  // }
}
