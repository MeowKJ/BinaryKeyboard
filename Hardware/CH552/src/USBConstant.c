#include "USBConstant.h"

// Device descriptor
__code USB_Descriptor_Device_t DeviceDescriptor = {
  .Header = { .Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device },

  .USBSpecification = VERSION_BCD(1, 1, 0),
  .Class = 0x00,
  .SubClass = 0x00,
  .Protocol = 0x00,

  .Endpoint0Size = DEFAULT_ENDP0_SIZE,

  .VendorID = 0x1209,
  .ProductID = 0xc55D,
  .ReleaseNumber = VERSION_BCD(1, 0, 0),

  .ManufacturerStrIndex = 1,
  .ProductStrIndex = 2,
  .SerialNumStrIndex = 3,

  .NumberOfConfigurations = 1
};

/** Configuration descriptor structure. This descriptor, located in FLASH
 * memory, describes the usage of the device in one of its supported
 * configurations, including information about any device interfaces and
 * endpoints. The descriptor is read out by the USB host during the enumeration
 * process when selecting a configuration so that the host may correctly
 * communicate with the USB device.
 */
__code USB_Descriptor_Configuration_t ConfigurationDescriptor = {
  .Config = { .Header = { .Size = sizeof(USB_Descriptor_Configuration_Header_t),
                          .Type = DTYPE_Configuration },

              .TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
              .TotalInterfaces = 1,

              .ConfigurationNumber = 1,
              .ConfigurationStrIndex = NO_DESCRIPTOR,

              .ConfigAttributes = (USB_CONFIG_ATTR_RESERVED),

              .MaxPowerConsumption = USB_CONFIG_POWER_MA(200) },

  .HID_Interface = { .Header = { .Size = sizeof(USB_Descriptor_Interface_t),
                                 .Type = DTYPE_Interface },

                     .InterfaceNumber = 0,
                     .AlternateSetting = 0x00,

                     .TotalEndpoints = 2,

                     .Class = HID_CSCP_HIDClass,
                     .SubClass = HID_CSCP_BootSubclass,
                     .Protocol = HID_CSCP_KeyboardBootProtocol,

                     .InterfaceStrIndex = NO_DESCRIPTOR },

  .HID_KeyboardHID = { .Header = { .Size = sizeof(USB_HID_Descriptor_HID_t),
                                   .Type = HID_DTYPE_HID },

                       .HIDSpec = VERSION_BCD(1, 1, 0),
                       .CountryCode = 0x00,
                       .TotalReportDescriptors = 1,
                       .HIDReportType = HID_DTYPE_Report,
                       .HIDReportLength = sizeof(ReportDescriptor) },

  .HID_ReportINEndpoint = { .Header = { .Size =
                                          sizeof(USB_Descriptor_Endpoint_t),
                                        .Type = DTYPE_Endpoint },

                            .EndpointAddress = KEYBOARD_EPADDR,
                            .Attributes =
                              (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
                            .EndpointSize = KEYBOARD_MOUSE_EPSIZE,
                            .PollingIntervalMS = 10 },

  .HID_ReportOUTEndpoint = { .Header = { .Size =
                                           sizeof(USB_Descriptor_Endpoint_t),
                                         .Type = DTYPE_Endpoint },

                             .EndpointAddress = KEYBOARD_LED_EPADDR,
                             .Attributes =
                               (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
                             .EndpointSize = KEYBOARD_MOUSE_EPSIZE,
                             .PollingIntervalMS = 10 },
};

__code uint8_t ReportDescriptor[] = {
  // =================== 键盘报告 (ID1) ===================
  0x05, 0x01,        // USAGE_PAGE (Generic Desktop)
  0x09, 0x06,        // USAGE (Keyboard)
  0xA1, 0x01,        // COLLECTION (Application)
  0x85, 0x01,        //   REPORT_ID (1)
  0x05, 0x07,        //   USAGE_PAGE (Keyboard)
  0x19, 0xE0,        //   USAGE_MINIMUM (Left Control)
  0x29, 0xE7,        //   USAGE_MAXIMUM (Right GUI)
  0x15, 0x00,        //   LOGICAL_MINIMUM (0)
  0x25, 0x01,        //   LOGICAL_MAXIMUM (1)
  0x95, 0x08,        //   REPORT_COUNT (8)
  0x75, 0x01,        //   REPORT_SIZE (1)
  0x81, 0x02,        //   INPUT (Data,Var,Abs)
  0x95, 0x01,        //   REPORT_COUNT (1)
  0x75, 0x08,        //   REPORT_SIZE (8)
  0x81, 0x03,        //   INPUT (Cnst,Var,Abs)
  0x95, 0x06,        //   REPORT_COUNT (6)
  0x75, 0x08,        //   REPORT_SIZE (8)
  0x15, 0x00,        //   LOGICAL_MINIMUM (0)
  0x26, 0xFF, 0x00,  //   LOGICAL_MAXIMUM (255)
  0x19, 0x00,        //   USAGE_MINIMUM (Reserved)
  0x29, 0xFF,        //   USAGE_MAXIMUM (Keyboard FFh)
  0x81, 0x00,        //   INPUT (Data,Ary,Abs)
  0x05, 0x08,        //   USAGE_PAGE (LEDs)
  0x19, 0x01,        //   USAGE_MINIMUM (Num Lock)
  0x29, 0x05,        //   USAGE_MAXIMUM (Kana)
  0x15, 0x00,        //   LOGICAL_MINIMUM (0)
  0x25, 0x01,        //   LOGICAL_MAXIMUM (1)
  0x95, 0x05,        //   REPORT_COUNT (5)
  0x75, 0x01,        //   REPORT_SIZE (1)
  0x91, 0x02,        //   OUTPUT (Data,Var,Abs)
  0x95, 0x01,        //   REPORT_COUNT (1)
  0x75, 0x03,        //   REPORT_SIZE (3)
  0x91, 0x03,        //   OUTPUT (Cnst,Var,Abs)
  0xC0,              // END_COLLECTION (Keyboard)

  // ================ 多媒体控制 (ID2) ================
  0x05, 0x0C,        // USAGE_PAGE (Consumer)
  0x09, 0x01,        // USAGE (Consumer Control)
  0xA1, 0x01,        // COLLECTION (Application)
  0x85, 0x02,        //   REPORT_ID (2)
  0x15, 0x00,        //   LOGICAL_MINIMUM (0)
  0x26, 0xFF, 0x03,  //   LOGICAL_MAXIMUM (1023)
  0x19, 0x00,        //   USAGE_MINIMUM (Unassigned)
  0x2A, 0xFF, 0x03,  //   USAGE_MAXIMUM (Undefined)
  0x95, 0x04,        //   REPORT_COUNT (4)
  0x75, 0x10,        //   REPORT_SIZE (16)
  0x81, 0x00,        //   INPUT (Data,Ary,Abs)
  0xC0,              // END_COLLECTION (Consumer)

  // ================== 鼠标报告 (ID3) ==================
  0x05, 0x01,  // USAGE_PAGE (Generic Desktop)
  0x09, 0x02,  // USAGE (Mouse)
  0xA1, 0x01,  // COLLECTION (Application)
  0x09, 0x01,  //   USAGE (Pointer)
  0xA1, 0x00,  //   COLLECTION (Physical)
  0x85, 0x03,  //     REPORT_ID (3)
  0x05, 0x09,  //     USAGE_PAGE (Button)
  0x19, 0x01,  //     USAGE_MINIMUM (Button 1)
  0x29, 0x05,  //     USAGE_MAXIMUM (Button 5)  ⭐ 增加侧键
  0x15, 0x00,  //     LOGICAL_MINIMUM (0)
  0x25, 0x01,  //     LOGICAL_MAXIMUM (1)
  0x95, 0x05,  //     REPORT_COUNT (5)  ⭐ 5 个按钮
  0x75, 0x01,  //     REPORT_SIZE (1)
  0x81, 0x02,  //     INPUT (Data,Var,Abs)
  0x95, 0x01,  //     REPORT_COUNT (1)
  0x75, 0x03,  //     REPORT_SIZE (3)  ⭐ 填充 3 位（总共 8 位）
  0x81, 0x03,  //     INPUT (Cnst,Var,Abs)
  0x05, 0x01,  //     USAGE_PAGE (Generic Desktop)
  0x09, 0x30,  //     USAGE (X)
  0x09, 0x31,  //     USAGE (Y)
  0x09, 0x38,  //     USAGE (Wheel)
  0x15, 0x81,  //     LOGICAL_MINIMUM (-127)
  0x25, 0x7F,  //     LOGICAL_MAXIMUM (127)
  0x75, 0x08,  //     REPORT_SIZE (8)
  0x95, 0x03,  //     REPORT_COUNT (3)
  0x81, 0x06,  //     INPUT (Data,Var,Rel)
  0xC0,        //   END_COLLECTION (Physical)
  0xC0,        // END_COLLECTION (Mouse)

  // ============ 自定义输出报告 (ID4) ============
  0x06, 0x00, 0xFF,  // USAGE_PAGE (Vendor 0xFF00)
  0x09, 0x01,        // USAGE (Vendor Usage 1)
  0xA1, 0x01,        // COLLECTION (Application)
  0x85, 0x04,        //   REPORT_ID (4)
  0x09, 0x02,        //   USAGE (Vendor Data)
  0x15, 0x00,        //   LOGICAL_MINIMUM (0)
  0x26, 0xFF, 0x00,  //   LOGICAL_MAXIMUM (255)
  0x75, 0x08,        //   REPORT_SIZE (8)
  0x95, 0x1F,        //   REPORT_COUNT (31)
  0x91, 0x02,        //   OUTPUT (Data,Var,Abs)
  0xC0,              // END_COLLECTION

  //============ 自定义输入报告 (ID5) ============
  0x06, 0x00, 0xFF,  // USAGE_PAGE (Vendor 0xFF00)
  0x09, 0x03,        // USAGE (Vendor Usage 3)
  0xA1, 0x01,        // COLLECTION (Application)
  0x85, 0x05,        //   REPORT_ID (5)
  0x09, 0x04,        //   USAGE (Vendor Data)
  0x15, 0x00,        //   LOGICAL_MINIMUM (0)
  0x26, 0xFF, 0x00,  //   LOGICAL_MAXIMUM (255)
  0x75, 0x08,        //   REPORT_SIZE (8)
  0x95, 0x1F,        //   REPORT_COUNT (31)
  0x81, 0x02,        //   INPUT (Data,Var,Abs)
  0xC0               // END_COLLECTION

};
// 语言描述符
__code uint8_t LanguageDescriptor[] = { 0x04, 0x03, 0x09, 0x04 };  // 英语（美国）

// 序列号描述符（Binary Keyboard）
__code uint16_t SerialDescriptor[] = {
  (((15 + 1) * 2) | (DTYPE_String << 8)),
  'B', 'i', 'n', 'a', 'r', 'y', ' ', 'K', 'e', 'y', 'b', 'o', 'a', 'r', 'd'
};

// 产品描述符（Binary Keyboard）
__code uint16_t ProductDescriptor[] = {
  (((15 + 1) * 2) | (DTYPE_String << 8)),
  'B', 'i', 'n', 'a', 'r', 'y', ' ', 'K', 'e', 'y', 'b', 'o', 'a', 'r', 'd'
};

// 制造商描述符（KJ）
__code uint16_t ManufacturerDescriptor[] = {
  (((2 + 1) * 2) | (DTYPE_String << 8)),
  'K', 'J'
};
