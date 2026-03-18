/*
 * ch552_defs.h — CH552G 基础定义
 *
 * 替代 Arduino.h，仅保留本项目实际使用的常量、类型和宏。
 * 不再拉入 Print / Serial / analogRead / PWM 等无关声明。
 */

#ifndef CH552_DEFS_H
#define CH552_DEFS_H

// clang-format off
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "include/ch5xx.h"
#include "include/ch5xx_usb.h"
// clang-format on

/* ---- GPIO 电平 / 模式 ---- */
#define HIGH         1
#define LOW          0
#define INPUT        0x00
#define OUTPUT       0x01
#define INPUT_PULLUP 0x02

/* ---- 中断开关 ---- */
#define interrupts()    (EA = 1)
#define noInterrupts()  (EA = 0)

/* ---- 常用位操作 ---- */
#define lowByte(w)  ((uint8_t)((w) & 0xFF))
#define highByte(w) ((uint8_t)((w) >> 8))
#define bitRead(value, bit)  (((value) >> (bit)) & 0x01)
#define bitSet(value, bit)   ((value) |=  (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))

/* ---- 数学工具 ---- */
#ifdef abs
#undef abs
#endif
#define min(a, b)  ((a) < (b) ? (a) : (b))
#define max(a, b)  ((a) > (b) ? (a) : (b))
#define abs(x)     ((x) > 0 ? (x) : -(x))
#define constrain(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

/* ---- BSP 函数声明 ---- */

/* ch552_sys.c */
void sys_init(void);

/* ch552_timer.c */
uint32_t millis(void);
uint32_t micros(void);
void delay(__data uint32_t ms);
void delayMicroseconds(__data uint16_t us);

/* hal.c (GPIO) */
void pinMode(__data uint8_t pin, __xdata uint8_t mode);
void digitalWrite(__data uint8_t pin, __xdata uint8_t val);
uint8_t digitalRead(__data uint8_t pin);

/* ch552_eeprom.c */
void eeprom_write_byte(__data uint8_t addr, __xdata uint8_t val);
uint8_t eeprom_read_byte(__data uint8_t addr);

/* ch552_flash.c */
void flash_erase_page(uint16_t page_addr);
void flash_write_word(uint16_t addr, uint8_t lo, uint8_t hi);
uint8_t flash_read_byte(uint16_t addr);

#endif /* CH552_DEFS_H */
