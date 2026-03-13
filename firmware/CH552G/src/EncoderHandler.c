#include <Arduino.h>

#include "EncoderHandler.h"

#include "CustomUSBHID.h"
#include "KeyScanner.h"
#include "KeysDataHandler.h"
#include "config.h"

#ifdef USE_KNOB
static bool encoderLeftPrev = false;

static void handleEncoderAction(uint8_t keyType, uint16_t keyValue)
{
  if (keyType == KEY_TYPE_MEDIA)
  {
    Consumer_write(keyValue);
  }
  else if (keyType == KEY_TYPE_KB)
  {
    uint8_t keycode = keyValue & 0xFF;
    uint8_t mod = (keyValue >> 8) & 0xFF;
    Keyboard_rawPress(keycode, mod);
    Keyboard_rawRelease(keycode, mod);
  }
  else if (keyType == KEY_TYPE_MOUSE)
  {
    uint8_t keycode = keyValue & 0xFF;
    int8_t scroll = (int8_t)((keyValue >> 8) & 0xFF);

    if (keycode != 0)
    {
      Mouse_press(keycode);
      Mouse_release(keycode);
    }
    if (scroll != 0)
    {
      Mouse_scroll(scroll);
    }
  }
}
#endif

void EncoderHandler_init(void)
{
#ifdef USE_KNOB
  pinMode(ENCODER_LEFT, INPUT_PULLUP);
  pinMode(ENCODER_RIGHT, INPUT_PULLUP);
  encoderLeftPrev = digitalRead(ENCODER_LEFT);
#endif
}

void EncoderHandler_process(void)
{
#ifdef USE_KNOB
  // FUNC 按住时不处理旋钮
  if (funcActive) {
    encoderLeftPrev = digitalRead(ENCODER_LEFT);
    return;
  }

  bool encoderLeft = digitalRead(ENCODER_LEFT);

  if (encoderLeftPrev && !encoderLeft)
  {
    uint8_t encoderIndex =
      digitalRead(ENCODER_RIGHT) ? ENCODER_LEFT_INDEX : ENCODER_RIGHT_INDEX;
    // 从当前层读取旋钮映射
    handleEncoderAction(getKeyType(encoderIndex), getKeyValue(encoderIndex));
  }

  encoderLeftPrev = encoderLeft;
#endif
}
