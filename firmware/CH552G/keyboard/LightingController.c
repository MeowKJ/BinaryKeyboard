#include "ch552_defs.h"

#include "LightingController.h"

#include "KeyScanner.h"
#include "KeysDataHandler.h"
#include "config.h"
#include "rgb.h"

static __xdata bool funcPressedPrev = false;
static __xdata bool funcUsedWhileHeld = false;
static __xdata bool keyPrevForFunc[KEY_COUNT];

void LightingController_init(void)
{
  pinMode(FUNC_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RGB_PIN, OUTPUT);

  funcPressedPrev = false;
  funcUsedWhileHeld = false;
  for (uint8_t i = 0; i < KEY_COUNT; i++)
  {
    keyPrevForFunc[i] = false;
  }
  led_init();
  digitalWrite(LED_PIN, HIGH);
}

void LightingController_process(void)
{
  bool funcCurrent = funcActive;

  if (funcCurrent && !funcPressedPrev)
  {
    funcUsedWhileHeld = false;
    for (uint8_t i = 0; i < KEY_COUNT; i++)
    {
      keyPrevForFunc[i] = KeyScanner_isPressed(i);
    }
  }

  if (funcCurrent)
  {
    for (uint8_t i = 0; i < KEY_COUNT; i++)
    {
      bool pressed = KeyScanner_isPressed(i);
      if (pressed && !keyPrevForFunc[i])
      {
        funcUsedWhileHeld = true;
        /* FN + 按键 = 切换到对应层 */
        if (i < LAYER_SWITCH_KEY_COUNT && i < MAX_LAYERS)
        {
          setCurrentLayer(i);
          flashLayerColor(i, i);
        }
      }
      keyPrevForFunc[i] = pressed;
    }
  }

  if (!funcCurrent && funcPressedPrev && !funcUsedWhileHeld)
  {
    nextEffect();
  }

  funcPressedPrev = funcCurrent;

  static uint16_t lastLedUpdate = 0;
  uint16_t now = (uint16_t)millis();

  // 60fps: updateLEDs 内部隔帧推进背景动画（背景30fps，overlay+刷新60fps）
  if ((uint16_t)(now - lastLedUpdate) >= 16)
  {
    lastLedUpdate = now;
    updateLEDs();
  }

  digitalWrite(LED_PIN, funcCurrent ? LOW : HIGH);
}
