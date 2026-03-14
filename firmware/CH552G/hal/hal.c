/*
 * hal.c — Minimal GPIO for CH552G
 *
 * Drop-in replacement for SDK/cores/wiring_digital.c.
 * Only handles P1 and P3 (the only ports CH552G exposes).
 * Removes PWM turn-off, multi-chip branches, and lookup tables
 * to cut DSEG (internal RAM) usage.
 *
 * Pin encoding is the same as ch55xduino:
 *   port * 10 + bit   (e.g. P3.2 = 32, P1.5 = 15)
 */

#include <stdint.h>
#include "include/ch5xx.h"

#define _INPUT       0x00
#define _OUTPUT      0x01
#define _INPUT_PU    0x02

void pinMode(__data uint8_t pin, __xdata uint8_t mode) {
  __data uint8_t bit  = (uint8_t)(1u << (pin % 10));
  __data uint8_t port = pin / 10;

  if (mode == _INPUT_PU) {
    if (port == 1)      { P1_MOD_OC |=  bit; P1_DIR_PU |=  bit; }
    else if (port == 3) { P3_MOD_OC |=  bit; P3_DIR_PU |=  bit; }
  } else if (mode == _OUTPUT) {
    if (port == 1)      { P1_MOD_OC &= ~bit; P1_DIR_PU |=  bit; }
    else if (port == 3) { P3_MOD_OC &= ~bit; P3_DIR_PU |=  bit; }
  } else {  /* INPUT (floating) */
    if (port == 1)      { P1_MOD_OC &= ~bit; P1_DIR_PU &= ~bit; }
    else if (port == 3) { P3_MOD_OC &= ~bit; P3_DIR_PU &= ~bit; }
  }
}

uint8_t digitalRead(__data uint8_t pin) {
  __data uint8_t bit  = (uint8_t)(1u << (pin % 10));
  __data uint8_t port = pin / 10;

  if (port == 1) return (P1 & bit) ? 1 : 0;
  if (port == 3) return (P3 & bit) ? 1 : 0;
  return 0;
}

void digitalWrite(__data uint8_t pin, __xdata uint8_t val) {
  __data uint8_t bit  = (uint8_t)(1u << (pin % 10));
  __data uint8_t port = pin / 10;

  __data uint8_t ea = EA;
  EA = 0;

  if (port == 1) {
    if (val) P1 |= bit; else P1 &= ~bit;
  } else if (port == 3) {
    if (val) P3 |= bit; else P3 &= ~bit;
  }

  if (ea) EA = 1;
}
