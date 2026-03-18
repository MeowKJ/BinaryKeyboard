/*
 * entry.c — CH552G 固件入口 (BSP 版)
 *
 * 替代 SDK/cores/main.c，不再依赖 Arduino 框架。
 * 职责: 定义 Timer0 计数器、USB/Timer 中断向量、main() 入口。
 */

// clang-format off
#include <stdint.h>
#include "include/ch5xx.h"
// clang-format on

/* ---- Forward declarations ---- */
void USBInterrupt(void);                                          /* USBHandler.c */
void Timer0Interrupt(void) __interrupt(INT_NO_TMR0) __using(1);   /* ch552_timer.c */
void sys_init(void);                                              /* ch552_sys.c */
void setup(void);                                                 /* app/main.c */
void loop(void);                                                  /* app/main.c */

/* ---- Timer0 overflow counter — register bank 1 (IRAM 0x08..0x0C) ---- */
// clang-format off
__idata __at(0x08) volatile uint32_t timer0_overflow_count       = 0;
__idata __at(0x0C) volatile uint8_t  timer0_overflow_count_5th_byte = 0;
// clang-format on

/* ---- USB interrupt vector ---- */
void DeviceUSBInterrupt(void) __interrupt(INT_NO_USB) {
    USBInterrupt();
}

/* ---- main ---- */
void main(void) {
    sys_init();   /* 时钟 + Timer0 + 中断 */
    setup();      /* 用户初始化 */
    for (;;) {
        loop();   /* 用户主循环 */
    }
}

/* ---- SDCC external startup ---- */
unsigned char __sdcc_external_startup(void) __nonbanked {
    return 0;
}
