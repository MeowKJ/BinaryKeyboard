#ifndef __MACRO_STORAGE_H__
#define __MACRO_STORAGE_H__

#include <stdint.h>

// ==================== 存储布局 (代码闪存末尾 192B) ====================
// Slot 0: 0x3740 - 0x379F (96 字节)
// Slot 1: 0x37A0 - 0x37FF (96 字节)
#define MACRO_SLOT_COUNT 2
#define MACRO_SLOT_SIZE 96
#define MACRO_SLOT0_ADDR 0x3740
#define MACRO_SLOT1_ADDR 0x37A0

// ==================== 宏头部 (2 字节, CH552G 精简版) ====================
// [0]     valid        (0xAA = 有效)
// [1]     action_count (动作数量, uint8_t)
// [2..]   actions      (每条 2 字节 [type, param])
#define MACRO_VALID_MARKER 0xAA
#define MACRO_HEADER_SIZE 2
#define MACRO_ACTION_SIZE 2
#define MACRO_MAX_ACTIONS ((MACRO_SLOT_SIZE - MACRO_HEADER_SIZE) / MACRO_ACTION_SIZE)

// ==================== 动作类型 (与 CH592F 对齐) ====================
#define MACRO_ACT_KEY_DOWN 0x01
#define MACRO_ACT_KEY_UP 0x02
#define MACRO_ACT_MOD_DOWN 0x03
#define MACRO_ACT_MOD_UP 0x04
#define MACRO_ACT_DELAY 0x10
#define MACRO_ACT_CONSUMER 0x20
#define MACRO_ACT_MOUSE_DOWN 0x30
#define MACRO_ACT_MOUSE_UP 0x31
#define MACRO_ACT_WHEEL 0x32
#define MACRO_ACT_END 0xFF

// ==================== 触发模式 (与 CH592F 对齐) ====================
#define MACRO_TRIG_ONCE 0x00
#define MACRO_TRIG_HOLD_ABORT 0x01
#define MACRO_TRIG_HOLD_FINISH 0x02
#define MACRO_TRIG_TOGGLE 0x03

#ifdef __cplusplus
extern "C"
{
#endif

    // ---- 存储操作 ----
    uint16_t macro_slot_addr(uint8_t slot);
    uint8_t macro_read_byte(uint16_t addr);
    void macro_erase_slot(uint8_t slot);
    void macro_write_word(uint16_t addr, uint8_t lo, uint8_t hi);
    uint8_t macro_is_valid(uint8_t slot);
    uint16_t macro_action_count(uint8_t slot);

    // ---- 执行引擎状态 (extern, 用于内联查询) ----
    extern __xdata uint8_t macro_m_state;
    extern __xdata uint8_t macro_m_trigger;
    extern __xdata uint8_t macro_m_key_idx;
    extern __xdata uint8_t macro_m_cancel_req;

    // ---- 执行引擎 ----
    void macro_execute(uint8_t slot, uint8_t trigger, uint8_t keyIndex);
    void macro_step(void);
    void macro_on_key_release(void);
    void macro_cancel(void);

// ---- 内联查询宏 ----
#define macro_is_running() (macro_m_state != 0)
#define macro_running_key() (macro_m_key_idx)
#define macro_is_looping() (macro_m_state != 0 && macro_m_trigger != MACRO_TRIG_ONCE)

#ifdef __cplusplus
}
#endif

#endif // __MACRO_STORAGE_H__
