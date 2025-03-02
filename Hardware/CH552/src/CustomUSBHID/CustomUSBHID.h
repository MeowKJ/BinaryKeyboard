#ifndef __USB_HID_KBD_H__
#define __USB_HID_KBD_H__

// clang-format off
#include <stdint.h>
#include "include/ch5xx.h"
#include "include/ch5xx_usb.h"
// clang-format on

#define KEY_LEFT_CTRL 0x80
#define KEY_LEFT_SHIFT 0x81
#define KEY_LEFT_ALT 0x82
#define KEY_LEFT_GUI 0x83
#define KEY_RIGHT_CTRL 0x84
#define KEY_RIGHT_SHIFT 0x85
#define KEY_RIGHT_ALT 0x86
#define KEY_RIGHT_GUI 0x87

#define KEY_UP_ARROW 0xDA
#define KEY_DOWN_ARROW 0xD9
#define KEY_LEFT_ARROW 0xD8
#define KEY_RIGHT_ARROW 0xD7
#define KEY_BACKSPACE 0xB2
#define KEY_TAB 0xB3
#define KEY_RETURN 0xB0
#define KEY_ESC 0xB1
#define KEY_INSERT 0xD1
#define KEY_DELETE 0xD4
#define KEY_PAGE_UP 0xD3
#define KEY_PAGE_DOWN 0xD6
#define KEY_HOME 0xD2
#define KEY_END 0xD5
#define KEY_CAPS_LOCK 0xC1
#define KEY_F1 0xC2
#define KEY_F2 0xC3
#define KEY_F3 0xC4
#define KEY_F4 0xC5
#define KEY_F5 0xC6
#define KEY_F6 0xC7
#define KEY_F7 0xC8
#define KEY_F8 0xC9
#define KEY_F9 0xCA
#define KEY_F10 0xCB
#define KEY_F11 0xCC
#define KEY_F12 0xCD
#define KEY_F13 0xF0
#define KEY_F14 0xF1
#define KEY_F15 0xF2
#define KEY_F16 0xF3
#define KEY_F17 0xF4
#define KEY_F18 0xF5
#define KEY_F19 0xF6
#define KEY_F20 0xF7
#define KEY_F21 0xF8
#define KEY_F22 0xF9
#define KEY_F23 0xFA
#define KEY_F24 0xFB

enum ConsumerKeycode
{
  // 系统控制
  CONSUMER_POWER = 0x30, // 电源键
  CONSUMER_SLEEP = 0x32, // 睡眠键

  // 媒体控制
  MEDIA_PLAY_PAUSE = 0xCD, // 播放/暂停
  MEDIA_STOP = 0xB7,       // 停止
  MEDIA_NEXT = 0xB5,       // 下一曲
  MEDIA_PREVIOUS = 0xB6,   // 上一曲

  // 音量控制
  MEDIA_VOLUME_MUTE = 0xE2, // 静音
  MEDIA_VOLUME_UP = 0xE9,   // 音量加
  MEDIA_VOLUME_DOWN = 0xEA, // 音量减

  // 浏览器快捷键
  CONSUMER_BROWSER_HOME = 0x223,    // 浏览器主页
  CONSUMER_BROWSER_BACK = 0x224,    // 后退
  CONSUMER_BROWSER_FORWARD = 0x225, // 前进
  CONSUMER_BROWSER_REFRESH = 0x227, // 刷新

  // 应用启动
  CONSUMER_EMAIL_READER = 0x18A, // 启动邮件程序
  CONSUMER_CALCULATOR = 0x192,   // 启动计算器
  CONSUMER_EXPLORER = 0x194      // 启动文件资源管理器
};

enum MOUSE_BUTTON
{
  MOUSE_LEFT = 1,
  MOUSE_RIGHT = 2,
  MOUSE_MIDDLE = 4,
};

#ifdef __cplusplus
extern "C"
{
#endif

  void USBInit(void);

  uint8_t Keyboard_press(__data uint8_t k);
  uint8_t Keyboard_release(__data uint8_t k);
  void Keyboard_releaseAll(void);

  uint8_t Keyboard_write(__data uint8_t c);
  void Keyboard_print(const char *str);

  uint8_t Consumer_press(__data uint16_t k);
  uint8_t Consumer_release(__data uint16_t k);
  void Consumer_releaseAll(void);

  uint8_t Consumer_write(__data uint16_t c);

  uint8_t Keyboard_getLEDStatus();

  uint8_t Mouse_press(__data uint8_t k);
  uint8_t Mouse_release(__data uint8_t k);
  uint8_t Mouse_click(__data uint8_t k);
  uint8_t Mouse_move(__data int8_t x, __xdata int8_t y);
  uint8_t Mouse_scroll(__data int8_t tilt);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
