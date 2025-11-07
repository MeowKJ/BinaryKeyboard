#ifndef KBD_CONFIG_H
#define KBD_CONFIG_H

#include "board_config.h"

// ================== 基本配置 ================= //

/* 固件版本 */
#define KBD_FW_VERSION 0x01u

// =================== 键盘布局选择 ================= //

// #define KBD_LAYOUT_BASIC // 基础款
// #define KBD_LAYOUT_KNOB // 旋钮款
#define KBD_LAYOUT_5KEY // 五键款

// ================== 五键款 ================= //
#ifdef KBD_LAYOUT_5KEY

#define KBD_NUM_KEYS 5u


/* 每个键的端口/引脚 */
#define KBD_K1_PORT GPIO_PORT_B
#define KBD_K1_PIN 4u

#define KBD_K2_PORT GPIO_PORT_B
#define KBD_K2_PIN 7u

#define KBD_K3_PORT GPIO_PORT_B
#define KBD_K3_PIN 12u

#define KBD_K4_PORT GPIO_PORT_B
#define KBD_K4_PIN 13u

#define KBD_K5_PORT GPIO_PORT_A
#define KBD_K5_PIN 13u

static const kbd_key_pin_t g_kbd_key_pin_map[KBD_NUM_KEYS] = {
    {KBD_K1_PORT, KBD_K1_PIN},
    {KBD_K2_PORT, KBD_K2_PIN},
    {KBD_K3_PORT, KBD_K3_PIN},
    {KBD_K4_PORT, KBD_K4_PIN},
    {KBD_K5_PORT, KBD_K5_PIN}};

#endif /* KBD_LAYOUT_5KEY */

#endif /* KBD_CONFIG_H */
