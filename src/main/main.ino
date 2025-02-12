#ifndef USER_USB_RAM
#error "This program needs to be compiled with a USER USB setting"
#endif

#include "USBHIDKeyboard.h"

// Pin define
#define LED_PIN 11

#define KEY0_PIN 33
#define KEY1_PIN 34
#define KEY2_PIN 31
#define KEY3_PIN 30
#define FUNC_PIN 32

// 状态变量
bool key0PressPrev = false;
bool key1PressPrev = false;
bool key2PressPrev = false;
bool key3PressPrev = false;

void setup() {
  USBInit();
  pinMode(LED_PIN, OUTPUT);
  pinMode(KEY0_PIN, INPUT_PULLUP);
  pinMode(KEY1_PIN, INPUT_PULLUP);
  pinMode(KEY2_PIN, INPUT_PULLUP);
  pinMode(KEY3_PIN, INPUT_PULLUP);
}

void loop() {
  bool key0Press = !digitalRead(KEY0_PIN);
  if (key0PressPrev != key0Press) {
    key0PressPrev = key0Press;
    key0Press ? Keyboard_press('0') : Keyboard_release('0');
  }

  bool key1Press = !digitalRead(KEY1_PIN);
  if (key1PressPrev != key1Press) {
    key1PressPrev = key1Press;
    key1Press ? Keyboard_press('1') : Keyboard_release('1');
  }

  bool key2Press = !digitalRead(KEY2_PIN);
  if (key2PressPrev != key2Press) {
    key2PressPrev = key2Press;
    key2Press ? Keyboard_press(' ') : Keyboard_release(' ');
  }

  bool key3Press = !digitalRead(KEY3_PIN);
  if (key3PressPrev != key3Press) {
    key3PressPrev = key3Press;
    if (key3Press) {
      digitalWrite(LED_PIN, LOW);
      Keyboard_press(KEY_RETURN);
    } else {
      digitalWrite(LED_PIN, HIGH);
      Keyboard_release(KEY_RETURN);
    }
  }
}
