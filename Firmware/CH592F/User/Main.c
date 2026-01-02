#include "CH59x_common.h"
#include "usb_device.h"
#include "usb_hid.h"
#include "key.h"
#include "hal_utils.h"
#include <string.h>

/**
 * @brief
 */
__INTERRUPT

__HIGH_CODE
void USB_IRQHandler (void) {
    USB_Device_TransferProcess();
}

int main() {
    SetSysClock (CLK_SOURCE_PLL_60MHz);

    USB_Device_Init();
    DelayMs(100);
    Key_Init();
    DelayMs(100);

    key_event_t evt;
    fnkey_event_t fn_evt;
    while (1) {
        // 处理按键
        while (Key_GetEvent (&evt)) {
            if (evt.type == KEY_EVT_PRESS) {
                // 按下处理
                // USB_Keyboard_Type (0x00, KEY_A);
            } else {
                // 释放处理
            }
        }

        // 功能键
        while (FnKey_GetEvent (&fn_evt)) {
            switch (fn_evt.id) {

            case FNKEY_1:
                if (fn_evt.type == FNKEY_EVT_CLICK) {
                    USB_Keyboard_Type (0x00, KEY_A);
                } else {
                    USB_Keyboard_Type (0x00, KEY_B);
                    Hal_JumpToBootloader();
                }

                break;

            case FNKEY_2:
                if (fn_evt.type == FNKEY_EVT_CLICK) {
                    USB_Keyboard_Type (0x00, KEY_C);
                } else {
                    USB_Keyboard_Type (0x00, KEY_D);
                }
                break;
            }
        }
    }
}