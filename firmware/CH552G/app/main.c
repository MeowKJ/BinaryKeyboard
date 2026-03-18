#ifndef USER_USB_RAM
#error "This program needs to be compiled with a USER USB setting"
#endif

#include "ch552_defs.h"

#include "CustomUSBHID.h"
#include "EncoderHandler.h"
#include "KeyScanner.h"
#include "KeysDataHandler.h"
#include "LightingController.h"
#include "MacroStorage.h"

void loop(void)
{
  KeyScanner_process();
  macro_step();
  LightingController_process();
  EncoderHandler_process();
  delay(1);
}

void setup(void)
{
  USBInit();
  KeysDataInit();
  KeyScanner_init();
  LightingController_init();
  EncoderHandler_init();
}
