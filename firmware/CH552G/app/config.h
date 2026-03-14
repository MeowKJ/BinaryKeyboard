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

// CH552 的特性级别用于旧 HID 读写兼容，不参与发布版本号
#define CH552_FEATURE_LEVEL 0x0D
// EEPROM 头部只保留本地 schema 常量，避免旧布局被误读
#define EEPROM_SCHEMA_ID 0x01

#define LED_PIN 11

// RGB 配置
#define RGB_PIN 15

#define FUNC_PIN 32

#ifdef USE_BASIC
#define EXPECT_DEVICE_TYPE 0x01

#define KEY_COUNT 4
#define NUM_LEDS 4
#define MAX_LAYERS 4
#define KEY_CONFIG_SLOTS 4
#define LAYER_SWITCH_KEY_COUNT 4
#define RGB_LOGICAL_TO_PHYSICAL_MAP {1, 0, 3, 2}

#define KEY0_PIN 33
#define KEY1_PIN 34
#define KEY2_PIN 30
#define KEY3_PIN 31
#endif

#ifdef USE_KNOB
#define EXPECT_DEVICE_TYPE 0x02

#define KEY_COUNT 5
#define NUM_LEDS 4
#define MAX_LAYERS 4
#define KEY_CONFIG_SLOTS 7
#define LAYER_SWITCH_KEY_COUNT 4
#define RGB_LOGICAL_TO_PHYSICAL_MAP {1, 0, 2, 3}
#define KEY0_PIN 34 // SW1
#define KEY1_PIN 14 // SWX1
#define KEY2_PIN 33 // SW2
#define KEY3_PIN 31 // SW3

#define ENCODER_LEFT 16  // SWA
#define ENCODER_RIGHT 17 // SWB
#define ENCODER_KEY 30   // SWK

#define ENCODER_LEFT_INDEX 5
#define ENCODER_RIGHT_INDEX 6

#endif

#ifdef USE_5KEYS
#define EXPECT_DEVICE_TYPE 0x03

#define KEY_COUNT 5
#define NUM_LEDS 5
#define MAX_LAYERS 5
#define KEY_CONFIG_SLOTS 5
#define LAYER_SWITCH_KEY_COUNT 5
#define RGB_LOGICAL_TO_PHYSICAL_MAP {2, 1, 0, 3, 4}
#define KEY0_PIN 33
#define KEY1_PIN 34
#define KEY2_PIN 14
#define KEY3_PIN 31
#define KEY4_PIN 30

#endif

#endif
