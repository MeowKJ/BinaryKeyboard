/*
 * ch552_sys.c — CH552G 系统初始化 (仅 24MHz 内部 RC)
 *
 * 从 ch55xduino wiring.c init() 精简而来:
 *  - 移除 CH549/CH559/外部晶振/多频率分支
 *  - 移除 PWM 初始化 (本项目不使用 PWM)
 *  - 移除 USB 初始化 (由 USBInit() 在 setup 中调用)
 *  - 仅保留: 时钟设置 + Timer0 (millis 基准) + 总中断
 */

// clang-format off
#include <stdint.h>
#include "include/ch5xx.h"
// clang-format on

void delayMicroseconds(__data uint16_t us);

/* Timer0: mode 2 auto-reload, Fsys/12 = 2MHz, 溢出周期 250 tick = 125us */
#define T0_CYCLE 250

void sys_init(void) {
    /* 设置系统时钟 24MHz (内部 RC) */
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    CLOCK_CFG = CLOCK_CFG & ~MASK_SYS_CK_SEL | 0x06;  /* 24MHz */
    SAFE_MOD = 0x00;

    delayMicroseconds(5000);  /* 等待内部 RC 稳定 */

    /* 初始化 Timer0 作为 millis/micros 时基 */
    TMOD = (TMOD & ~0x0F) | bT0_M1;   /* mode 2: 8-bit auto-reload */
    T2MOD &= ~bT0_CLK;                /* Fsys/12 */
    TH0 = 255 - T0_CYCLE + 1;         /* reload value */
    TF0 = 0;
    ET0 = 1;                           /* enable Timer0 interrupt */
    TR0 = 1;                           /* start Timer0 */

    EA = 1;                            /* 开总中断 */
}
