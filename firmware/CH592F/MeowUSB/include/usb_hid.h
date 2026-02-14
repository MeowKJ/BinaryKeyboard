/********************************** USB HID Functions Header ******************
 * File Name          : usb_hid.h
 * Author             : Custom USB Library
 * Version            : V2.0
 * Date               : 2024/11/07
 * Description        : USB HID 功能接口定义（基于 WCH 官方库）
 *******************************************************************************/

#ifndef __USB_HID_H__
#define __USB_HID_H__

#include "CH59x_common.h"
#include "usb_descriptors.h"

/* ==================== Keyboard Definitions ==================== */
/* Keyboard Modifier Keys */
#define KEY_MOD_LCTRL       0x01
#define KEY_MOD_LSHIFT      0x02
#define KEY_MOD_LALT        0x04
#define KEY_MOD_LMETA       0x08
#define KEY_MOD_RCTRL       0x10
#define KEY_MOD_RSHIFT      0x20
#define KEY_MOD_RALT        0x40
#define KEY_MOD_RMETA       0x80

/* Keyboard Key Codes (常用键) */
#define KEY_A               0x04
#define KEY_B               0x05
#define KEY_C               0x06
#define KEY_D               0x07
#define KEY_E               0x08
#define KEY_F               0x09
#define KEY_G               0x0A
#define KEY_H               0x0B
#define KEY_I               0x0C
#define KEY_J               0x0D
#define KEY_K               0x0E
#define KEY_L               0x0F
#define KEY_M               0x10
#define KEY_N               0x11
#define KEY_O               0x12
#define KEY_P               0x13
#define KEY_Q               0x14
#define KEY_R               0x15
#define KEY_S               0x16
#define KEY_T               0x17
#define KEY_U               0x18
#define KEY_V               0x19
#define KEY_W               0x1A
#define KEY_X               0x1B
#define KEY_Y               0x1C
#define KEY_Z               0x1D

#define KEY_1               0x1E
#define KEY_2               0x1F
#define KEY_3               0x20
#define KEY_4               0x21
#define KEY_5               0x22
#define KEY_6               0x23
#define KEY_7               0x24
#define KEY_8               0x25
#define KEY_9               0x26
#define KEY_0               0x27

#define KEY_ENTER           0x28
#define KEY_ESC             0x29
#define KEY_BACKSPACE       0x2A
#define KEY_TAB             0x2B
#define KEY_SPACE           0x2C
#define KEY_MINUS           0x2D
#define KEY_EQUAL           0x2E
#define KEY_LEFTBRACE       0x2F
#define KEY_RIGHTBRACE      0x30
#define KEY_BACKSLASH       0x31
#define KEY_SEMICOLON       0x33
#define KEY_APOSTROPHE      0x34
#define KEY_GRAVE           0x35
#define KEY_COMMA           0x36
#define KEY_DOT             0x37
#define KEY_SLASH           0x38
#define KEY_CAPS_LOCK       0x39

#define KEY_F1              0x3A
#define KEY_F2              0x3B
#define KEY_F3              0x3C
#define KEY_F4              0x3D
#define KEY_F5              0x3E
#define KEY_F6              0x3F
#define KEY_F7              0x40
#define KEY_F8              0x41
#define KEY_F9              0x42
#define KEY_F10             0x43
#define KEY_F11             0x44
#define KEY_F12             0x45

#define KEY_DELETE          0x4C
#define KEY_INSERT          0x49
#define KEY_HOME            0x4A
#define KEY_END             0x4D
#define KEY_PAGEUP          0x4B
#define KEY_PAGEDOWN        0x4E

#define KEY_RIGHT           0x4F
#define KEY_LEFT            0x50
#define KEY_DOWN            0x51
#define KEY_UP              0x52

/* Keyboard LED Status */
#define LED_NUM_LOCK        0x01
#define LED_CAPS_LOCK       0x02
#define LED_SCROLL_LOCK     0x04

/* ==================== Mouse Definitions ==================== */
/* Mouse Buttons */
#define MOUSE_BTN_LEFT      0x01
#define MOUSE_BTN_RIGHT     0x02
#define MOUSE_BTN_MIDDLE    0x04

/* ==================== Consumer Control Definitions ==================== */
/* Consumer Control Keys (多媒体按键) */
// #define CONSUMER_PLAY_PAUSE     0xCD
// #define CONSUMER_STOP           0xB7
// #define CONSUMER_NEXT_TRACK     0xB5
// #define CONSUMER_PREV_TRACK     0xB6
// #define CONSUMER_VOLUME_UP      0xE9
// #define CONSUMER_VOLUME_DOWN    0xEA
// #define CONSUMER_MUTE           0xE2
// #define CONSUMER_BRIGHTNESS_UP  0x6F
// #define CONSUMER_BRIGHTNESS_DN  0x70

/* ==================== Config Command Definitions ==================== */
/* 配置命令定义 */
#define CONFIG_CMD_GET_VERSION      0x01
#define CONFIG_CMD_GET_STATUS       0x02
#define CONFIG_CMD_SET_CONFIG       0x10
#define CONFIG_CMD_GET_CONFIG       0x11
#define CONFIG_CMD_RESET            0xFF

/* 配置响应码 */
#define CONFIG_RESP_OK              0x00
#define CONFIG_RESP_ERROR           0x01
#define CONFIG_RESP_INVALID_CMD     0x02

/* ==================== Structures ==================== */

/* Keyboard Report Structure */
typedef struct __attribute__((packed)) {
    uint8_t modifier;       // 修饰键
    uint8_t reserved;       // 保留字节
    uint8_t keycode[6];     // 最多6个按键
} USB_KeyboardReport_t;

/* Mouse Report Structure */
typedef struct __attribute__((packed)) {
    uint8_t buttons;        // 按键状态
    int8_t  x;              // X轴移动
    int8_t  y;              // Y轴移动
    int8_t  wheel;          // 滚轮
} USB_MouseReport_t;

/* Consumer Report Structure */
typedef struct __attribute__((packed)) {
    uint16_t key;           // 多媒体键值
} USB_ConsumerReport_t;

/* Config Report Structure */
typedef struct __attribute__((packed)) {
    uint8_t cmd;            // 命令字节
    uint8_t data[63];       // 数据
} USB_ConfigReport_t;

/* ==================== Global Variables ==================== */
extern USB_KeyboardReport_t g_KeyboardReport;
extern USB_MouseReport_t    g_MouseReport;
extern USB_ConsumerReport_t g_ConsumerReport;
extern USB_ConfigReport_t   g_ConfigReport;

extern uint8_t g_KeyboardLEDs;  // 键盘LED状态

/* ==================== Function Prototypes ==================== */

/* === Keyboard Functions === */
void USB_Keyboard_Init(void);
void USB_Keyboard_Press(uint8_t modifier, uint8_t *keys, uint8_t num_keys);
void USB_Keyboard_Release(void);
void USB_Keyboard_Type(uint8_t modifier, uint8_t key);
void USB_Keyboard_SendReport(void);
void USB_Keyboard_SetLEDs(uint8_t leds);

/* === Mouse Functions === */
void USB_Mouse_Init(void);
void USB_Mouse_Move(int8_t x, int8_t y, int8_t wheel);
void USB_Mouse_Click(uint8_t buttons);
void USB_Mouse_Press(uint8_t buttons);
void USB_Mouse_Release(void);
void USB_Mouse_SendReport(void);

/* === Consumer Control Functions === */
void USB_Consumer_Init(void);
void USB_Consumer_Press(uint16_t key);
void USB_Consumer_Release(void);
void USB_Consumer_SendReport(void);

/* === Config Functions === */
void USB_Config_Init(void);
void USB_Config_SendResponse(uint8_t cmd, uint8_t *data, uint8_t len);
void USB_Config_ProcessCommand(USB_ConfigReport_t *report);
void USB_Config_PollProcess(void);

/* === USB Device Callbacks === */
void USB_DevEP1_IN_Callback(void);   // Keyboard
void USB_DevEP2_IN_Callback(void);   // Mouse
void USB_DevEP3_IN_Callback(void);   // Consumer
void USB_DevEP4_IN_Callback(void);   // Config IN

#endif /* __USB_HID_H__ */
