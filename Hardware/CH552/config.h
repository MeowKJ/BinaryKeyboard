#ifndef __CONFIG_H__
#define __CONFIG_H__

// 基础款
//#define USE_BASIC

// 旋钮款
#define USE_KNOB

//五键款
//#define USE_5KEYS


#define LED_PIN 11
#define FUNC_PIN 32


#ifdef USE_BASIC
#define KEY_COUNT 4

#define KEY0_PIN 33
#define KEY1_PIN 34
#define KEY2_PIN 31
#define KEY3_PIN 30
#endif

#ifdef USE_KNOB

#define KEY_COUNT 5
#define LED_PIN 11
#define LED_PIN 11
#define KEY0_PIN 34 //SW1
#define KEY1_PIN 14 //SWX1
#define KEY2_PIN 33 //SW2
#define KEY3_PIN 31 //SW3


#define ENCODER_LEFT 16 //SWA
#define ENCODER_RIGHT 17 //SWB
#define ENCODER_KEY 30 //SWK

#define ENCODER_LEFT_INDEX 5
#define ENCODER_RIGHT_INDEX 6

#endif

#ifdef USE_5KEYS

#define KEY_COUNT 5
#define LED_PIN 11
#define KEY0_PIN 34
#define KEY1_PIN 14
#define KEY2_PIN 33
#define KEY3_PIN 31

#endif

#endif