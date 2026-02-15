/********************************** USB Descriptors Header ********************
 * File Name          : usb_descriptors.h
 * Author             : Custom USB Library
 * Version            : V2.0
 * Date               : 2024/11/07
 * Description        : USB 描述符定义（基于 WCH 官方库）
 *******************************************************************************/

#ifndef __USB_DESCRIPTORS_H__
#define __USB_DESCRIPTORS_H__

#include "CH59x_common.h"

/* USB 设备配置 */
#define DevEP0SIZE              0x40
#define USB_VID                 0x413D  // Vendor ID
#define USB_PID                 0x2107  // Product ID

/* 接口编号定义 */
#define INTF_KEYBOARD           0
#define INTF_MOUSE              1
#define INTF_CONSUMER           2
#define INTF_CONFIG             3
#define INTF_COUNT              4

/* 端点地址定义 */
#define EP_KEYBOARD_IN          0x81    // EP1 IN - 键盘
#define EP_MOUSE_IN             0x82    // EP2 IN - 鼠标
#define EP_CONSUMER_IN          0x83    // EP3 IN - 多媒体
#define EP_CONFIG_IN            0x84    // EP4 IN - 配置
#define EP_CONFIG_OUT           0x04    // EP4 OUT - 配置

/* HID 报告长度定义 */
#define HID_KEYBOARD_REPORT_SIZE    8
#define HID_MOUSE_REPORT_SIZE       4
#define HID_CONSUMER_REPORT_SIZE    2
#define HID_CONFIG_REPORT_SIZE      64

/* HID 报告描述符长度 */
#define HID_KEYBOARD_REPORT_DESC_SIZE   64
#define HID_MOUSE_REPORT_DESC_SIZE      52
#define HID_CONSUMER_REPORT_DESC_SIZE   23
#define HID_CONFIG_REPORT_DESC_SIZE     34


/* USB Qualifier 描述符长度 */
#define USB_QUALIFIER_DESC_SIZE         10

#define HID_KeyboardReportDescSize      HID_KEYBOARD_REPORT_DESC_SIZE
#define HID_MouseReportDescSize         HID_MOUSE_REPORT_DESC_SIZE
#define HID_ConsumerReportDescSize      HID_CONSUMER_REPORT_DESC_SIZE
#define HID_ConfigReportDescSize        HID_CONFIG_REPORT_DESC_SIZE

/* 配置描述符总长度 */
#define USB_CONFIG_DESC_SIZE    (9 + \
                                 9 + 9 + 7 + \
                                 9 + 9 + 7 + \
                                 9 + 9 + 7 + \
                                 9 + 9 + 7 + 7)

/* 外部声明 */
extern const uint8_t USB_DeviceDescriptor[];
extern const uint8_t USB_ConfigDescriptor[];
extern const uint8_t USB_StringLangID[];
extern const uint8_t USB_StringVendor[];
extern const uint8_t USB_StringProduct[];
extern const uint8_t USB_QualifierDescriptor[];

/* HID 报告描述符 */
extern const uint8_t HID_KeyboardReportDescriptor[];
extern const uint8_t HID_MouseReportDescriptor[];
extern const uint8_t HID_ConsumerReportDescriptor[];
extern const uint8_t HID_ConfigReportDescriptor[];



#endif /* __USB_DESCRIPTORS_H__ */