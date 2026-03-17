#include <stdint.h>
#include "MacroStorage.h"
#include "CustomUSBHID.h"

// clang-format off
#include "include/ch5xx.h"
// clang-format on

// ==================== 存储操作 ====================

uint16_t macro_slot_addr(uint8_t slot)
{
    return (slot == 0) ? MACRO_SLOT0_ADDR : MACRO_SLOT1_ADDR;
}

uint8_t macro_read_byte(uint16_t addr)
{
    return *(__code uint8_t *)addr;
}

static void erase_flash_page(uint16_t pageAddr)
{
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    GLOBAL_CFG |= bCODE_WE;
    ROM_ADDR = pageAddr;
    if (ROM_STATUS & bROM_ADDR_OK)
    {
        ROM_CTRL = 0xA6;
    }
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    GLOBAL_CFG &= ~bCODE_WE;
    SAFE_MOD = 0;
}

void macro_erase_slot(uint8_t slot)
{
    // 192B macro region is 3×64B pages. Two 96B slots share the middle page:
    //   Page 0x3740: slot0[0..63]
    //   Page 0x3780: slot0[64..95] + slot1[0..31]   (shared erase page)
    //   Page 0x37C0: slot1[32..95]
    //
    // Because erase granularity is 64B, erasing one slot must preserve the
    // other slot's 32B half inside the shared page.

    static __xdata uint8_t preserve[32];

    if (slot == 0)
    {
        // Preserve slot1 first 32B (0x37A0..0x37BF) before erasing 0x3780 page.
        for (uint8_t i = 0; i < 32; i++)
            preserve[i] = macro_read_byte((uint16_t)(MACRO_SLOT1_ADDR + i));

        erase_flash_page(0x3740);
        erase_flash_page(0x3780);

        // Restore preserved bytes back into shared page.
        for (uint8_t i = 0; i < 32; i += 2)
            macro_write_word((uint16_t)(MACRO_SLOT1_ADDR + i), preserve[i], preserve[i + 1]);
        return;
    }

    // slot == 1
    for (uint8_t i = 0; i < 32; i++)
        preserve[i] = macro_read_byte((uint16_t)(MACRO_SLOT0_ADDR + 64 + i)); // 0x3780..0x379F

    erase_flash_page(0x3780);
    erase_flash_page(0x37C0);

    for (uint8_t i = 0; i < 32; i += 2)
        macro_write_word((uint16_t)(MACRO_SLOT0_ADDR + 64 + i), preserve[i], preserve[i + 1]);
}

void macro_write_word(uint16_t addr, uint8_t lo, uint8_t hi)
{
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    GLOBAL_CFG |= bCODE_WE;
    ROM_ADDR = addr;
    ROM_DATA_L = lo;
    ROM_DATA_H = hi;
    if (ROM_STATUS & bROM_ADDR_OK)
    {
        ROM_CTRL = ROM_CMD_WRITE;
    }
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    GLOBAL_CFG &= ~bCODE_WE;
    SAFE_MOD = 0;
}

uint8_t macro_is_valid(uint8_t slot)
{
    if (slot >= MACRO_SLOT_COUNT)
        return 0;
    return macro_read_byte(macro_slot_addr(slot)) == MACRO_VALID_MARKER;
}

uint16_t macro_action_count(uint8_t slot)
{
    uint16_t base = macro_slot_addr(slot);
    if (macro_read_byte(base) != MACRO_VALID_MARKER)
        return 0;
    uint8_t count = macro_read_byte(base + 1);
    return (count > MACRO_MAX_ACTIONS) ? MACRO_MAX_ACTIONS : count;
}

// ==================== 执行引擎 ====================

extern __xdata uint8_t HIDKey[8];

#define MACRO_IDLE 0
#define MACRO_RUNNING 1
#define MACRO_DELAY 2

__xdata uint8_t macro_m_state;
static __xdata uint8_t m_slot;
__xdata uint8_t macro_m_trigger;
__xdata uint8_t macro_m_key_idx;       // 触发按键索引
static __xdata uint16_t m_act_idx;     // 当前动作索引
static __xdata uint16_t m_act_count;   // 总动作数
static __xdata uint8_t m_key_released; // 触发键已松开
__xdata uint8_t macro_m_cancel_req;    // 取消请求 (TOGGLE)
static __xdata uint16_t m_delay_start;
static __xdata uint16_t m_delay_ms;

uint16_t millis(void);

static void macro_release_all(void)
{
    Keyboard_releaseAll();
    Mouse_releaseAll();
    Consumer_releaseAll();
}

static uint8_t should_loop(void)
{
    switch (macro_m_trigger)
    {
    case MACRO_TRIG_HOLD_ABORT:
    case MACRO_TRIG_HOLD_FINISH:
        return !m_key_released;
    case MACRO_TRIG_TOGGLE:
        return !macro_m_cancel_req;
    default:
        return 0;
    }
}

void macro_execute(uint8_t slot, uint8_t trigger, uint8_t keyIndex)
{
    if (slot >= MACRO_SLOT_COUNT || !macro_is_valid(slot))
        return;

    if (macro_m_state != MACRO_IDLE)
        macro_cancel();

    macro_release_all();

    m_slot = slot;
    macro_m_trigger = trigger;
    macro_m_key_idx = keyIndex;
    m_act_idx = 0;
    m_act_count = macro_action_count(slot);
    m_key_released = 0;
    macro_m_cancel_req = 0;
    macro_m_state = MACRO_RUNNING;
}

void macro_step(void)
{
    __xdata uint16_t addr;
    __xdata uint8_t type;
    __xdata uint8_t param;

    if (macro_m_state == MACRO_IDLE)
        return;

    if (macro_m_state == MACRO_DELAY)
    {
        if ((uint16_t)((uint16_t)millis() - m_delay_start) < m_delay_ms)
            return;
        macro_m_state = MACRO_RUNNING;
    }

    while (m_act_idx < m_act_count)
    {
        addr = macro_slot_addr(m_slot) + MACRO_HEADER_SIZE + (uint16_t)(m_act_idx * 2);
        type = macro_read_byte(addr);
        param = macro_read_byte(addr + 1);
        m_act_idx++;

        switch (type)
        {
        case MACRO_ACT_KEY_DOWN:
            Keyboard_rawPress(param, 0);
            break;
        case MACRO_ACT_KEY_UP:
            Keyboard_rawRelease(param, 0);
            break;
        case MACRO_ACT_MOD_DOWN:
            HIDKey[0] |= param;
            USB_EP1_send(1);
            break;
        case MACRO_ACT_MOD_UP:
            HIDKey[0] &= ~param;
            USB_EP1_send(1);
            break;
        case MACRO_ACT_MOUSE_DOWN:
            Mouse_press(param);
            break;
        case MACRO_ACT_MOUSE_UP:
            Mouse_release(param);
            break;
        case MACRO_ACT_WHEEL:
            Mouse_scroll((int8_t)(param == 1 ? 1 : -1));
            break;
        case MACRO_ACT_CONSUMER:
        {
            uint16_t code = param;
            Consumer_press(code);
            Consumer_release(code);
            break;
        }
        case MACRO_ACT_DELAY:
            m_delay_start = (uint16_t)millis();
            m_delay_ms = (uint16_t)param * 10u;
            macro_m_state = MACRO_DELAY;
            return;
        case MACRO_ACT_END:
            goto round_done;
        default:
            break;
        }

        // HOLD_ABORT: 松开时立即中断
        if (macro_m_trigger == MACRO_TRIG_HOLD_ABORT && m_key_released)
        {
            macro_release_all();
            macro_m_state = MACRO_IDLE;
            return;
        }
    }

round_done:
    if (should_loop())
    {
        m_act_idx = 0;
    }
    else
    {
        macro_release_all();
        macro_m_state = MACRO_IDLE;
    }
}

void macro_on_key_release(void)
{
    m_key_released = 1;
    if (macro_m_trigger == MACRO_TRIG_HOLD_ABORT && macro_m_state != MACRO_IDLE)
    {
        macro_release_all();
        macro_m_state = MACRO_IDLE;
    }
}

void macro_cancel(void)
{
    if (macro_m_state != MACRO_IDLE)
    {
        macro_release_all();
        macro_m_state = MACRO_IDLE;
    }
}
