/********************************** (C) COPYRIGHT *******************************
 * File Name          : Main.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2020/08/06
 * Description        : 串口1收发演示
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#include "CH59x_common.h"
#include "usb_device.h"
#include "usb_hid.h"
#include <string.h>

/**
 * @brief USB 中断服务函数
 */
__INTERRUPT

__HIGH_CODE
void USB_IRQHandler (void) {
    USB_Device_TransferProcess();
}

/*********************************************************************
 * @fn      main
 *
 * @brief   主函数
 *
 * @return  none
 */
int main() {
    SetSysClock (CLK_SOURCE_PLL_60MHz);

    USB_Device_Init();
    PFIC_EnableIRQ (USB_IRQn);

    while (1);
}
