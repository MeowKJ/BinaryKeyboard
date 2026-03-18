#include "ch552_defs.h"
#include "MacroStorage.h"
#include "CustomUSBHID.h"

// ==================== MeowFS scanning ====================

static uint8_t page_is_blank(uint16_t page_addr)
{
    for (uint8_t i = 0; i < MEOWFS_PAGE_SIZE; i++)
    {
        if (flash_read_byte(page_addr + i) != 0xFF)
            return 0;
    }
    return 1;
}

/* Output via static xdata to avoid 3 generic-pointer params (saves ~9B IRAM/OSEG) */
static __xdata uint8_t  _meta_marker;
static __xdata uint8_t  _meta_count;
static __xdata uint16_t _meta_next;

static uint8_t read_entry_meta(uint16_t pos, uint16_t end)
{
    uint8_t marker;
    uint8_t count;
    uint16_t next_pos;

    if (pos + MEOWFS_HEADER_SIZE > end)
        return 0;

    marker = flash_read_byte(pos);
    count = flash_read_byte(pos + 1);

    if (marker == 0xFF)
        return 0;
    if (marker == 0x00 && count == 0x00)
        return 0;
    if (marker != 0x00 && marker != MEOWFS_VALID_MARKER)
        return 0;

    next_pos = pos + MEOWFS_HEADER_SIZE + (uint16_t)count * MEOWFS_ACTION_SIZE;
    if (next_pos <= pos || next_pos > end)
        return 0;

    _meta_marker = marker;
    _meta_count = count;
    _meta_next = next_pos;
    return 1;
}

void meowfs_format(void)
{
    for (uint8_t i = 0; i < MEOWFS_PAGES; i++)
    {
        uint16_t page_addr = MEOWFS_BASE + (uint16_t)i * MEOWFS_PAGE_SIZE;
        uint8_t retries = 0;
        do
        {
            flash_erase_page(page_addr);
            retries++;
        } while (!page_is_blank(page_addr) && retries < 3);

        if (!page_is_blank(page_addr))
        {
            if (i == 0)
                flash_write_word(MEOWFS_BASE, 0x00, 0x00);
            break;
        }
    }
}

uint16_t meowfs_find_macro(uint8_t index)
{
    uint16_t pos = MEOWFS_BASE;
    uint16_t end = MEOWFS_BASE + MEOWFS_SIZE;
    uint8_t found = 0;

    while (pos + MEOWFS_HEADER_SIZE <= end)
    {
        if (!read_entry_meta(pos, end))
            break;

        if (_meta_marker == MEOWFS_VALID_MARKER)
        {
            if (found == index)
                return pos;
            found++;
        }
        pos = _meta_next;
    }
    return 0;
}

uint8_t meowfs_macro_count(void)
{
    uint16_t pos = MEOWFS_BASE;
    uint16_t end = MEOWFS_BASE + MEOWFS_SIZE;
    uint8_t n = 0;

    while (pos + MEOWFS_HEADER_SIZE <= end)
    {
        if (!read_entry_meta(pos, end))
            break;

        if (_meta_marker == MEOWFS_VALID_MARKER)
            n++;
        pos = _meta_next;
    }
    return n;
}

uint16_t meowfs_used_bytes(void)
{
    uint16_t pos = MEOWFS_BASE;
    uint16_t end = MEOWFS_BASE + MEOWFS_SIZE;

    while (pos + MEOWFS_HEADER_SIZE <= end)
    {
        if (!read_entry_meta(pos, end))
            break;
        pos = _meta_next;
    }
    return (uint16_t)(pos - MEOWFS_BASE);
}

uint8_t meowfs_macro_valid(uint8_t index)
{
    return meowfs_find_macro(index) != 0;
}

// ==================== Execution engine ====================

extern __xdata uint8_t HIDKey[8];

#define MACRO_IDLE 0
#define MACRO_RUNNING 1
#define MACRO_DELAY 2

__xdata uint8_t macro_m_state;
__xdata uint8_t macro_m_trigger;
__xdata uint8_t macro_m_key_idx;
__xdata uint8_t macro_m_cancel_req;

static __xdata uint16_t m_base_addr;
static __xdata uint16_t m_act_idx;
static __xdata uint16_t m_act_count;
static __xdata uint8_t m_key_released;
static __xdata uint16_t m_delay_start;
static __xdata uint16_t m_delay_ms;

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

void macro_execute(uint8_t index, uint8_t trigger, uint8_t keyIndex)
{
    uint16_t addr = meowfs_find_macro(index);
    if (addr == 0)
        return;

    if (macro_m_state != MACRO_IDLE)
        macro_cancel();

    macro_release_all();

    uint8_t count = flash_read_byte(addr + 1);
    if (count > MEOWFS_MAX_ACTIONS)
        count = MEOWFS_MAX_ACTIONS;

    m_base_addr = addr + MEOWFS_HEADER_SIZE;
    macro_m_trigger = trigger;
    macro_m_key_idx = keyIndex;
    m_act_idx = 0;
    m_act_count = count;
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
        addr = m_base_addr + (uint16_t)(m_act_idx * 2);
        type = flash_read_byte(addr);
        param = flash_read_byte(addr + 1);
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
