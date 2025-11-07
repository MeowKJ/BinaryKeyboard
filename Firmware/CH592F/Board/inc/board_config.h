#ifndef __BOARD_CONFIG_H__
#define __BOARD_CONFIG_H__

#include "CH59x_common.h"

typedef enum
{
    GPIO_PORT_A = 0u,
    GPIO_PORT_B = 1u,
} gpio_port_t;

typedef struct
{
    uint8_t port; /* GPIO_PORT_A / GPIO_PORT_B */
    uint8_t pin;  /* 引脚号 */
} kbd_key_pin_t;

#endif /* __SYS_CONFIG_H__ */