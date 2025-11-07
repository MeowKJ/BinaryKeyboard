/********************************** USB Device Layer Header *******************
 * File Name          : usb_device.h
 * Author             : Custom USB Library
 * Version            : V2.0
 * Date               : 2024/11/07
 * Description        : USB 设备层接口（基于 WCH 官方库）
 *******************************************************************************/

#ifndef __USB_DEVICE_H__
#define __USB_DEVICE_H__

#include "CH59x_common.h"
#include "CH59x_usbdev.h"
#include "usb_descriptors.h"
#include "usb_hid.h"

/* ==================== USB Device State ==================== */
typedef enum {
    USB_STATE_DETACHED = 0,
    USB_STATE_ATTACHED,
    USB_STATE_POWERED,
    USB_STATE_DEFAULT,
    USB_STATE_ADDRESS,
    USB_STATE_CONFIGURED,
    USB_STATE_SUSPENDED
} USB_DeviceState_t;

/* ==================== Global Variables ==================== */
extern USB_DeviceState_t g_USB_DeviceState;
extern uint8_t g_USB_DeviceConfig;
extern uint8_t g_USB_DeviceAddress;
extern uint8_t g_USB_SleepStatus;

/* EP0 控制传输相关 */
extern uint8_t  g_SetupReqCode;
extern uint16_t g_SetupReqLen;
extern const uint8_t *g_pDescriptor;

/* Idle 和 Protocol 设置 (每个接口独立) */
extern uint8_t g_IdleValue[INTF_COUNT];
extern uint8_t g_ProtocolValue[INTF_COUNT];

/* ==================== Function Prototypes ==================== */

/**
 * @brief USB 设备初始化（包含底层和应用层）
 */
void USB_Device_Init(void);

/**
 * @brief USB 设备传输处理 (在中断中调用)
 */
void USB_Device_TransferProcess(void);

/**
 * @brief USB 设备唤醒主机
 */
void USB_Device_Wakeup(void);

/**
 * @brief 检查端点是否可以发送数据
 */
uint8_t USB_Device_EP1_Ready(void);
uint8_t USB_Device_EP2_Ready(void);
uint8_t USB_Device_EP3_Ready(void);
uint8_t USB_Device_EP4_Ready(void);

#endif /* __USB_DEVICE_H__ */