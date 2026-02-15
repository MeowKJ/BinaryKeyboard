/********************************** USB Descriptors Implementation *************
 * File Name          : usb_descriptors.c
 * Author             : Custom USB Library
 * Version            : V2.0
 * Date               : 2024/11/07
 * Description        : USB 描述符实现（基于 WCH 官方库）
 *******************************************************************************/

#include "usb_descriptors.h"

/* USB 设备描述符 */
const uint8_t USB_DeviceDescriptor[] = {
    0x12,                           // bLength
    0x01,                           // bDescriptorType (Device)
    0x10, 0x01,                     // bcdUSB (USB 1.1)
    0x00,                           // bDeviceClass
    0x00,                           // bDeviceSubClass
    0x00,                           // bDeviceProtocol
    DevEP0SIZE,                     // bMaxPacketSize0
    (USB_VID & 0xFF), (USB_VID >> 8),  // idVendor
    (USB_PID & 0xFF), (USB_PID >> 8),  // idProduct
    0x00, 0x03,                     // bcdDevice (3.00)
    0x01,                           // iManufacturer
    0x02,                           // iProduct
    0x00,                           // iSerialNumber
    0x01                            // bNumConfigurations
};

/* USB 配置描述符 */
const uint8_t USB_ConfigDescriptor[] = {
    /* Configuration Descriptor */
    0x09,                           // bLength
    0x02,                           // bDescriptorType (Configuration)
    (USB_CONFIG_DESC_SIZE & 0xFF), (USB_CONFIG_DESC_SIZE >> 8),
    INTF_COUNT,                     // bNumInterfaces (4个接口)
    0x01,                           // bConfigurationValue
    0x00,                           // iConfiguration
    0xA0,                           // bmAttributes (Bus Powered, Remote Wakeup)
    0x32,                           // bMaxPower (100mA)

    /* ================ Interface 0: Keyboard ================ */
    0x09,                           // bLength
    0x04,                           // bDescriptorType (Interface)
    INTF_KEYBOARD,                  // bInterfaceNumber
    0x00,                           // bAlternateSetting
    0x01,                           // bNumEndpoints
    0x03,                           // bInterfaceClass (HID)
    0x01,                           // bInterfaceSubClass (Boot Interface)
    0x01,                           // bInterfaceProtocol (Keyboard)
    0x00,                           // iInterface

    /* HID Descriptor */
    0x09,                           // bLength
    0x21,                           // bDescriptorType (HID)
    0x11, 0x01,                     // bcdHID (1.11)
    0x00,                           // bCountryCode
    0x01,                           // bNumDescriptors
    0x22,                           // bDescriptorType (Report)
    (HID_KEYBOARD_REPORT_DESC_SIZE & 0xFF), (HID_KEYBOARD_REPORT_DESC_SIZE >> 8),

    /* Endpoint Descriptor */
    0x07,                           // bLength
    0x05,                           // bDescriptorType (Endpoint)
    EP_KEYBOARD_IN,                 // bEndpointAddress
    0x03,                           // bmAttributes (Interrupt)
    HID_KEYBOARD_REPORT_SIZE, 0x00, // wMaxPacketSize
    0x0A,                           // bInterval (10ms)

    /* ================ Interface 1: Mouse ================ */
    0x09,                           // bLength
    0x04,                           // bDescriptorType (Interface)
    INTF_MOUSE,                     // bInterfaceNumber
    0x00,                           // bAlternateSetting
    0x01,                           // bNumEndpoints
    0x03,                           // bInterfaceClass (HID)
    0x01,                           // bInterfaceSubClass (Boot Interface)
    0x02,                           // bInterfaceProtocol (Mouse)
    0x00,                           // iInterface

    /* HID Descriptor */
    0x09,                           // bLength
    0x21,                           // bDescriptorType (HID)
    0x11, 0x01,                     // bcdHID (1.11)
    0x00,                           // bCountryCode
    0x01,                           // bNumDescriptors
    0x22,                           // bDescriptorType (Report)
    (HID_MOUSE_REPORT_DESC_SIZE & 0xFF), (HID_MOUSE_REPORT_DESC_SIZE >> 8),

    /* Endpoint Descriptor */
    0x07,                           // bLength
    0x05,                           // bDescriptorType (Endpoint)
    EP_MOUSE_IN,                    // bEndpointAddress
    0x03,                           // bmAttributes (Interrupt)
    HID_MOUSE_REPORT_SIZE, 0x00,    // wMaxPacketSize
    0x0A,                           // bInterval (10ms)

    /* ================ Interface 2: Consumer Control ================ */
    0x09,                           // bLength
    0x04,                           // bDescriptorType (Interface)
    INTF_CONSUMER,                  // bInterfaceNumber
    0x00,                           // bAlternateSetting
    0x01,                           // bNumEndpoints
    0x03,                           // bInterfaceClass (HID)
    0x00,                           // bInterfaceSubClass
    0x00,                           // bInterfaceProtocol
    0x00,                           // iInterface

    /* HID Descriptor */
    0x09,                           // bLength
    0x21,                           // bDescriptorType (HID)
    0x11, 0x01,                     // bcdHID (1.11)
    0x00,                           // bCountryCode
    0x01,                           // bNumDescriptors
    0x22,                           // bDescriptorType (Report)
    (HID_CONSUMER_REPORT_DESC_SIZE & 0xFF), (HID_CONSUMER_REPORT_DESC_SIZE >> 8),

    /* Endpoint Descriptor */
    0x07,                           // bLength
    0x05,                           // bDescriptorType (Endpoint)
    EP_CONSUMER_IN,                 // bEndpointAddress
    0x03,                           // bmAttributes (Interrupt)
    HID_CONSUMER_REPORT_SIZE, 0x00, // wMaxPacketSize
    0x0A,                           // bInterval (10ms)

    /* ================ Interface 3: Config (Vendor) ================ */
    0x09,                           // bLength
    0x04,                           // bDescriptorType (Interface)
    INTF_CONFIG,                    // bInterfaceNumber
    0x00,                           // bAlternateSetting
    0x02,                           // bNumEndpoints (IN + OUT)
    0x03,                           // bInterfaceClass (HID)
    0x00,                           // bInterfaceSubClass
    0x00,                           // bInterfaceProtocol
    0x00,                           // iInterface

    /* HID Descriptor */
    0x09,                           // bLength
    0x21,                           // bDescriptorType (HID)
    0x11, 0x01,                     // bcdHID (1.11)
    0x00,                           // bCountryCode
    0x01,                           // bNumDescriptors
    0x22,                           // bDescriptorType (Report)
    (HID_CONFIG_REPORT_DESC_SIZE & 0xFF), (HID_CONFIG_REPORT_DESC_SIZE >> 8),

    /* Endpoint Descriptor (IN) */
    0x07,                           // bLength
    0x05,                           // bDescriptorType (Endpoint)
    EP_CONFIG_IN,                   // bEndpointAddress
    0x03,                           // bmAttributes (Interrupt)
    HID_CONFIG_REPORT_SIZE, 0x00,   // wMaxPacketSize
    0x0A,                           // bInterval (10ms)

    /* Endpoint Descriptor (OUT) */
    0x07,                           // bLength
    0x05,                           // bDescriptorType (Endpoint)
    EP_CONFIG_OUT,                  // bEndpointAddress
    0x03,                           // bmAttributes (Interrupt)
    HID_CONFIG_REPORT_SIZE, 0x00,   // wMaxPacketSize
    0x0A                            // bInterval (10ms)
};

/* 字符串描述符 - 语言ID */
const uint8_t USB_StringLangID[] = {
    0x04, 0x03,
    0x09, 0x04      // 英文(美国)
};

/* 字符串描述符 - 厂商 */
const uint8_t USB_StringVendor[] = {
    0x12, 0x03,
    'M', 0, 'e', 0, 'o', 0, 'w', 0, 'K', 0, 'e', 0, 'y', 0, 's', 0
};

/* 字符串描述符 - 产品 */
const uint8_t USB_StringProduct[] = {
    0x1E, 0x03,
    'B', 0, 'i', 0, 'n', 0, 'a', 0, 'r', 0, 'y', 0,
    'K', 0, 'e', 0, 'y', 0, 'b', 0, 'o', 0, 'a', 0, 'r', 0, 'd', 0
};

/* USB 限定描述符 */
const uint8_t USB_QualifierDescriptor[] = {
    0x0A, 0x06, 0x00, 0x02,
    0xFF, 0x00, 0xFF, 0x40,
    0x01, 0x00
};

/* ==================== HID Report Descriptors ==================== */

/* Keyboard Report Descriptor (Boot Protocol) */
const uint8_t HID_KeyboardReportDescriptor[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    
    // Modifier keys
    0x05, 0x07,        //   Usage Page (Key Codes)
    0x19, 0xE0,        //   Usage Minimum (224)
    0x29, 0xE7,        //   Usage Maximum (231)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data, Variable, Absolute)
    
    // Reserved byte
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x01,        //   Input (Constant)
    
    // LED output report
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x01,        //   Report Size (1)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01,        //   Usage Minimum (1)
    0x29, 0x05,        //   Usage Maximum (5)
    0x91, 0x02,        //   Output (Data, Variable, Absolute)
    
    // LED padding
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x03,        //   Report Size (3)
    0x91, 0x01,        //   Output (Constant)
    
    // Key arrays (6 keys)
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x05, 0x07,        //   Usage Page (Key Codes)
    0x19, 0x00,        //   Usage Minimum (0)
    0x29, 0xFF,        //   Usage Maximum (255)
    0x81, 0x00,        //   Input (Data, Array)
    
    0xC0               // End Collection
};

/* Mouse Report Descriptor (Boot Protocol) */
const uint8_t HID_MouseReportDescriptor[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    0x09, 0x01,        //   Usage (Pointer)
    0xA1, 0x00,        //   Collection (Physical)
    
    // Buttons
    0x05, 0x09,        //     Usage Page (Buttons)
    0x19, 0x01,        //     Usage Minimum (1)
    0x29, 0x03,        //     Usage Maximum (3)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x75, 0x01,        //     Report Size (1)
    0x95, 0x03,        //     Report Count (3)
    0x81, 0x02,        //     Input (Data, Variable, Absolute)
    
    // Padding
    0x75, 0x05,        //     Report Size (5)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x01,        //     Input (Constant)
    
    // X, Y, Wheel
    0x05, 0x01,        //     Usage Page (Generic Desktop)
    0x09, 0x30,        //     Usage (X)
    0x09, 0x31,        //     Usage (Y)
    0x09, 0x38,        //     Usage (Wheel)
    0x15, 0x81,        //     Logical Minimum (-127)
    0x25, 0x7F,        //     Logical Maximum (127)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x03,        //     Report Count (3)
    0x81, 0x06,        //     Input (Data, Variable, Relative)
    
    0xC0,              //   End Collection
    0xC0               // End Collection
};

/* Consumer Control Report Descriptor */
const uint8_t HID_ConsumerReportDescriptor[] = {
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x03,  //   Logical Maximum (1023)
    0x19, 0x00,        //   Usage Minimum (0)
    0x2A, 0xFF, 0x03,  //   Usage Maximum (1023)
    0x75, 0x10,        //   Report Size (16)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x00,        //   Input (Data, Array)
    
    0xC0               // End Collection
};

/* Config Report Descriptor (Vendor Defined) - 无 Report ID，通过独立接口区分 */
const uint8_t HID_ConfigReportDescriptor[] = {
    0x06, 0x00, 0xFF,  // Usage Page (Vendor Defined 0xFF00)
    0x09, 0x01,        // Usage (Vendor Usage 1)
    0xA1, 0x01,        // Collection (Application)
    
    // Output Report (主机到设备 - 64 字节)
    0x09, 0x02,        //   Usage (Vendor Usage 2)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x40,        //   Report Count (64)
    0x91, 0x02,        //   Output (Data, Variable, Absolute)
    
    // Input Report (设备到主机 - 64 字节)
    0x09, 0x03,        //   Usage (Vendor Usage 3)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x40,        //   Report Count (64)
    0x81, 0x02,        //   Input (Data, Variable, Absolute)
    
    0xC0               // End Collection
};