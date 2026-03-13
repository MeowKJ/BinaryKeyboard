/*
  main.c - Entry point for CH552G firmware (CMake build)
  Based on ch55xduino by Deqing Sun.

  Stripped-down version: only USB interrupt, Timer0 (millis), and main loop.
  UART / GPIO / Touch-key ISRs removed (unused by BinaryKeyboard firmware).
*/

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
void USBInterrupt(void);  // defined in USBHandler.c
void Timer0Interrupt(void) __interrupt(INT_NO_TMR0) __using(1);  // in wiring.c

// ---------------------------------------------------------------------------
// Timer0 overflow counter — register bank 1 (0x08..0x0F)
// Used by millis() / micros() in wiring.c
// ---------------------------------------------------------------------------
// clang-format off
__idata __at(0x08) volatile uint32_t timer0_overflow_count = 0;
__idata __at(0x0C) volatile uint8_t  timer0_overflow_count_5th_byte = 0;
// clang-format on

// ---------------------------------------------------------------------------
// USB interrupt vector → dispatches to firmware's USBInterrupt()
// ---------------------------------------------------------------------------
void DeviceUSBInterrupt(void) __interrupt(INT_NO_USB) {
    USBInterrupt();
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
void main(void) {
    init();     // system clock, Timer0, PWM  (from wiring.c)
    setup();    // user initialization        (from User/Main.c)

    for (;;) {
        loop(); // user main loop             (from User/Main.c)
    }
}

// ---------------------------------------------------------------------------
// SDCC external startup — return 0 to perform normal data initialization
// ---------------------------------------------------------------------------
unsigned char __sdcc_external_startup(void) __nonbanked {
    return 0;
}
