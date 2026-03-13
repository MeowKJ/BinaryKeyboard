#include <Arduino.h>

#include "LightingController.h"

#include "KeyScanner.h"
#include "config.h"
#include "rgb.h"

static bool funcPressedPrev = false;

void LightingController_init(void)
{
  pinMode(FUNC_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RGB_PIN, OUTPUT);

  funcPressedPrev = false;
  led_init();
  digitalWrite(LED_PIN, HIGH);
}

void LightingController_process(void)
{
  bool funcCurrent = !digitalRead(FUNC_PIN);

  if (funcCurrent && !funcPressedPrev)
  {
    nextEffect();
  }

  if (funcCurrent)
  {
    for (uint8_t i = 0; i < KEY_COUNT; i++)
    {
      if (KeyScanner_isPressed(i))
      {
        switch (i)
        {
        case 0:
          increaseBrightness();
          break;
        case 1:
          decreaseBrightness();
          break;
        case 2:
          increaseSpeed();
          break;
        case 3:
          decreaseSpeed();
          break;
        default:
          break;
        }
      }
    }
  }

  funcPressedPrev = funcCurrent;
  updateLEDs();
  digitalWrite(LED_PIN, HIGH);
}
