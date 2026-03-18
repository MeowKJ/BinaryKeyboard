#ifndef __MACRO_STORAGE_H__
#define __MACRO_STORAGE_H__

#include <stdint.h>

// ==================== MeowFS — Dynamic Macro File System ====================
//
// Flash area at tail of user flash: 1024 bytes (16 × 64B pages)
// Macros stored sequentially with variable size, no fixed slot limit.
//
// Layout:
//   [Macro 0: header(2) + actions(N×2)]
//   [Macro 1: header(2) + actions(N×2)]
//   ...
//   [0xFF = end / free space]
//
// Header format (2 bytes, flash-word aligned):
//   [0] marker:  0xAA = valid, 0x00 = deleted, 0xFF = free/end
//   [1] count:   number of actions
//
// Delete: write marker 0xAA → 0x00 (NOR flash: only clears bits)
// Compact: Studio reads valid macros, erases all, re-writes.
//

#define MEOWFS_BASE       0x3400
#define MEOWFS_SIZE       1024
#define MEOWFS_PAGE_SIZE  64
#define MEOWFS_PAGES      (MEOWFS_SIZE / MEOWFS_PAGE_SIZE)

#define MEOWFS_VALID_MARKER 0xAA
#define MEOWFS_HEADER_SIZE  2
#define MEOWFS_ACTION_SIZE  2
#define MEOWFS_MAX_ACTIONS  255

// ==================== Action types (aligned with CH592F) ====================
#define MACRO_ACT_KEY_DOWN   0x01
#define MACRO_ACT_KEY_UP     0x02
#define MACRO_ACT_MOD_DOWN   0x03
#define MACRO_ACT_MOD_UP     0x04
#define MACRO_ACT_DELAY      0x10
#define MACRO_ACT_CONSUMER   0x20
#define MACRO_ACT_MOUSE_DOWN 0x30
#define MACRO_ACT_MOUSE_UP   0x31
#define MACRO_ACT_WHEEL      0x32
#define MACRO_ACT_END        0xFF

// ==================== Trigger modes (aligned with CH592F) ====================
#define MACRO_TRIG_ONCE        0x00
#define MACRO_TRIG_HOLD_ABORT  0x01
#define MACRO_TRIG_HOLD_FINISH 0x02
#define MACRO_TRIG_TOGGLE      0x03

#ifdef __cplusplus
extern "C"
{
#endif

    // ---- MeowFS operations ----
    void     meowfs_format(void);
    uint8_t  meowfs_macro_count(void);
    uint16_t meowfs_find_macro(uint8_t index);
    uint16_t meowfs_used_bytes(void);
    uint8_t  meowfs_macro_valid(uint8_t index);

    // ---- Execution engine state (extern for inline queries) ----
    extern __xdata uint8_t macro_m_state;
    extern __xdata uint8_t macro_m_trigger;
    extern __xdata uint8_t macro_m_key_idx;
    extern __xdata uint8_t macro_m_cancel_req;

    // ---- Execution engine ----
    void macro_execute(uint8_t index, uint8_t trigger, uint8_t keyIndex);
    void macro_step(void);
    void macro_on_key_release(void);
    void macro_cancel(void);

// ---- Inline queries ----
#define macro_is_running() (macro_m_state != 0)
#define macro_running_key() (macro_m_key_idx)
#define macro_is_looping() (macro_m_state != 0 && macro_m_trigger != MACRO_TRIG_ONCE)

#ifdef __cplusplus
}
#endif

#endif // __MACRO_STORAGE_H__
