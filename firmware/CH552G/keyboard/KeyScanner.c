#include <Arduino.h>
#include <string.h>

#include "KeyScanner.h"

#include "config.h"
#include "CustomUSBHID.h"
#include "KeysDataHandler.h"
#include "MacroStorage.h"
#include "rgb.h"

#define KEY_SCANNER_DEBOUNCE_THRESHOLD 3
#define KEY_SCANNER_MAX_REPORT_KEYS 6

volatile bool funcActive = false;

static __xdata bool keyState[KEY_COUNT];
static __xdata bool keyPressPrev[KEY_COUNT];
static __xdata bool keyBuffer[KEY_COUNT];
static __xdata uint8_t debounceCount[KEY_COUNT];
static __near uint8_t keyReportBuffer[KEY_SCANNER_MAX_REPORT_KEYS] = {0};
static __near uint8_t prevKeyReportBuffer[KEY_SCANNER_MAX_REPORT_KEYS] = {0};
static uint8_t modReport = 0;
static uint8_t prevModReport = 0;

static const uint8_t keyPins[KEY_COUNT] = {
#ifdef USE_BASIC
    KEY0_PIN, KEY1_PIN, KEY2_PIN, KEY3_PIN
#endif
#ifdef USE_5KEYS
                                      KEY0_PIN,
    KEY1_PIN, KEY2_PIN, KEY3_PIN, KEY4_PIN
#endif
#ifdef USE_KNOB
                                      KEY0_PIN,
    KEY1_PIN, KEY2_PIN, KEY3_PIN, ENCODER_KEY
#endif
};

static void scanKeys(void)
{
  for (uint8_t i = 0; i < KEY_COUNT; i++)
  {
    bool currentState = !digitalRead(keyPins[i]);

    if (currentState != keyBuffer[i])
    {
      debounceCount[i] = 0;
      keyBuffer[i] = currentState;
    }
    else if (debounceCount[i] < KEY_SCANNER_DEBOUNCE_THRESHOLD)
    {
      debounceCount[i]++;
    }
    else if (debounceCount[i] == KEY_SCANNER_DEBOUNCE_THRESHOLD)
    {
      keyState[i] = currentState;
      debounceCount[i]++;
    }
  }
}

void KeyScanner_init(void)
{
  for (uint8_t i = 0; i < KEY_COUNT; i++)
  {
    pinMode(keyPins[i], INPUT_PULLUP);
    keyState[i] = false;
    keyPressPrev[i] = false;
    keyBuffer[i] = false;
    debounceCount[i] = 0;
  }
}

void KeyScanner_process(void)
{
  funcActive = !digitalRead(FUNC_PIN);
  scanKeys();

  // 功能键按住时不发送普通按键报告
  if (funcActive)
  {
    // 取消正在运行的宏
    if (macro_is_running())
      macro_cancel();
    // 更新 prev 状态以避免释放功能键后误触发
    for (uint8_t i = 0; i < KEY_COUNT; i++)
    {
      keyPressPrev[i] = keyState[i];
    }
    // 发送空报告释放所有按键
    Keyboard_releaseAll();
    Consumer_releaseAll();
    Mouse_releaseAll();
    return;
  }

  // 宏运行中: 只跟踪触发键状态
  if (macro_is_running())
  {
    uint8_t mk = macro_running_key();
    if (keyState[mk] && !keyPressPrev[mk] && macro_is_looping())
    {
      if (macro_m_trigger == MACRO_TRIG_TOGGLE)
        macro_cancel();
    }
    if (!keyState[mk] && keyPressPrev[mk])
      macro_on_key_release();
    for (uint8_t i = 0; i < KEY_COUNT; i++)
      keyPressPrev[i] = keyState[i];
    return;
  }

  memset(keyReportBuffer, 0, sizeof(keyReportBuffer));
  modReport = 0;
  uint8_t keyCount = 0;

  for (uint8_t i = 0; i < KEY_COUNT; i++)
  {
    uint16_t keyValue = getKeyValue(i);
    uint8_t keyType = getKeyType(i);

    if (keyState[i] && !keyPressPrev[i])
    {
      rgbRegisterKeyPress(i);
    }

    if (keyType == KEY_TYPE_MACRO)
    {
      if (keyState[i] && !keyPressPrev[i])
      {
        uint8_t trigger = (keyValue >> 8) & 0xFF;
        uint8_t slot = keyValue & 0xFF;
        macro_execute(slot, trigger, i);
      }
      keyPressPrev[i] = keyState[i];
      continue;
    }

    if (keyType == KEY_TYPE_KB)
    {
      if (keyState[i])
      {
        uint8_t keycode = keyValue & 0xFF;
        uint8_t mod = (keyValue >> 8) & 0xFF;

        modReport |= mod;
        if (keycode != 0 && keyCount < KEY_SCANNER_MAX_REPORT_KEYS)
        {
          keyReportBuffer[keyCount++] = keycode;
        }
      }
    }
    else if (keyType == KEY_TYPE_MEDIA)
    {
      if (keyState[i] != keyPressPrev[i])
      {
        if (keyState[i])
        {
          Consumer_press(keyValue);
        }
        else
        {
          Consumer_release(keyValue);
        }
      }
    }
    else if (keyType == KEY_TYPE_MOUSE)
    {
      uint8_t keycode = keyValue & 0xFF;
      int8_t scroll = (int8_t)((keyValue >> 8) & 0xFF);

      if (keyState[i] != keyPressPrev[i])
      {
        if (keyState[i])
        {
          if (keycode != 0)
          {
            Mouse_press(keycode);
          }
          if (scroll != 0)
          {
            Mouse_scroll(scroll);
          }
        }
        else if (keycode != 0)
        {
          Mouse_release(keycode);
        }
      }

      if (keyState[i] && keycode != 0 && keyState[i] == keyPressPrev[i])
      {
        Mouse_press(keycode);
      }
    }

    keyPressPrev[i] = keyState[i];
  }

  // 仅在键盘报告变化时发送，避免无谓 USB 流量
  if (modReport != prevModReport ||
      memcmp(keyReportBuffer, prevKeyReportBuffer, KEY_SCANNER_MAX_REPORT_KEYS) != 0)
  {
    Keyboard_sendReport(modReport, keyReportBuffer);
    prevModReport = modReport;
    memcpy(prevKeyReportBuffer, keyReportBuffer, KEY_SCANNER_MAX_REPORT_KEYS);
  }
}

bool KeyScanner_isPressed(uint8_t index)
{
  return (index < KEY_COUNT) ? keyState[index] : false;
}
