#ifndef __CONFIG_H__
#define __CONFIG_H__

// 设备变体选择
// Arduino IDE: 取消注释对应的款式
// CMake 构建: 通过 -DKEYBOARD=BASIC|KNOB|5KEY 选择，无需修改此处
#if !defined(USE_BASIC) && !defined(USE_KNOB) && !defined(USE_5KEYS)
#define USE_BASIC
// #define USE_KNOB
// #define USE_5KEYS
#endif

#include "bk_version_config.h"

#define FW_VERSION_MAJOR BK_FIRMWARE_VERSION_MAJOR
#define FW_VERSION_MINOR BK_FIRMWARE_VERSION_MINOR
#define FW_VERSION_PATCH BK_FIRMWARE_VERSION_PATCH
#define PROTOCOL_VERSION_MAJOR BK_PROTOCOL_VERSION_MAJOR
#define PROTOCOL_VERSION_MINOR BK_PROTOCOL_VERSION_MINOR
#define STORAGE_VERSION_MAJOR BK_STORAGE_VERSION_MAJOR
#define STORAGE_VERSION_MINOR BK_STORAGE_VERSION_MINOR

#define CURRENT_FW_VERSION BK_LEGACY_COMPAT_VERSION

#define MAX_LAYERS 4

#define LED_PIN 11

//RGB配置
#define RGB_PIN 15
#define NUM_LEDS KEY_COUNT

#define FUNC_PIN 32


#ifdef USE_BASIC
#define EXPECT_DEVICE_TYPE 0x01

#define KEY_COUNT 4

#define KEY0_PIN 33
#define KEY1_PIN 34
#define KEY2_PIN 30
#define KEY3_PIN 31
#endif

#ifdef USE_KNOB
#define EXPECT_DEVICE_TYPE 0x02

#define KEY_COUNT 5
#define KEY0_PIN 34  //SW1
#define KEY1_PIN 14  //SWX1
#define KEY2_PIN 33  //SW2
#define KEY3_PIN 31  //SW3


#define ENCODER_LEFT 16   //SWA
#define ENCODER_RIGHT 17  //SWB
#define ENCODER_KEY 30    //SWK

#define ENCODER_LEFT_INDEX 5
#define ENCODER_RIGHT_INDEX 6

#endif

#ifdef USE_5KEYS
#define EXPECT_DEVICE_TYPE 0x03

#define KEY_COUNT 5
#define KEY0_PIN 33
#define KEY1_PIN 34
#define KEY2_PIN 14
#define KEY3_PIN 31
#define KEY4_PIN 30

#endif

#endif
