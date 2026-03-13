#include <Arduino.h>

#include "LightingController.h"

#include "KeyScanner.h"
#include "KeysDataHandler.h"
#include "config.h"
#include "rgb.h"

static __xdata bool funcPressedPrev = false;
static __xdata bool funcUsedForCombo = false;  // FUNC 按住期间是否触发了组合操作
static __xdata bool keyPrevForFunc[KEY_COUNT]; // 追踪 FUNC 按住期间的按键边沿

void LightingController_init(void)
{
  pinMode(FUNC_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RGB_PIN, OUTPUT);

  funcPressedPrev = false;
  funcUsedForCombo = false;
  for (uint8_t i = 0; i < KEY_COUNT; i++) {
    keyPrevForFunc[i] = false;
  }
  led_init();
  digitalWrite(LED_PIN, HIGH);
}

void LightingController_process(void)
{
  bool funcCurrent = !digitalRead(FUNC_PIN);

  // FUNC 刚按下：重置组合标志
  if (funcCurrent && !funcPressedPrev)
  {
    funcUsedForCombo = false;
    for (uint8_t i = 0; i < KEY_COUNT; i++) {
      keyPrevForFunc[i] = KeyScanner_isPressed(i);
    }
  }

  // 设置全局标志，KeyScanner 据此抑制普通按键输出
  funcActive = funcCurrent;

  // FUNC 按住时：检测按键按下边沿 → 切层
  if (funcCurrent)
  {
    uint8_t layerCount = MAX_LAYERS;
    // 仅用物理主键切层（不含旋钮编码器方向）
    uint8_t switchKeyCount = KEY_COUNT;
#ifdef USE_KNOB
    switchKeyCount = 4;  // KNOB 只用前 4 个物理键切层
#endif
    if (switchKeyCount > layerCount) switchKeyCount = layerCount;

    for (uint8_t i = 0; i < switchKeyCount; i++)
    {
      bool pressed = KeyScanner_isPressed(i);
      if (pressed && !keyPrevForFunc[i])
      {
        // 按下边沿 → 切到对应层
        if (i != getCurrentLayer()) {
          setCurrentLayer(i);
          flashLayerColor(i);
        }
        funcUsedForCombo = true;
      }
      keyPrevForFunc[i] = pressed;
    }
  }

  // FUNC 释放且期间没有触发组合 → 单按 = 切换 RGB 灯效
  if (!funcCurrent && funcPressedPrev && !funcUsedForCombo)
  {
    nextEffect();
  }

  funcPressedPrev = funcCurrent;
  updateLEDs();
  digitalWrite(LED_PIN, HIGH);
}
