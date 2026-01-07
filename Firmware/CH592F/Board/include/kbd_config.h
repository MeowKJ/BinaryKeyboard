#ifndef KBD_CONFIG_H
#define KBD_CONFIG_H

#include "CH59x_common.h"

#define KBD_LAYOUT_5KEY

typedef enum
{
    GPIO_PORT_A = 0,
    GPIO_PORT_B = 1,
} gpio_port_t;

typedef struct
{
    gpio_port_t port;
    uint32_t pin; // 位掩码：GPIO_Pin_x
} kbd_key_pin_t;

/* ======================= 特殊IO ======================= */

// WS2812: TMR1
#define WS2812_PORT GPIO_PORT_A
#define WS2812_PIN GPIO_Pin_10

// WS2812 ENABLE: PA9
#define WS2812_EN_PORT GPIO_PORT_A
#define WS2812_EN_PIN GPIO_Pin_9
/* ======================= 功能键======================= */
/* 只需要下降沿触发（按下事件），不需要释放事件 */

#define KBD_FN_NUM_KEYS 2

// BOOT: PB22
#define KBD_FN_BOOT_PORT GPIO_PORT_B
#define KBD_FN_BOOT_PIN GPIO_Pin_22

// FN1: PB14
#define KBD_FN1_PORT GPIO_PORT_B
#define KBD_FN1_PIN GPIO_Pin_14

// FN2: PB15
#define KBD_FN2_PORT GPIO_PORT_B
#define KBD_FN2_PIN GPIO_Pin_15

/* ======================= 普通键（键盘键位）======================= */
#ifdef KBD_LAYOUT_5KEY
#define KBD_NUM_KEYS 5u

// 五键映射
#define KBD_K1_PORT GPIO_PORT_B
#define KBD_K1_PIN GPIO_Pin_4

#define KBD_K2_PORT GPIO_PORT_B
#define KBD_K2_PIN GPIO_Pin_7

#define KBD_K3_PORT GPIO_PORT_B
#define KBD_K3_PIN GPIO_Pin_12

#define KBD_K4_PORT GPIO_PORT_B
#define KBD_K4_PIN GPIO_Pin_13

#define KBD_K5_PORT GPIO_PORT_A
#define KBD_K5_PIN GPIO_Pin_13

#endif /* KBD_LAYOUT_5KEY */

#endif /* KBD_CONFIG_H */
