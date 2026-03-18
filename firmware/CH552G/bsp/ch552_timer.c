/*
 * ch552_timer.c — Timer0 中断 + millis / micros / delay / delayMicroseconds
 *
 * 从 ch55xduino wiring.c 精简而来，仅保留 CH552 @ 24MHz 分支。
 * 所有时间关键路径保持原始内联汇编不变。
 *
 * Timer0 配置: mode 2, auto-reload, Fsys/12 = 2MHz, 溢出周期 250 tick
 * 每次溢出 = 125us, timer0_overflow_count 以 125us 为单位递增
 *
 * millis() = timer0_overflow_count / 8   (125us * 8 = 1ms)
 * micros() = timer0_overflow_count * 4 + TL0_fraction
 *            (其中 TL0 范围 6..255, 减去 6 后 * ~0.5us)
 */

// clang-format off
#include <stdint.h>
#include "include/ch5xx.h"
// clang-format on

/* Timer0 溢出计数器 — 放在 register bank 1 (IRAM 0x08..0x0C)
 * 由 entry.c 中 __idata __at() 定义，这里 extern 引用 */
extern __idata volatile uint32_t timer0_overflow_count;
extern __idata volatile uint8_t  timer0_overflow_count_5th_byte;

/* ================================================================
 * Timer0 中断 (register bank 1)
 * 每 125us 触发一次，5 字节递增计数器
 * ================================================================ */
void Timer0Interrupt(void) __interrupt(INT_NO_TMR0) __using(1) {
    __asm__(";Increase timer0_overflow_count on R0~R4(5bytes)\n"
            "    inc r0                                      \n"
            "    cjne r0,#0,incTimer0_overflow_countOver$    \n"
            "    inc r1                                      \n"
            "    cjne r1,#0,incTimer0_overflow_countOver$    \n"
            "    inc r2                                      \n"
            "    cjne r2,#0,incTimer0_overflow_countOver$    \n"
            "    inc r3                                      \n"
            "    cjne r3,#0,incTimer0_overflow_countOver$    \n"
            "    inc r4                                      \n"
            "incTimer0_overflow_countOver$:                  \n");
}

/* ================================================================
 * micros()  —  CH552 @ 24MHz
 *
 * 算法: timer0_overflow_count * 4 (每 tick = 0.5us, ×4 得 us ×8, >>1 得 us)
 * 实际实现: (count << 2) | TL0_frac, 用内联汇编避免 32 位乘法
 * ================================================================ */
uint32_t micros(void) {
    /* 原子读取 count + TL0 */
    __asm__(";uint8_t interruptOn = EA; //to c            \n"
            ";clr and mov won't affect c                  \n"
            "    mov c,_EA                                \n"
            ";EA = 0;                                     \n"
            "    clr _EA                                  \n"
            ";Copy _timer0_overflow_count to local R0~R3,m\n"
            "    mov r0, (_timer0_overflow_count)         \n"
            "    mov r1, (_timer0_overflow_count)+1       \n"
            "    mov r2, (_timer0_overflow_count)+2       \n"
            "    mov r3, (_timer0_overflow_count)+3       \n"
            ";Copy TL0 to local R4, t                     \n"
            "    mov r4, _TL0                             \n"
            ";Copy TCON (TF0) to local R5                 \n"
            "    mov b, _TCON                             \n"
            ";if (interruptOn) EA = 1;                    \n"
            "    mov _EA,c                                \n"

            ";if ((TF0 in b) && (R4 != 255)){             \n"
            "    jnb b.5,incTimer0_overf_cntCopyOver$     \n"
            "    mov a,#1     \n"
            "    add a,r4     \n"
            "    jz incTimer0_overf_cntCopyOver$\n"

            ";m++                                         \n"
            "    inc r0                                   \n"
            "    cjne r0,#0,incTimer0_overf_cntCopyOver$  \n"
            "    inc r1                                   \n"
            "    cjne r1,#0,incTimer0_overf_cntCopyOver$  \n"
            "    inc r2                                   \n"
            "    cjne r2,#0,incTimer0_overf_cntCopyOver$  \n"
            "    inc r3                                   \n"
            "incTimer0_overf_cntCopyOver$:                \n");

    /* TL0 范围 6..255, 减去 (256-250)=6 归零 */
    __asm__("    clr c                                    \n"
            "    mov a, r4                                \n"
            "    subb a,#6                                \n"
            "    mov r4, a                                \n");

    /* 24MHz: return (count * 32) >> 8
     * 即 count >> 3, 但保留 TL0 低位精度 */
    __asm__(";return (timer0_overflow_count*32)>>8        \n"
            "    mov b, #32                               \n"
            "    mov a, r0                                \n"
            "    mul ab                                   \n"
            "    mov r0, b                                \n"
            ";lowest 8 bit not used (a), r0 free to use   \n"
            "    mov b, #32                               \n"
            "    mov a, r1                                \n"
            "    mul ab                                   \n"
            "    add a, r0                                \n"
            ";carry won't be set, if I calculated right   \n"
            "    mov dpl, a                               \n"
            "    mov r0, b                                \n"

            "    mov b, #32                               \n"
            "    mov a, r2                                \n"
            "    mul ab                                   \n"
            "    add a, r0                                \n"
            ";carry won't be set, if I calculated right   \n"
            "    mov dph, a                               \n"
            "    mov r0, b                                \n"

            "    mov b, #32                               \n"
            "    mov a, r3                                \n"
            "    mul ab                                   \n"
            "    add a, r0                                \n"
            ";carry won't be set, if I calculated right   \n"
            "    mov r1, a                                \n"
            "    mov r0, b                                \n"

            "    mov b, #32                               \n"
            "    mov a, r4                                \n"
            "    mul ab                                   \n"
            "    add a, r0                                \n"
            ";carry won't be set, if I calculated right   \n"

            ";calculation finished, a already in place    \n"
            "    mov b, r1                                \n");
}

/* ================================================================
 * millis()  —  CH552 @ 24MHz
 *
 * 算法: count / 8 = (count * 32) >> 8 (同 micros 的高位部分)
 * 但只取整毫秒，无需 TL0 低位
 * ================================================================ */
uint32_t millis(void) {
    __asm__(";uint8_t interruptOn = EA; //to c            \n"
            ";clr and mov won't affect c                  \n"
            "    mov c,_EA                                \n"
            ";EA = 0;                                     \n"
            "    clr _EA                                  \n"
            ";Copy _timer0_overflow_count to local R0~R4  \n"
            "    mov r0, (_timer0_overflow_count)         \n"
            "    mov r1, (_timer0_overflow_count)+1       \n"
            "    mov r2, (_timer0_overflow_count)+2       \n"
            "    mov r3, (_timer0_overflow_count)+3       \n"
            "    mov r4, (_timer0_overflow_count)+4       \n"
            ";if (interruptOn) EA = 1;                    \n"
            "    mov _EA,c                                \n"

            ";return timer0_overflow_count>>3             \n"
            "    mov a,r4                                 \n"
            "    rrc a                                    \n"
            "    mov r4,a                                 \n"
            "    mov a,r3                                 \n"
            "    rrc a                                    \n"
            "    mov r3,a                                 \n"
            "    mov a,r2                                 \n"
            "    rrc a                                    \n"
            "    mov r2,a                                 \n"
            "    mov a,r1                                 \n"
            "    rrc a                                    \n"
            "    mov r1,a                                 \n"
            "    mov a,r0                                 \n"
            "    rrc a                                    \n"
            "    mov r0,a                                 \n"

            "    mov a,r4                                 \n"
            "    rrc a                                    \n"
            "    mov r4,a                                 \n"
            "    mov a,r3                                 \n"
            "    rrc a                                    \n"
            "    mov r3,a                                 \n"
            "    mov a,r2                                 \n"
            "    rrc a                                    \n"
            "    mov r2,a                                 \n"
            "    mov a,r1                                 \n"
            "    rrc a                                    \n"
            "    mov r1,a                                 \n"
            "    mov a,r0                                 \n"
            "    rrc a                                    \n"
            "    mov r0,a                                 \n"

            "    mov a,r4                                 \n"
            "    rrc a                                    \n"
            "    mov a,r3                                 \n"
            "    rrc a                                    \n"
            "    mov b,a                                  \n"
            "    mov a,r2                                 \n"
            "    rrc a                                    \n"
            "    xch a,b                                  \n"
            "    mov a,r1                                 \n"
            "    rrc a                                    \n"
            "    mov dph,a                                \n"
            "    mov a,r0                                 \n"
            "    rrc a                                    \n"
            "    mov dpl,a                                \n");
}

/* ================================================================
 * delay()
 * ================================================================ */
void delay(__data uint32_t ms) {
    __data uint32_t start = micros();
    while (ms > 0) {
        while (ms > 0 && (micros() - start) >= 1000) {
            ms--;
            start += 1000;
        }
    }
}

/* ================================================================
 * delayMicroseconds()  —  CH552 @ 24MHz 专用汇编
 *
 * 24MHz / 12 = 2 cycles/us 在 8051 中，但 SDCC 下每条指令
 * 实际耗时需精确计数。此汇编已由 ch55xduino 作者校准。
 * ================================================================ */
void delayMicroseconds(__data uint16_t us) {
    us; /* avoid unreferenced argument warning */
    __asm__(
        ".even                                    \n"
        "    mov  r6, dpl                         \n"
        "    mov  r7, dph                         \n"
        "    clr  c                               \n"
        "    mov  a,#0x01                         \n"
        "    subb a, r6                           \n"
        "    clr  a                               \n"
        "    subb a, r7                           \n"
        "    jc skip_0us$                         \n"
        "    ret                                  \n"
        "    nop                                  \n"
        "skip_0us$:                               \n"
        "    clr  c                               \n"
        "    mov  a, #0x02                        \n"
        "    subb a, r6                           \n"
        "    mov  r6, a                           \n"
        "    mov  a, #0x00                        \n"
        "    subb a, r7                           \n"
        "    mov  r7, a                           \n"

        "    nop                                  \n"
        "    cjne r6,#0x00,loop24m_us$            \n"
        "    cjne r7,#0x00,loop24m_us$            \n"
        "    nop                                  \n"
        "    ret                                  \n"

        "loop24m_us$:                             \n"
        "    nop \n nop \n nop \n nop \n nop \n    "
        "    nop \n "
        "loop24m_us_2$:                          \n"
        "    nop \n nop \n nop \n nop \n nop \n    "
        "    nop \n nop \n nop \n nop \n nop \n    "
        "    nop \n                                "
        "    inc  r6                              \n"
        "    cjne r6, #0,loop24m_us$              \n"
        "    inc  r7                              \n"
        "    cjne r7, #0,loop24m_us_2$            \n"
        "    nop                                  \n");
}
